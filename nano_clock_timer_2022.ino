/* 
 *  Nano Pendulum Clock Timer
 *  Reference clock from GPS
 *  Pendulum triggers an optoswitch at bottom dead centre
 *  DiyMore Nano requires Old Bootloader
 *  Hazel Mitchell & Mike Godfrey 21/10/21
 *  CC BY-NC-SA 4.0 https://creativecommons.org/licenses/by-nc-sa/4.0/
*/

#include <SPI.h>
#include <Wire.h>


//Define hardware connections
#define Pen_sw          3
#define PPS_pin         2


//Variables setup
volatile unsigned long Phase_Offset = 0;
volatile unsigned long Pen_Pass = 0;
volatile unsigned long Pen_Start = 0;
volatile unsigned long Pen_End = 0;
volatile unsigned long Timer_End = 0;

volatile byte GPS_state = LOW; 
volatile byte Pen_state = 0;

volatile byte mystatus = 0;
volatile byte penPasses = 0;
volatile byte timesincePPS = 1;
volatile byte timesincePen = 1;

volatile unsigned long lastPen = 0;
volatile unsigned long debounceTime = 800;
volatile unsigned long bounceTime = 0;


void setup() {
  pinMode(Pen_sw, INPUT);
  pinMode(PPS_pin, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(PPS_pin), PPS_trigger, RISING);
  attachInterrupt(digitalPinToInterrupt(Pen_sw), Pendulum_rise, RISING);
  attachInterrupt(digitalPinToInterrupt(Pen_sw), Pendulum_fall, FALLING);
  
  Serial.begin(9600);
  Wire.begin();

  delay(4000);
}


void loop() {
  tester();
}


//Triggers when the pendulum pass begins
void Pendulum_rise(){
  bounceTime = (millis()-lastPen);
  if (bounceTime > debounceTime){
    lastPen = millis();
    //Serial.println("Pendulum triggered");
    
    //Check Pendulum state 
    if (Pen_state == 1){
      Pen_state = 2;  //-> Pendulum is on second pass
      Pen_Start = micros(); //-> Record current time from micros in timer_start
    }
  
    else {
      Pen_state = 1; //-> Pendulum is on first pass
    }
  }
}

//Triggers when the pendulum pass ends
void Pendulum_fall(){
  if (Pen_state == 2){  //-> Pendulum is on second pass
    Pen_End = micros(); //-> Record current time from micros in Pen_End
  }
}

//Triggers every second on pulse from GPS
void PPS_trigger(){
  //Serial.println("GPS triggered");
  mystatus = 1;
  timesincePPS = 0;

  if (Pen_state == 2){ 
    Timer_End = micros();
    Phase_Offset = Timer_End - Pen_Start; //-> Record current time vs timer_start in microseconds
    Pen_Pass = Pen_End - Pen_Start; //-> Duration of pendulum pass
    noInterrupts();
    sendToMega();
    interrupts();
    Pen_state = 0;   //-> Reset states - i.e. Pendulum state = 0
  }
}



void sendToMega() {
  char buf[50];
  ltoa(Phase_Offset, buf, 10);
  Serial.write(buf); 
  Serial.write("v");
  ltoa(Pen_Pass, buf, 10);
  Serial.write(buf);
}


//Test code using simulated pendulum and pps triggers
void tester() {
  delay(50);
  
  Pendulum_rise();
  //Serial.println("Pendulum triggered");
  delay(1);
  Pendulum_fall();

  delay(950);

  PPS_trigger();
  //Serial.println("GPS triggered");
  
  //Serial.print("Phase offset = ");
  //Serial.println(Phase_Offset);
}
