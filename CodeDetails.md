# Code Details

## Web Interface
The web interface is created with HTML and Javascript(AJAX). The HTML/CSS and Javascript code is stored in the [index.h](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/AlexaIntruderAlert/index.h) file and can be easily changed.

In the main sketch the web requests are handled with the following functions:

```
  server.on("/", handleRoot);             // This is the display page
  server.on("/get_data", handleGetData);  // To get updates of values
  server.on("/on", handleOn);             // Handle On button
  server.on("/off", handleOff);           // Handle Off button
  server.on("/uptime", handleUptime);     // Handle uptime request
  server.on("/test", handleTest);         // Handle test request
````
The events and callback functions are defined in setup(). The [functions](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/b849df58299e61e71e646e5ed7547d1ba0f17cba/AlexaIntruderAlert/AlexaIntruderAlert.ino#L169) are called when an URL is requested from the web client.

The status data is passed to the web client in the **handleGetData()** function (as JSON data).

A good introduction to Javascript/AJAX can be found here: https://www.w3schools.com/js/js_ajax_intro.asp

## Double Movement Detection with FSM
The different states of movement detection and the alarm wait time is implemented as [Finite State Machine](https://en.wikipedia.org/wiki/Finite-state_machine). This is an easy way to maintan different states in a sketch, avoiding complex if/then/else constructs.

The states are defined here:

```
typedef enum        // States for state machine for double movement detection
{
  WAIT_FIRST,
  WAIT_LOW1,
  WAIT_SECOND,
  WAIT_LOW2,
  WAIT_DELAY
} en_fsm_state;
en_fsm_state g_state;
```
And the state transitions are handled here:
```
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
```

The structure is always the same:
- case statement with current state
- if() block
- Set new state

You can see that after the sencond movement detection a picture is stored with the function **capturePhotoSaveSpiffs()**.

## Storage of Photo to SPIFFS
The e-mail library requires a photo either stored to SD or to the internal [SPIFFS](https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm) file systen.

We are using SPIFSS in this project. It is therefore important to select a partition scheme in the Arduino IDE that supports SPIFFS.

```
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
```
The storage process itself is straight forward:

1. Get a JPG picture from the camere: **fb = esp_camera_fb_get();**
2. Open SIFFS filesystem for write with filename for photo: **File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);**
3. Store the photo: **file.write(fb->buf, fb->len);**
4. Close the file and release the fb reserved memory: **file.close(); esp_camera_fb_return(fb);**

## SendMail

Sending mails with the library is stright forward:

```
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
```
First create the attachment for the picture. We will provide the same filename **FILE_PHOTO** for the photo stored within the **capturePhotoSaveSpiffs()** function.

And then create the message object, set the subject and message text and send the mail. **M_DEST** contains the destination e-mail aaddress, **message** the message text and **attachs** the attachement.

## Phone Call

To do phone calls with the TR-064 API is really simple. Only thre lines of code are necessary:

```
  String params[][2] = {{"NewX_AVM-DE_PhoneNumber", FB_NUMBER}};
  String req[][2] = {{}};
  connection.action("urn:dslforum-org:service:X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0);
```
**FB_NUMBER** Contains the number to be dialled.

The necessary connection init function is called withing the connectWifi() function.
```
if (CALL_PHONE) connection.init(); // TR-064 init.
```

## Alexa Device Name and Callback Function
Alexa devices are added with the defined name **AlexaName** with the following commands in **setup()**:
```
if (USE_ALEXA) {                      // Add Alexa device with device name and set callback function
    device = new EspalexaDevice(AlexaName, AlertChanged);
    espalexa.addDevice(device);
    espalexa.begin();
  }
```
The callback function **AlertChanged()** is called later in the event of Alexa commands for the defined devices.
With **espalexa.begin()** the service will be started.

```
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

```
The callback function is called for every Alexa command to the system. Within the function you can simply check the device state and react on the command. Brightness 0 means Off, 1-255 means On with dim level.
Here we simply control the arm and off state.

The Alexa device state is also maintained if the web interface is used:
``` 
 handleOn() {
 if (USE_ALEXA) device->setValue(255);  // Set Alexa state On
```
 and 
```
 handleOff() {
 if (USE_ALEXA) device->setValue(0);   // Set Alexa state Off
``` 

## Interference of WiFi with PIR sensor
The used PIR sensor HC-SR501 is in general a very reliable PIR sensor. But if it is used together with a WLAN device connected to the sensor, then there is a chance for an interference between the WLAN signal and the PIR detection.

The effect is, that you see strange false detections from time to time without any movement.

Here you can find a long discussion regarding the problem: https://www.letscontrolit.com/forum/viewtopic.php?t=671

A good solution for me was an ferrite ring for the cabling between the ESP32CAM and the SR501.

## [Back](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/README.md)
