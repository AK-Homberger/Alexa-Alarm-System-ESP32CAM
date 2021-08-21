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
// External Alarm Contact Sketch for ESP32-CAM Intruder Alert System
// With ESP deep sleep and battery CR123 (3 Volt)

// Version 1.0, 21.08.2021

// Set board to generic ESP8266

// ESP12 wiring:
// Connect GPIO15 to GND
// Connect EN/CH to 3,3V
// Connect D0/GPIO16 with RST for ESP12 timer wakeup

// RST alarm contact to GND               - Push RST button for programming with IDE (GPIO0 must be LOW)
// D3/GPIO0 Programming switch            - Switch to GND and push RST button for programing with IDE

// USB Serial adapter (set to 3,3 Volt logic level !!!!!)
// Connect ESP12 TX to USB Serial RX pin
// Connect ESP12 RX to USB Serial TX pin
// Connect ESP12 GND USB Serial GND
// Connect ESP12 3,3V with USB Serial 3,3V only if no CR123 battery is inserted!!!

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WLAN credentials
const char* ssid = "ssid";
const char* password = "password";

void setup() {
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    i++;
    if (i > 50) ESP.deepSleep(0);
  }

  HTTPClient http;
  http.begin("http://192.168.0.51:90/alarm_trigger?s=Sensorname");  // Change IP to ESP32-CAM alarm system IP
  http.GET();
  http.end();

  ESP.deepSleep(0);
}


void loop() {   // We do nothing in loop()

}
