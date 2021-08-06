# External Alarm Contact

It is possible to extend the alarm system with additional door or windows contacts.
We use a battery powered ESP12 module for this task. The module is usually in deep sleep and only wakes up if the conatct is closed. In this case it connects to WLAN and requests the trigger URL to the ESP32-CAM alarm module. After that it goes directry to deep sleep again.

Here is the plan for the external components and connection.

![Schematics](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/Pictures/ESP12AlarmContact.png)

The softwre is [here](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/ESP12AlarmContact/ESP12AlarmContact.ino).

Just change the WLAN credentials:
```
// WLAN credentials
const char* ssid = "ssid";
const char* password = "password";
```
And set the IP for the ESP32-CAM module:

http.begin("http://192.168.0.51:90/alarm_trigger");  // Change IP to ESP32-CAM alarm system IP

Thats all.

For programming the device you need again the USB-To-Serial adapter. For programming connect it to RX/TX and GND. Close the Flash switch and press RST. The upload the sketch.

As module you have to set "Generic ESP8266 Device" in the Arduino IDE.


