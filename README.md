# IoT-Rainwater-Tank
IoT Rainwater tank with ESP8266. Web Server &amp; Client, OLED Dispaly, HC-SR04, SPIFFS
<br>
On Youtube: https://www.youtube.com/watch?v=UjX3-XNddWY  (
Sorry, the Video is in German only.)
![alt tag](https://github.com/DIYDave/IoT-Rainwater-Tank/blob/master/photos/Github.jpg)
<br>
This is about a rainwater tank which, thanks to an ESP8266, independently gets the weather data and sends a push message to the smartphone if the current water level in the tank is too high for the expected rainfall.

ESP8266 NodeMCU1.0(ESP12E)
Act as HTML-5 Web server and manage multiple client tasks

Sends push notification over Prowl to iOS devices

Sends data to thingspeak.com

Measures water level with ultrasonic

Controls two pumps start / stop over HTML or radio remote control

List of resources:

Hardware:
- ESP8266:                  http://s.click.aliexpress.com/e/cKcNI4xI
- OLED Display:          http://s.click.aliexpress.com/e/bJe1XnpQ
- Ultrasonic:                http://s.click.aliexpress.com/e/c5ogZbmC
- Housing to it (3D Print) : https://goo.gl/QfyFNG
- M8 Cable 4p:           http://s.click.aliexpress.com/e/cgtmaWcg
- M8 Connector 4p:    http://s.click.aliexpress.com/e/bTbkFVfi
- Relays board             http://s.click.aliexpress.com/e/b0hN6Y0g
- RF Sender / Receiver:     http://s.click.aliexpress.com/e/cGoPzU1K
12V type modified to 5V supply and 5V output to the ESP
You can select a 5V type i.e. : http://s.click.aliexpress.com/e/braq4jEu

Useful:
Cheap but good Fluke Multimeter:  http://s.click.aliexpress.com/e/tH9jMvIs

Cable set for Multimeter  : http://s.click.aliexpress.com/e/s7EqNMre

Software:
- Arduino Sketch for ESP and HTML Code:
https://github.com/DIYDave/IoT-Rainwater-Tank


Services used:
- Weather:           https://openweathermap.org/
- Thingspeak:  https://thingspeak.com/
- Push Messages (nur iOS):    https://www.prowlapp.com/

Other DIY projects from me:
https://www.youtube.com/playlist?list=PLMaFfEa45zsyNV_LPKk5LShPhTd0iBbzv
