/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Alexa controlled intruder alert with ESP32CAM and SR501 PIR module
//
// - Is using Espalexa Alexa library (Hue emulation) to switch On/Off motion detection via Alaxa voice commands
// - Is using www.virtualsmarthome.xyz URL trigger service to start Alexa routines
// - Status can be controlled also via web interface (port 90)
// - Can call phones via fritzbox TR-064 API
// - Can send e-mail notifications with picture via gmail account
//
// Version 1.0 - 04.08.2021

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <esp_camera.h>
#include <SPIFFS.h>
#include <FS.h>

// Most external libraries can be installed via Library Manager.
// If not, install as ZIP file downloaded from GitHub (green button "Code")
#include <ArduinoWebsockets.h>  // https://github.com/gilmaimon/ArduinoWebsockets
#include <EMailSender.h>        // https://github.com/xreef/EMailSender
#include <Espalexa.h>           // https://github.com/Aircoookie/Espalexa
#include <NTPClient.h>          // https://github.com/arduino-libraries/NTPClient
#include <ArduinoJson.h>        // https://arduinojson.org/v6/doc/
#include <tr064.h>              // https://github.com/Aypac/Arduino-TR-064-SOAP-Library

// Local includes
#include "index.h"          // Web page code (HTML/JS)
#include "certificate.h"    // Root certificate for www.virtualsmarthome.xyz
#define CAMERA_MODEL_AI_THINKER  // Camera model for ESP32-CAM
#include "camera_pins.h"

#define PIR_SENSOR_PIN 2    // PIR sensor connected to GPIO 2 on ESP32CAM
#define ARM_DELAY 60000     // 60 seconds delay after Alexa On command before armed
#define ALARM_DELAY 15000   // 15 seconds delay to allow Alexa Off command before alarm is raised
#define ALARM_WAIT 300000   // 5 minutes wait time between alarms

#define On true
#define Off false
#define LED_BUILTIN 4

#define FILE_PHOTO "/photo.jpg" // Filename of picture stored to SPIFFS


// Change credentials, Alexa device name and Trigger URLs !!!
//*******************************************************************************
#define CALL_PHONE On                   // Call phone On/Off      
#define SEND_MAIL On                    // Send mail On/Off
#define USE_ALEXA On                    // Alexa service On/Off
#define SILENT_ALARM Off                // On means no internal LED blink and no Alexa notification sound

// WLAN credentials
char* ssid = "ssid";
char* password = "password";

// Alexa device name
char* AlexaName = "Alert";              // Define your Alexa device name

// Fritzbox credentials for TR064
char* FB_NUMBER = "0123456789";         // Phone number to dial
char* FB_USER = "user";                 // Fritzbox user name
char* FB_PASSWORD = "password";         // Password for user
char* FB_IP = "192.168.0.1";            // IP of Fritzbox
int  FB_PORT = 49000;

// Mail credentials for gmail account. Create new gmail alert account if needed.
// Allow access for less secure clients in gmail security settings.
char* M_USER = "myaccount@gmail.com";   // Mail user name
char* M_PASSWORD = "password";          // Mail password
char* M_DEST = "destination@domain.de"; // Destination e-mail address

// Trigger URLs: 1. Alexa alarm routine. 2. Ping notification sound
const char *URL[] PROGMEM = {"https://www.virtualsmarthome.xyz/url_routine_trigger/activate.php?trigger=",
                             "https://www.virtualsmarthome.xyz/url_routine_trigger/activate.php?trigger="
                            };

//*******************************************************************************
WiFiClientSecure client;            // Create HTTPS client
WiFiUDP ntpUDP;                     // Create UDP object
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200); // Define NTP Client to get time
WebServer server(90);               // Create web server on port 90. Port 80 is used for Alexa service

using namespace websockets;
WebsocketsServer socket_server;     // Create Web Socket server
bool no_socket_connection = true;   // Socket connection flag

Espalexa espalexa;                  // Create Espalexa object
EspalexaDevice* device;             // Create an Espalexa device (to set state)
TR064 connection(FB_PORT, FB_IP, FB_USER, FB_PASSWORD);  // Create TR064 object
EMailSender emailSend(M_USER, M_PASSWORD);  // Create EMailSender object

