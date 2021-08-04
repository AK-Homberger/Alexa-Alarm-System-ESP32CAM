# Code Details

## Web Interface
The web interface is created with HTML and Javascript(AJAX). The HTML/CSS and Javascript code is stored in the [index.h](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/AlexaIntruderAlert/index.h) file and can be easily changed.

In the main sketch the web requests are handled with the following functions:

```
  server.on("/", handleRoot);             // This is the display page
  server.on("/get_data", handleGetData);  // To get updates of values
  server.on("/on", handleOn);             // Handle On button
  server.on("/off", handleOff);           // Handle Off button
  server.on("/uptime", handleUptime);     // Handle uptime request
````
The events and callback functions are defined in setup(). The [functions](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/f78259da06e159be662db124a53efa886676962c/AlexaIntruderAlert/AlexaIntruderAlert.ino#L269) are called when an URL is requested from the web client.

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
  if (!SILENT_ALARM) digitalWrite(D4, !PIR_On); // Show state on internal LED if not SILENT_ALARM

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

        if (pir_sensor_active) {        // Sensor is active and double move detected
          alarm_time = millis();        // Store time of alarm (to measure alarm delay for disarm)
          alarm_state = true;           // Set alarm status to true
          if (!SILENT_ALARM && USE_ALEXA) ReqURL(1);   // Play ping sound on Alexa if available and not SILENT_ALARM
        }
        Serial.println("Double movement detected.");
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

## Phone Call an Mail Send

The **CallPhone()** and the **SendMail()** functions are direct implementations of the library examples.

**CallPhone:**
```
  String params[][2] = {{"NewX_AVM-DE_PhoneNumber", FB_NUMBER}};
  String req[][2] = {{}};
  connection.action("urn:dslforum-org:service:X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0);
```

The necessary connection init function is called withing the connectWifi() function
```
if (CALL_PHONE) connection.init(); // TR-064 init.
```

**SendMail:**

Sending mails with the library is stright forward:

```
  EMailSender::EMailMessage message;          // Create email message
  message.subject = "Intruder Alert!";
  message.message = time_str;
  EMailSender::Response resp = emailSend.send(M_DEST, message);  // Send email
```
Simply create the message object, set the subject and message text and send the mail. **M_DEST** contains the destination e-mail address.

## HTTPS Request to URL Trigger Service
With the function **ReqURL(i)** one of both defined URL triggers are started (i=0 or i=1).
For security reasons, the service requires a HTTPS connection. To check the identity of the web server, we also check the fingerprint of the server in the code.

Only if the fingerprint check is positive, we will call the URL.

```
void ReqURL(int i) {

  HTTPClient https;   // Create https client object

  client.setFingerprint(vsh_fingerprint);     // Set Virtualsmartome TLS fingerprint

  if (!client.connect("www.virtualsmarthome.xyz", 443)) {     // Check fingerprint for web site
    Serial.println("Fingerprint does not match!");
    return;
  } else Serial.println("Fingerprint does match!");

  if (https.begin(client, URL[i])) {  // Set HTTPS request for URL i

    int httpCode = https.GET();       // Request URL
```
The remaining code in the function is just response checking.


## Interference of WiFi with PIR sensor
The used PIR sensor HC-SR501 is in general a very reliable PIR sensor. But if it is used together with a WLAN device connected to the sensor, then there is a chance for an interference between the WLAN signal and the PIR detection.

The effect is, that you see strange false detections from time to time without any movement.

Here you can find a long discussion regarding the problem: https://www.letscontrolit.com/forum/viewtopic.php?t=671

A good solution for me was an ferrite ring for the cabling between the ESP32CAM and the SR501.

## [Back](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/README.md)
