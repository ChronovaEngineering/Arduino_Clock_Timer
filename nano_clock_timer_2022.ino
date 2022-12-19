// Pendulum Clock Timer
// Reference clock from GPS
// Pendulum triggers an optoswitch at bottom dead centre
// DiyMore Nano requires Old Bootloader
// Hazel Mitchell & Mike Godfrey 19/12/2022

#include <SPI.h>
#include <Wire.h>


//Define hardware connections
#define Pen_sw          2
#define PPS_pin         3


//Variables setup
volatile long Phase_Offset = 0;
volatile long Pen_Pass = 0;
volatile unsigned long Pen_Start = 0;
volatile unsigned long Pen_End = 0;
volatile unsigned long Timer_End = 0;

volatile byte GPS_state = LOW; 
volatile byte Pen_state = 0;
volatile byte Pass_state = 0;

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
  
  Serial.begin(9600);
  Wire.begin();

  delay(500);
}


void loop() {
  //tester();
  if (Pass_state == 1){
    if (digitalRead(Pen_sw) == 0){
      Pen_End = micros();
      Pass_state = 0;
      //Serial.println("Pendulum falling");
    }
  }
}


//Triggers when the pendulum pass begins
void Pendulum_rise(){
  bounceTime = (millis()-lastPen);
  if (bounceTime > debounceTime){
    lastPen = millis();
    //Serial.println("Pendulum rising");
    
    //Check Pendulum state 
    if (Pen_state == 1){
      Pen_state = 2;  //-> Pendulum is on second pass
      Pen_Start = micros(); //-> Record current time from micros in timer_start
      
      Pass_state = 1; //-> Used to indicate that the pendulum is still passing 
    }
  
    else {
      Pen_state = 1; //-> Pendulum is on first pass
    }
  }
}

//Triggers when the pendulum pass ends
void Pendulum_fall(){
  bounceTime = (micros()-Pen_Start);
  if (bounceTime > 0){
    Serial.println("Pendulum falling");
    if (Pen_state == 2){  //-> Pendulum is on second pass
      Pen_End = micros(); //-> Record current time from micros in Pen_End
      detachInterrupt(digitalPinToInterrupt(Pen_sw));
      attachInterrupt(digitalPinToInterrupt(Pen_sw), Pendulum_rise, RISING);
    }
  }
}

//Triggers every second on pulse from GPS
void PPS_trigger(){
  mystatus = 1;
  timesincePPS = 0;

  if (Pen_state == 2){ 
    Timer_End = micros();
    
    Phase_Offset = Timer_End - Pen_Start; //-> Record current time vs timer_start in microseconds
    Pen_Pass = Pen_End - Pen_Start; //-> Duration of pendulum pass
    
    noInterrupts();
    sendToMega();
    //Serial.println();
    //Serial.println();
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
  delay(40);
  
  Pendulum_rise();
  //Serial.println("Pendulum triggered");
  delay(10);
  Pendulum_fall();

  delay(950);

  PPS_trigger();
  Serial.println("GPS triggered");
}
