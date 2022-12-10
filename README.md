# arduino-clock-timer

These files are used with the Arduino Clock Timer being developed by Hazel Mitchell and Mike Godfrey. 

The nano_clock_timer_2022.ino and mega_clock_timer_2022.ino are files to be uploaded to an Arduino Nano and Mega respectively. Note that the MAC address and writeAPIKey variables will need to be set by the user in the mega_clock_timer_2022.ino file.

All files are presented as-is, without any warranty. This project is licensed as CC BY-NC-SA 4.0 (https://creativecommons.org/licenses/by-nc-sa/4.0/). You may share and use this material, provided you give credit and share derivative material under the same license. You may not use this material for commercial applications.

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

# Libraries
BME280_MOD-1022.h - https://github.com/embeddedadventures/BME280
