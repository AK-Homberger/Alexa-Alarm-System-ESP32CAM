# Alexa integrated Alarm System
This is an Alexa controlled intruder alert system with a ESP32CAM and a HC-SR501 PIR motion detection module.

![Front](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Housing-Front.jpg)

# Features
- The [Espalexa](https://github.com/Aircoookie/Espalexa) library (Hue emulation) is used to switch On/Off motion detection via Alexa voice commands
- The www.virtualsmarthome.xyz URL trigger service is used to start Alexa routines in case of an alarm
- Activation status and video stream can be controlled via web interface
- Can call phones via [fritzbox](https://en.avm.de/products/fritzbox/) TR-064 API (only on certain routers)
- Can send e-mail notifications with picture via gmail account
- Last activation state is stored in EEPROM and restored after reboot (e.g. after a power loss)

# Usage
Just place the alarm module somewhere in your home where it can detect suspicious movements with the infrared PIR sensor. The module has to be powered via the USB connector.

Then activate the detector with an Alexa voice command: "Alexa, Alert On". There is a 60 seconds arm delay time before the detector is activated. This shall allow leaving the home before the motion detector is activated. The device name can be changed in the  settings section. Set the device name to something special, to ensure an intruder is not guessing the device name to switch off the alarm.

To avoid false alarms, the detector is programmed to wait for two detected motions within 30 seconds. 

If a (double)motion is detected while the detector is activated, then a alarm notification is done (phone call, e-mail). You can also trigger Alexa routines in case of an alarm. You can for example let Alexa play an alarm sound, speak a text or switch on/off lights. It's totally up to you.

To allow disarming of the detector when coming back home, there is an alarm delay time of 20 seconds defined. During this 20 seconds you can switch the module off with an "Alexa, Alert Off" command.

After an alarm, there is a wait time defined of 5 minutes before a next alarm can be raised. The wait time can be stopped with any On/Off command.

The alert system can also be controlled with a [web interface](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/README.md#web-interface) with (IP-Address:90).

# Hardware

The whole alarm system consists of two components only. The ESP32CAM and the HC-SR501 PIR sensor (less then 15 Euro).
See [Parts](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/README.md#parts) section for order details.

The HC-SR501 PIR motion detection sensor is connected to the ESP32CAM with three wires:

## ESP <-> SR501
- 5V to VCC
- GND to GND
- GPIO2 to OUT (signal output of HC-SR501)

That's all. 

You can either solder the wires or you can use jumper wires instead. Power is provided via the 5 Volt pin of ESP32CAM.

![Connected](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Connected2.jpg)

![Back](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Housing-Back.jpg)

![Front](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Housing-Front.jpg)

# Software

The [software](https://github.com/AK-Homberger/Alexa-Alarm-System/tree/main/AlexaIntruderAlert) is created to be used within the Arduino IDE (tested with version 1.8.13). For the ESP32CAM, the ESP32 board support has to be installed in the IDE.

The following external libraries are used:

- EMailSender   (https://github.com/xreef/EMailSender)
- Espalexa      (https://github.com/Aircoookie/Espalexa)
- NTPClient     (https://github.com/arduino-libraries/NTPClient)
- ArduinoJson   (https://arduinojson.org/v6/doc/)
- TR-064        (https://github.com/Aypac/Arduino-TR-064-SOAP-Library)

Most external libraries can be installed via the Library Manager. If not possible, install as ZIP file downloaded from GitHub (green button "Code").

After uploading the sketch to the ESP32CAM via an FTDI USB to serial adapter, you can start the device detection with Alexa (use "Other devices" not "Pilips Hue").

After the initial sketch upload via USB, you can also do "Over the Air" updates via WLAN. The OTA device name is set to "IntruderAlertCam".

## Settings
To use the sketch, just change the relevant settings in the code:

```
// Change credentials, Alexa device name and Trigger URLs !!!
//*******************************************************************************
#define CALL_PHONE On                   // Call phone On/Off      
#define SEND_MAIL On                    // Send mail On/Off
#define USE_ALEXA On                    // Alexa service On/Off
#define SILENT_ALARM Off                // On means no internal LED blink and no Alexa notification sound
```
You can switch individually the notification methods on or off. Phone calls are only possible with special kind of routers (Fritzbox from AVM). You can also switch silent alarm on. That means, no internal LED is blinking and no Alexa notification sound is played.

The alarm system is mainly created for Alexa integration, but it works also without Alaxa. You can control it then via the web interface.

## WLAN Settings
The WiFi credentials has to be set according to your local needs.
```
// WLAN credentials
char* ssid = "ssid";
char* password = "password";
```

## Espalexa HUE Emulation / Device Name
The [Espalexa](https://github.com/Aircoookie/Espalexa) Philips Hue emulation service is emulating a device that can be discovered and controlled from Alexa.
The alarm system can be armed/disarmed with Alexa voice commands (device on/off). Just define the device name to be used with Alexa. When activated via Alexa command, there will be 60 seconds arm delay time until the alarm system is activated. That shall allow to leave the house before the system is activated.

```
// Alexa device name
char* AlexaName = "Alert";              // Define your Alexa device name
```

## TR-064 Phone Call support for Fritzbox
The [TR-064 API](https://github.com/Aypac/Arduino-TR-064-SOAP-Library) allows to place calls as alarm notification. But this works only for certain WLAN routers with telephone support. Mainly [AVM Fritzbox](https://en.avm.de/products/fritzbox/) and some Zyxel models.

To use TR-064 on a Fritzbox, you have to create a user on the Fritzbox and enable the "dial support" function (Wählhilfe). See details within the library documentation.

The TR-064 credentials have to be configured in this section of the sketch:

```
// Fritzbox credentials for TR064
char* FB_NUMBER = "0123456789";         // Phone number to dial
char* FB_USER = "user";                 // Fritzbox user name
char* FB_PASSWORD = "password";         // Password for user
char* FB_IP = "192.168.0.1";            // IP of Fritzbox
int  FB_PORT = 49000;
```
## Email Notification via gmail Account

Email alert notification can be used with the [EMailSender](https://github.com/xreef/EMailSender) library. The library might be usable also with other providers. But the easiest way is to use a Google mail account for sending e-mail notification.

Just create a new gmail account for SMTP notifications and place the credentials here.
Important: Allow less secure connections in gmail security settings. 

With "destination@domain.de" you can define the e-mail destination address.
In case of an alarm, a notification is sent to this e-mail address with the subject "Intruder Alert!" and a text "time stamp: Movement detected!".

```
// Mail credentials for gmail account. Create new gmail alert account if needed.
// Allow access for less secure clients in gmail security settings.
char* M_USER = "myaccount@gmail.com";   // Mail user name
char* M_PASSWORD = "password";          // Mail password
char* M_DEST = "destination@domain.de"; // Destination e-mail address
```

## Start Alexa Routines with VSH URL Trigger Service
To start Alexa routines, you have to register on the www.virtualsmarthome.xyz web site with your Amazon account and also enable the "URL Routine Trigger" skill in Alexa. In Alexa, the URL triggers are shown as virtual doorbells.

Then define two URL triggers as shown in the screenshot:

![VSH](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/VSH-URL-Trigger.png)

The URLs have to be copied to the sketch below.

```
// Trigger URLs: 1. Alexa alarm routine. 2. Ping notification sound
const char *URL[] PROGMEM = {"https://www.virtualsmarthome.xyz/url_routine_trigger/activate.php?trigger=",
                             "https://www.virtualsmarthome.xyz/url_routine_trigger/activate.php?trigger="
                            };
```
The first URL is the alarm routine trigger and the second the notification routine trigger.
Use the JSON variant (short response). But all three variants will work.

In the Alexa APP you can then create two routines. One is the routine for alarm activities (play sound, speak text etc.). And the second is a notification tone, as a reminder to disarm the alarm system when coming back home. There is 20 seconds alarm delay until the alarm is raised.

![Alexa1](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Alexa1.png)
![Alexa2](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/Alexa2.PNG)

# Web Interface
In addition to Alexa commands, the system can be controlled also with a web interface. Use **IP-address:90** to start the interface. Port 80 is used for the Espalexa service. The IP address is shown during the start of the sketch in the serial monitor of the IDE.

![Web](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/Pictures/web-interface.png)

You can use On/Off commands to arm/disarm the system. When armed with the web interface, the system is directly activated without an arm delay time.

The "Active (0)" state also shows the trigger counts for single PIR triggers. 

The **IP-address:90/uptime** web request, shows the current time, the uptime (in hours), the remaining memory heap of the system and single/double PIR counters. The maximum picture rate is also shown.
```
Time: 09:16:03
Uptime: 13 (hours)
Free Heap: 27728
Single=6 Double=2
Max FPS=10
```
The counters shall support the setting of the sensitivity of the PIR module. If the sensitivity is too high, then false triggers are happening. An On/Off command is setting both values to zero.

To avoid false alarms, the software always waits for two PIR trigger events within 30 seconds.

# Sensitivity Adjustment
To adjust the sensitivity on the HC-SR501 module, you have to change the left potentiometer. Turn counter clockwise to reduce the sensitivity. Reduce the sensitivity so far, to ensure that there are not too many wrong PIR triggers (e.g. because of normal temperature changes). 
The right potentiometer is defining the delay time. Keep this a the shortest time (about 3 seconds).
And leave the standard setting for single triggers (jumper to the outside).

![SR501](https://www.makerguides.com/wp-content/uploads/2019/07/HC-SR501-Pinout-Annotation.jpg)

If you still got false positives, you should have a look to [WLAN interference](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/CodeDetails.md#interference-of-wifi-with-pir-sensor) section in the code details.

A nice tutorial for the HC-SR501 can be found [here](https://www.makerguides.com/hc-sr501-arduino-tutorial/).

# TLS Fingerprints

To improve security against man-in-the-middle-attacks, the sketch is checking the TLS fingerprint for the www.virtualsmarthome.xyz web site. The fingerprint is connected to the certificate of the web site. If the certificate is renewed/changed then also a new fingerprint for the site has to be retrieved and stored in the sketch. 

A full certificate check as with the ESP32 is currently not supported with the ESP8266.

The fingerprint is shown together with the other certificate details within the browser. The current certificate will expire in September 2021.

```
// Fingerprint for www.virtualsmarthome.xyz
const char* vsh_fingerprint PROGMEM = "0F EA 45 74 82 2F 25 2E 70 0F 63 1C E4 43 6E 7E BA 8C A3 EF";
```

There is also a fingerprint stored for gmail. But currently the e-mail library is not supporting fingerprint checks.

# Code Details
If you are interested in code details and explanations please read [here](https://github.com/AK-Homberger/Alexa-Alarm-System/blob/main/CodeDetails.md) further.

# Parts
- ESP32CAM [Link](https://www.reichelt.de/index.html?ACTION=446&LA=2&nbc=1&q=esp32cam)
- HC-SR501 PIR Sensor [Link](https://www.reichelt.de/raspberry-pi-infrarot-bewegungsmelder-pir-hc-sr501-rpi-hc-sr501-p224216.html)

# Updates:
- 04.08.21: Version 1.0: Initial version.
