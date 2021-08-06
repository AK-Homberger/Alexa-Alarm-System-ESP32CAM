# External Alarm Contact

It is possible to extend the alarm system with additional door or windows contacts.
We use a battery powered ESP12 module for this task. The module is usually in deep sleep and only wakes up if the conatct is closed. In this case it connects to WLAN and requests the trigger URL to the ESP32-CAM alarm module. After that it goes directry to deep sleep again.

[Schematics](https://github.com/AK-Homberger/Alexa-Alarm-System-ESP32CAM/blob/main/Pictures/ESP12AlarmContact.png)

The softwre is [here](
