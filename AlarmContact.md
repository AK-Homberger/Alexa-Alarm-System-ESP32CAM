# External Alarm Contact

It is possible to extend the alarm system with additional door or window contacts.
We can use a battery powered ESP12 module for this task. The module is usually in deep sleep and only wakes up if the conatct is closed. In this case it connects to WLAN and requests the trigger URL to the ESP32-CAM alarm module. After that it goes directry to deep sleep again.

Here is the plan for the external components and connection:

![Schematics](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/Pictures/ESP12AlarmContact.png)

The software for the ESP12 is [here](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/ESP12AlarmContact/ESP12AlarmContact.ino).

Just change the WLAN credentials:
```
// WLAN credentials
const char* ssid = "ssid";
const char* password = "password";
```
And set the IP for the ESP32-CAM module:

```
http.begin("http://192.168.0.51:90/alarm_trigger");  // Change IP to ESP32-CAM alarm system IP
```

Thats all.

For programming the device, you need again the USB-To-Serial adapter. Connect it to RX/TX and GND. Close the Flash switch and press RST. Then upload the sketch. As module you have to set "Generic ESP8266 Device" in the Arduino IDE.

To reduce the required space you can remove the USB-Serial adapter, the Flash switch and RST button after sucessfull programming. 

## Parts:

- ESP12 [Link](https://www.reichelt.de/de/en/index.html?ACTION=446&LA=3&nbc=1&q=esp12)
- Push button [Link](https://www.reichelt.de/miniatur-drucktaster-0-5a-24vac-1x-ein-rt-t-250a-rt-p31772.html?&trstct=pol_12&nbc=1)
- Toggle switch [Link](https://www.reichelt.de/de/en/miniature-toggle-switch-1x-on-off-on-rnd-210-00448-p240580.html?GROUPID=7584&START=0&OFFSET=16&SID=96Xk5YJngRlij1C8dm7WFa8cc43c9fd0145a715a7ea5bf81fdb75&LANGUAGE=EN&&r=1)
- Battery 123A [Link](https://www.reichelt.de/de/en/varta-photo-3-volt-1430-mah-17x34-5mm-varta-cr-123a-p7352.html?search=123a&&r=1)
- Battery holder [Link](https://www.reichelt.de/de/en/battery-holder-for-2-3a-cr-123--halter-2-3a-p44605.html?search=battery+holder+123&&r=1)
- Reed Contact [Link](https://www.reichelt.de/de/en/opening-detector-normally-open-switch-contact-2-core-white-mk-2000w-p62375.html?&nbc=1)

- ## [Back](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/README.md)