boolean pir_sensor_active = false;  // PIR sensor status (armed/disarmed)
boolean alarm_state = false;        // True if double movement detected and alarm raised
unsigned long double_time = 0;      // Timer for double movement detection
unsigned long alarm_time = 0;       // Timer for alarm delay to allow disarm via Alexa Off command
boolean arm = false;                // Arm request state. True after On command from Alexa
unsigned long arm_time = 0;         // Timer for arm delay after On command from Alexa
int sent_mail_counter = 0;          // Sent mail counter. Stop sending after 20 mails.
int single_counter = 0;             // To count single movements
int double_counter = 0;             // To count double movements
boolean wifiConnected = false;      // WiFi connection status
camera_fb_t* fb;                    // Frame buffer pointer for picture from camera
unsigned long picture_timer = 0;    // Time between pictures send to web client
static int fps = 10;                // Target pictures per second
int max_fps = 0;                    // Maximum FPS measured

typedef enum        // States for state machine for double movement detection
{
  WAIT_FIRST,
  WAIT_LOW1,
  WAIT_SECOND,
  WAIT_LOW2,
  WAIT_DELAY
} en_fsm_state;
en_fsm_state g_state;

// Prototypes
boolean connectWifi();


//*******************************************************************************
//
void setup() {
  Serial.begin(115200);

  esp_err_t result = camera_init();    // Initialise camera

  if (result != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", result);
    delay(2500);
    ESP.restart();
  }

  if (!SPIFFS.begin(true)) {    // Initialise internal filesystem of flash memory
    Serial.println("An Error has occurred while mounting SPIFFS");
    delay(2500);
    ESP.restart();
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }

  pinMode(PIR_SENSOR_PIN, INPUT);      // PIR sensor pin as Input
  pinMode(LED_BUILTIN, OUTPUT);        // Internal LED as output

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  server.on("/", handleRoot);             // This is the display page
  server.on("/get_data", handleGetData);  // To get updates of values
  server.on("/on", handleOn);             // Handle On button
  server.on("/off", handleOff);           // Handle Off button
  server.on("/uptime", handleUptime);     // Handle uptime request
  server.on("/test", handleTest);         // Handle test request

  wifiConnected = connectWifi();          // Initialise wifi connection

  if (!wifiConnected) {
    Serial.println("Cannot connect to WiFi. Please check data. Reset the ESP now.");
    delay(2500);
    ESP.restart();
  }

  timeClient.begin();   // Start NTP time client to get current time

  client.setCACert(rootCACertificate);  // Set root CA certificate of VSH

  EEPROM.begin(16);
  pir_sensor_active = EEPROM.read(0);   // Read last arm/disarm state e.g. to set after reset

  server.begin();                       // Start web server handling
  Serial.println("Web server startet.");

  socket_server.listen(91);             // Start web socket server on port 91 (for streaming)

  if (USE_ALEXA) {                      // Add Alexa device with device name and set callback function
    device = new EspalexaDevice(AlexaName, AlertChanged);
    espalexa.addDevice(device);
    espalexa.begin();
  }

  ArduinoOTA.setHostname("IntruderAlertCam");     // Arduino OTA config and start
  ArduinoOTA.begin();

  g_state = WAIT_FIRST;  // Wait for first PIR signal
  Serial.println("Wait for first movement.");
}


//*****************************************************************************
// Capture photo to SPIFFS
//
void capturePhotoSaveSpiffs( void ) {
  // Take a photo with the camera
  Serial.println("Taking a photo...");

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO);
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO);
    Serial.print(" - Size: ");
    Serial.print(file.size());
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}


//*****************************************************************************
// Configure camera settings
//
esp_err_t camera_init(void) {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // Frame size 1/4 VGA as required for face detection
  config.jpeg_quality = 10;
  config.fb_count = 1;

  return esp_camera_init(&config);
}


