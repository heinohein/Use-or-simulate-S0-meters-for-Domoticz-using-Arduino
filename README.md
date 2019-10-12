## **Project use and/or simulate S0 kWh meters for Domoticz**

This project uses an Arduino to interface between S0 kWh pulse meters and Domoticz. The Arduino accept as input 0, 1 or 
upto 12 S0 meters. 

The  output of the Arduino is designed to test offline from the real world the integration of S0 meters and the  
"Domoticz Open Source Home Automation System", im my situation running on a Raspberry PI 3B+. 

The Arduino scetch can easily be modified to "generate" your own S0 input pulses to test different kWh simulations
for your own target applications. See comments around DEBUG in the scetch.

When using/simulating upto 5 S0 meters the Arduino behaves like a S0PCM5 hardwarekit, which can be connected to Domoticz. 
The S0PCM5 uses an USB connector for hooking up to the PI, so use for this project an Arduino with an USB output or 
use a Serial-to-USB converter like the FTDI232


## **The following files are available:**

-  "Simulate_S0PCM5_Domoticz.ino"                   The Arduino scetch
-  "Output_example.txt"                             An example of the generated serial-output


## **Prerequisites:**

- This is an Arduino based project. You need an Arduino to run it. I run this project on a Arduino Pro Mini 5V/16Ghz 
  connected to an FTDI serial-to-USB interface, which is hooked up to or my PC or to an USB port of the Raspberry PI.
- You must be able to download the compiled sketch into your Arduino, therefore
- You must have installed a recent version of the Arduino IDE.


## **Installing:**

You must copy the map Simulate_S0PCM5_Domoticz and the file Simulate_S0PCM5_Domoticz.ino to compile this sketch.


## **Author:**

heinohein


**The sketch is based on the work of:**

Trystan Lea and "AWI" using their scetch "Arduino Pulse Counting Sketch for counting pulses from up to 12 
pulse output meters". See also Power/ Usage sensor - multi channel - local display on MySensors Forum
https://forum.mysensors.org

They are credited and thanked for their example code, which gave me ideas for this project
