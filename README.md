# arduino-clock-timer

These files are used with the Arduino Clock Timer being developed by Hazel Mitchell and Mike Godfrey. 

The nano_clock_timer_2022.ino and mega_clock_timer_2022.ino are files to be uploaded to an Arduino Nano and Mega respectively. Note that the MAC address and writeAPIKey variables will need to be set by the user in the mega_clock_timer_2022.ino file.

All files are presented as-is, without any warranty. This project is licensed as CC BY-NC-SA 4.0 (https://creativecommons.org/licenses/by-nc-sa/4.0/). You may share and use this material, provided you give credit and share derivative material under the same license. You may not use this material for commercial applications.

# Usage advice
- Printing to the serial monitor from the Nano for debugging *is not* recommended. Serial.print() is a very slow command so printing long lines of characters can interfere with the interrupts and cause weird artefacts in the output.
- tester() in the Nano code can be used when the GPS and light gate are disconnected to simulate these pulses. It's very useful for testing everything on the Mega side of the system.
- Loss of either the GPS or light gate pulses will cause "GPS lost" to be displayed on the LCD. Disconnecting the Nano from the Mega will have the same effect. (GPS signal loss was the most common issue in testing, hence the error message phrasing.)
- Pendulum_fall() is only used in tester().

# Notes on operating principle
The Nano is detecting the following events:
1) A rising edge from the light gate, as the pendulum enters it. Set Pen_state to 1 so we know it has passed once.
2) A second rising edge from the light gate, as the second pass begins. Set Pen_state to 2 and start a pass duration timer (save the time to Pen_Start).
3) Use the loop to check when the pendulum has left the light gate and stop the pass duration timer (save the time to Pen_End).
4) Wait for the PPS pulse from the GPS signal.
5) Calculate the offset between the light gate pulse and the PPS signal, as well as the pass duration, and send it to the Mega.

# Pinout
Mega:

4 = SD?;

8 = LCD DC,

9 = LCD CS,

10-13 = Ethernet?,

18 = NANO RX,

19 = NANO TX,

20 = SDA,

21 = SCL,

22 = SW1,

24 = SW2,

26 = SW3,

28 = SW4,

51 = LCD MOSI,

52 = LCD SCK

Nano:

D2 = Pendulum trigger,

D3 = PPS,

TX = MEGA RX,

RX = MEGA TX

Pins marked ? might vary if you use a different Ethernet shield

# Libraries
BME280_MOD-1022.h - https://github.com/embeddedadventures/BME280