//*******************************************************************************
// Connect to wifi – returns true if successful or false if not
//
boolean connectWifi() {
  boolean state = true;
  int i = 0;

  //wifi_set_phy_mode(PHY_MODE_11G);  // This shall reduce interference of WLAN with PIR sensor
  //system_phy_set_max_tpw(32);     // Reduce WLAN power further if necesry

  WiFi.mode(WIFI_STA);
  WiFi.hostname("IntruderAlertCam");
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (CALL_PHONE) connection.init(); // TR-064 init.
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}


//*******************************************************************************
// Check WiFi and try WiFi restart id connection is lost. Restart ESP if not sucessful
//
void Check_WiFi(void) {
  boolean wifiConnected = false;

  if (WiFi.status() != WL_CONNECTED) { // Wifi restart
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    Serial.println("Reconnect!");
    wifiConnected = connectWifi();

    if (!wifiConnected) {
      Serial.println("Cannot connect to WiFi. Please check data. Reset the ESP now.");
      delay(2500);
      ESP.restart();
    }
  }
}


//*******************************************************************************
// This function is called when a command from Alexa is received
//
void AlertChanged(uint8_t brightness) {
  if (brightness) {                           // On command (brightness > 0). Arm device. On after 60 seconds
    Serial.println("Arm");
    pir_sensor_active = false;
    arm = true;
    arm_time = millis();
    alarm_state = false;
    g_state = WAIT_FIRST;  // Wait for first PIR signal
    Serial.println("Wait for first movement.");
    EEPROM.write(0, true);
  }
  else {                                     // Off command. Disarm device (Off)
    Serial.println("Off");
    pir_sensor_active = false;
    arm = false;
    alarm_state = false;
    g_state = WAIT_FIRST;  // Wait for first PIR signal
    Serial.println("Wait for first movement.");
    EEPROM.write(0, false);
  }
  EEPROM.commit();
}


//*****************************************************************************
// Send main web page if no WebSocket connection is established
// Otherwise error message
//
void handleRoot() {
  if (no_socket_connection) {
    server.send(200, "text/html", MAIN_page); // MAIN_page is defined in index.h
  } else {
    server.send(200, "text/plain", "Sorry, only one (WebSocket) connection possible!");
  }
}


//*******************************************************************************
// Send sensor and alarm state as JSON data
//
void handleGetData() {
  char Text[40];
  String HTML_Text;

  StaticJsonDocument<200> root;

  if (pir_sensor_active) {
    snprintf(Text, sizeof(Text), "Active (%d)", single_counter);
    root["psa"] = Text;
  } else if (arm) {
    root["psa"] = "Arm";
  } else {
    //snprintf(Text, sizeof(Text), "Off (%d/%d)", single_counter, double_counter);
    root["psa"] = "Off";
  }

  if (alarm_state) {
    snprintf(Text, sizeof(Text), "Alarm delay (%d)", (ALARM_DELAY - (millis() - alarm_time)) / 1000);
    root["as"] = Text;
  }
  else if (arm) {
    snprintf(Text, sizeof(Text), "Arm delay (%d)", (ARM_DELAY - (millis() - arm_time)) / 1000);
    root["as"] = Text;
  }
  else if (g_state == WAIT_FIRST) {
    root["as"] = "Wait first";
  }
  else if (g_state == WAIT_LOW1) {
    root["as"] = "Movement 1";
  }
  else if (g_state == WAIT_SECOND) {
    root["as"] = "Wait second";
  }
  else if (g_state == WAIT_LOW2) {
    root["as"] = "Movement 2";
  }
  else if (g_state == WAIT_DELAY) {
    snprintf(Text, sizeof(Text), "Alarm! Wait (%d)", (ALARM_WAIT - (millis() - alarm_time)) / 1000);
    root["as"] = Text;
  }

  serializeJsonPretty(root, HTML_Text);
  server.send(200, "text/plain", HTML_Text); //Send  values to client ajax request
}


//*******************************************************************************
// On button in web interface pressed
//
void handleOn() {
  if (USE_ALEXA) device->setValue(255);   // Set Alexa state On
  pir_sensor_active = true;
  alarm_state = false;
  arm = false;
  g_state = WAIT_FIRST;  // Wait for first PIR signal
  Serial.println("Wait for first movement.");
  EEPROM.write(0, true);
  EEPROM.commit();
  single_counter = 0;    // Reset counter to 0
  double_counter = 0;
  max_fps = 0;
  server.send(200, "text/html");
}


//*******************************************************************************
// Off button in web interface pressed
//
void handleOff() {
  if (USE_ALEXA) device->setValue(0);   // Set Alexa state Off
  pir_sensor_active = false;
  alarm_state = false;
  arm = false;
  g_state = WAIT_FIRST;  // Wait for first PIR signal
  Serial.println("Wait for first movement.");
  EEPROM.write(0, false);
  EEPROM.commit();
  single_counter = 0;    // Reset counter to 0
  double_counter = 0;
  max_fps = 0;
  server.send(200, "text/html");
}


//*****************************************************************************
// Handle uptime request
// Show time, uptime and free heap size (to detect memory leaks)
// Single, double movements and max fps are also shown
//
void handleUptime() {
  char time_str[40];
  char text[120];

  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());

  snprintf(text, sizeof(text), "Time: %s\nUptime: %d (hours)\nFree Heap: %d\nSingle=%d Double=%d\nMax FPS=%d\n",
           time_str, millis() / 3600000, ESP.getFreeHeap(), single_counter, double_counter, max_fps);

  server.send(200, "text/plain", text);
}


//*****************************************************************************
// Handle Test
// Request Alexa notification sound to test VSH service
//
void handleTest() {
  ReqURL(1);
  server.send(200, "text/plain", "Test Request");
}


//*******************************************************************************
// Unknown request. Send error 404
//
void handleNotFound() {
  server.send(404, "text/plain", "File Not Found\n\n");
}


//*******************************************************************************
// Handle PIR sensor FSM state changes and set alarm state and alarm time
//
void Handle_PIR_Sensor(void) {
  bool PIR_On;

  PIR_On = digitalRead(PIR_SENSOR_PIN);   // Read PIR sensor state
  if (!SILENT_ALARM) digitalWrite(LED_BUILTIN, PIR_On); // Show state on internal LED if not SILENT_ALARM

  if (alarm_state) return;                // An alarm is currently active, return

  switch (g_state) {    // Handle FSM state changes

    case WAIT_FIRST:    // Wait for first movement (signal to high). State change from WAIT_FIRST to WAIT_LOW1.
      if (PIR_On) {
        Serial.println("First movement detected.");
        single_counter++;
        g_state = WAIT_LOW1;
        double_time = millis();
      }
      break;

    case WAIT_LOW1:   // Wait for signal back to low. State change from WAIT_LOW1 to WAIT_SECOND
      if (!PIR_On) {
        Serial.println("Wait for second movement.");
        g_state = WAIT_SECOND;
      }
      break;

    case WAIT_SECOND: // Check for double movement: State is WAIT_SECOND and PIR sensor high within 30 seconds.
      if (PIR_On && millis() < double_time + 30000) {

        capturePhotoSaveSpiffs();       // Store picture

        if (pir_sensor_active) {        // Sensor is active and double move detected
          alarm_time = millis();        // Store time of alarm (to measure alarm delay for disarm)
          alarm_state = true;           // Set alarm status to true
          if (!SILENT_ALARM && USE_ALEXA) ReqURL(1);   // Play ping sound on Alexa if available and not SILENT_ALARM
        }
        Serial.println("Double movement detected.");
        single_counter--;
        double_counter++;
        g_state = WAIT_LOW2;
      } else if (millis() >= double_time + 30000 ) { // No second movement. Set state to WAIT_FIRST
        Serial.println("Wrong alarm.");
        g_state = WAIT_FIRST;
      }
      break;

    case WAIT_LOW2:     // Wait for signal back to low. State change from WAIT_LOW2 to WAIT_FIRST
      if (!PIR_On) {
        Serial.println("Wait for first movement.");
        g_state = WAIT_FIRST;
      }
      break;

    case WAIT_DELAY:     // Wait 5 minutes after alarm. Then WAIT_DELAY to WAIT_FIRST
      if (millis() > alarm_time + ALARM_WAIT) {
        Serial.println("Wait for first movement.");
        g_state = WAIT_FIRST;
      }
      break;
  }
}


//*****************************************************************************
// Request URL i
//
void ReqURL(int i) {
  HTTPClient https;

  if (https.begin(client, URL[i])) {  // Set HTTPS request for URL i

    int httpCode = https.GET();       // Request URL

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);      // Show web server response
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());

      ESP.restart();  // Restart ESP in case of connection problems
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}


//*******************************************************************************
// Send alarm mail with alarm time and picture
//
boolean SendMail(void) {
  char time_str[80];

  if (sent_mail_counter++ > 20) return false;  // Stop after 20 mails (avoid spamming in case of malfunction)

  Serial.println("Sending mail.");
  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d: Movement detected!", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());

  EMailSender::FileDescriptior fileDescriptor[1];   // Attach picture
  fileDescriptor[0].filename = "photo.jpg";
  fileDescriptor[0].url = FILE_PHOTO;
  fileDescriptor[0].mime = "image/jpg";
  fileDescriptor[0].encode64 = true;
  fileDescriptor[0].storageType = EMailSender::EMAIL_STORAGE_TYPE_SPIFFS;

  EMailSender::Attachments attachs = {1, fileDescriptor};

  EMailSender::EMailMessage message;    // Create email message
  message.subject = "Intruder Alert!";
  message.message = time_str;

  EMailSender::Response resp = emailSend.send(M_DEST, message, attachs);  // Send email
  Serial.println("Sending status: ");   // Print status
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);

  return true;
}


//*******************************************************************************
// Call phone via Fritzbox. See TR064 library for details
//
void CallPhone(void) {
  Serial.println("Call phone!");
  String params[][2] = {{"NewX_AVM-DE_PhoneNumber", FB_NUMBER}};
  String req[][2] = {{}};
  connection.action("urn:dslforum-org:service:X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0);
}


//*******************************************************************************
// Handle updates for alarm and libraries
//
void handle_updates(void) {

  // Wait ARM_DELAY time after Alexa On command before armed
  if (arm == true && millis() > arm_time + ARM_DELAY) {
    pir_sensor_active = true;
    alarm_state = false;
    arm = false;
    g_state == WAIT_FIRST;  // Wait for first PIR signal
    Serial.println("Wait for first movement.");
  }

  // Alarm detected. Wait ALARM_DELAY time to allow disarm via Alexa or do notification
  if (alarm_state && millis() > alarm_time + ALARM_DELAY) {
    Serial.println("Alarm.");

    if (CALL_PHONE) CallPhone();  // Call phone number
    if (SEND_MAIL) SendMail();    // Send e-mail
    if (USE_ALEXA) ReqURL(0);     // Start Alexa alarm routine via VSH

    alarm_state = false;   // Alarm Off
    g_state = WAIT_DELAY;  // Wait 5 minutes before next alert
    Serial.println("Wait 5 minutes.");
  }

  ArduinoOTA.handle();            // Handle OTA updates
  if (USE_ALEXA) espalexa.loop(); // Handle Alexa communication
  Handle_PIR_Sensor();            // Handle PIR sensor detection changes
  server.handleClient();          // Handle web server requests
  Check_WiFi();                   // Check WiFi connection status and try to reconnect if connection is lost
  timeClient.update();            // Handle NTP client time server updates
}


//*******************************************************************************
//
void loop() {
  
  while (!socket_server.poll()) {   // No web socket connection
    no_socket_connection = true;
    handle_updates();
  }

  WebsocketsClient client = socket_server.accept();  // Accept web socket connection

  while (client.available()) {  // Loop as long as a client is connected
    no_socket_connection = false;
    handle_updates();
    client.poll();

    if (millis() > picture_timer + (1000 / fps)) {      // Is it time to send next picture
      int current_fps = (1000.0 / (millis() - picture_timer)) + 0.5; // Calculate current fps rate
      if (current_fps > max_fps) max_fps = current_fps; // Set new max_fps
      picture_timer = millis();

      fb = esp_camera_fb_get();       // Get frame buffer pointer from camera (JPEG picture)
      client.sendBinary((const char *)fb->buf, fb->len); // Send frame buffer (JPEG picture) to client
      esp_camera_fb_return(fb);       // Release frame buffer
    }
  }
  client.close();
}
