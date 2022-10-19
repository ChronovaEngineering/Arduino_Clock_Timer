/*
 * Mega Clock Timer
 * by Hazel Mitchell
 * CC BY-NC-SA 4.0 https://creativecommons.org/licenses/by-nc-sa/4.0/
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <BME280_MOD-1022.h>
#include "SHTSensor.h"

//Define hardware connections
#define TFT_CS          9
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC          8
#define SS_SD_CARD      4
#define SS_ETHERNET     10

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
SHTSensor sht;

//Variables setup
char filename[16]; // make it long enough to hold your longest file name, plus a null terminator

//Timing settings
volatile unsigned long nowTime = 0;
volatile unsigned long lastDisplay = 0;
volatile unsigned long lastThingspeak = 0;
volatile unsigned long lastSdsave = 0;
volatile unsigned long lastClear = 0;
volatile byte GPSstate = 0;

const unsigned long displayPeriod = 5000; //Time between LCD display refreshes in milliseconds
const unsigned long thingspeakPeriod = 15000; //Time between Thingspeak uploads in milliseconds
const unsigned long sdsavePeriod = 15000; //Time between SD saves in milliseconds
const unsigned long clearPeriod = 10000; //Time between resetting datastring to zero in milliseconds

//Data variables
char Nano_String[100];
volatile float bme_temp = 0;
volatile float bme_hum = 0;
volatile float bme_pres = 0;
String phaseOffset = Nano_String;
String penPass = Nano_String;
String temperature = String(bme_temp, DEC);
String humidity = String(bme_hum, DEC);
String pressure = String(bme_pres, DEC); 
String dataToSend = "";

// ThingSpeak Settings
byte mac[] = { EDIT ME }; // Add your MAC address here - it must be unique on the local network
byte server[]  = { 184, 106, 153, 149 }; // ThingSpeak IP Address: 184.106.153.149
byte usingSD = 0;
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "EDIT ME"; //Add your API key here
const int updateThingSpeakInterval = 5 * 1000;      // Time interval in milliseconds to update ThingSpeak (number of seconds * 1000 = interval)
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;
boolean ThingSpeakOn = true; //TODO

EthernetClient client; // Initialize Arduino Ethernet Client


void setup()
{
  pinMode(SS_SD_CARD, OUTPUT);
  pinMode(SS_ETHERNET, OUTPUT);
  digitalWrite(SS_SD_CARD, HIGH);  // SD Card not active
  digitalWrite(SS_ETHERNET, HIGH); // Ethernet not active
  
  //Set up comms
  Serial.begin(9600);     //Serial to USB port
  Serial1.begin(9600);    //Serial to Nano
  Wire.begin();           

  //Initialise display
  tft.init(240, 240);     // Init ST7789 240x240
  PrintWelcome();
  delay(100);

  //Connect to ThingSpeak
  digitalWrite(SS_ETHERNET, LOW);  // Ethernet ACTIVE
  PrintConnecting();
  startEthernet();      //Start Ethernet on Arduino
  digitalWrite(SS_ETHERNET, HIGH); // Ethernet not active
  
  //Initialise SD card
  digitalWrite(SS_SD_CARD, LOW);  // SD Card ACTIVE
  Serial.print("Initialising SD card...");
  if (!SD.begin(SS_SD_CARD)) {
    Serial.println("SD initialisation failed.");
  }
  else {
    usingSD = 1;
    Serial.println("initialisation done.");
  }
  digitalWrite(SS_SD_CARD, HIGH); // SD Card not active
  
  //BME Setup
  BME280.readCompensationParams();
  BME280.writeStandbyTime(tsb_0p5ms);         // tsb = 0.5ms
  BME280.writeFilterCoefficient(fc_16);       // IIR Filter coefficient 16
  BME280.writeOversamplingPressure(os16x);    // pressure x16
  BME280.writeOversamplingTemperature(os2x);  // temperature x2
  BME280.writeOversamplingHumidity(os1x);     // humidity x1
  BME280.writeMode(smNormal);

  //SHT Setup
  if (sht.init()) {
      Serial.print("SHT init(): success\n");
  } else {
      Serial.print("SHT init(): failed\n");
  }
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x

}

void loop() {
  nowTime = millis();
  
  //-----Receive phase offset from Nano-----//
  int i=0;
  if (Serial1.available()) {
    delay(100); //allows all serial sent to be received together
    while(Serial1.available() && i<100) {
      Nano_String[i++] = Serial1.read();
    }
    Nano_String[i++]='\0';
  }
  if(i>0) {
    //Get BME280 values
    BME280.readMeasurements();
    BMECompensatedMeasurements();
    
    // Prepare datastring
    phaseOffset = getValue(Nano_String, 'v', 0);
    penPass = getValue(Nano_String, 'v', 1);

    sht.readSample();
    
    temperature = String(sht.getTemperature(), DEC);
    humidity = String(sht.getHumidity(), DEC);
    //temperature = String(bme_temp, DEC);
    //humidity = String(bme_hum, DEC);
    pressure = String(bme_pres, DEC); 
    
    dataToSend = phaseOffset + ", " + temperature + ", " + humidity + ", " + pressure + ", " + penPass;
    Serial.println("dataToSend: ");
    Serial.println(dataToSend);
    
    GPSstate = 1;
    lastClear = millis();
  }

  //-----Update display at set intervals-----//
  if ((millis()-lastDisplay) > displayPeriod){
    Serial.println("Updating display");
    if (GPSstate == 0){
      PrintLostGPS();
    }
    else {
      PrintData();
    }
    lastDisplay = millis();
  }

  //-----Upload to ThingSpeak at set intervals-----//
  if ((millis()-lastThingspeak) > thingspeakPeriod){
    digitalWrite(SS_ETHERNET, LOW);  // Ethernet ACTIVE
    Serial.println("Uploading to ThingSpeak");
    thingSpeak();
    lastThingspeak = millis();
    digitalWrite(SS_ETHERNET, HIGH); // Ethernet not active

  //-----Upload to SD card at set intervals-----//
    digitalWrite(SS_SD_CARD, LOW);  // SD Card ACTIVE
    Serial.println("Uploading to SD card");
    //String dataToSend = "50000, 23.5, 108, 67";
    sdSave(dataToSend);
    lastSdsave = millis();
    digitalWrite(SS_SD_CARD, HIGH); // SD Card not active
  }

  //-----Clear data at set intervals-----//
  //Only activates if more than [clearPeriod] ms have passed since the last update from the Nano
  if ((millis()-lastClear) > clearPeriod){
    Serial.println("Resetting data to zeros");
    clearData();
    lastClear = millis();
    GPSstate = 0;
  }
}

void clearData(){
  //Nano_String[0] = '\0';
//  Nano_String[0] = '0';
//  Nano_String[1] = 'v';
//  Nano_String[2] = '0';
//  Nano_String[3]='\0';

  phaseOffset = "0";
  penPass = "0";

  BME280.readMeasurements();
  BMECompensatedMeasurements();
  sht.readSample();
  
  temperature = String(sht.getTemperature(), DEC);
  humidity = String(sht.getHumidity(), DEC);
  //temperature = String(bme_temp, DEC);
  //humidity = String(bme_hum, DEC);
  pressure = String(bme_pres, DEC); 
  
  dataToSend = phaseOffset + ", " + temperature + ", " + humidity + ", " + pressure + ", " + penPass;

  Serial.println("dataToSend: ");
  Serial.println(dataToSend);
  
//  bme_temp = 0;
//  bme_hum = 0;
//  bme_pres = 0; 
}

void sdSave(String dataString) {

  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();

    // print to the serial port too:
    Serial.println(dataString);
  }

  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
    PrintNoSD();
  }
  delay(1000);
}

void thingSpeak() {
  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    Serial.println("...disconnected");
    Serial.println();
    PrintNoDHCP();
    client.stop();
  }
  
  // Update ThingSpeak
//  if(!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval))
//  {
    Serial.println("Running ln226: updateThingSpeak()");
    updateThingSpeak("field1="+phaseOffset+"&field2="+temperature+"&field3="+humidity+"&field4="+pressure+"&field5="+penPass);
//  }
  
  // Check if Arduino Ethernet needs to be restarted
  if (usingSD == 0){
    if (failedCounter > 3 ) {startEthernet();}
    lastConnected = client.connected();
  } 
}

void updateThingSpeak(String tsData)
{
  if (client.connect(thingSpeakAddress, 80))
  {         
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    client.print(tsData);
    
    lastConnectionTime = millis();
    
    if (client.connected())
    {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      
      failedCounter = 0;
    }
    else
    {
      failedCounter++;
  
      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      Serial.println();
      PrintNoDHCP();
    }
    
  }
  else
  {
    failedCounter++;
    
    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();
    PrintNoDHCP();
    
    lastConnectionTime = millis(); 
  }
}

void startEthernet()
{
  
  client.stop();

  Serial.println("Connecting Arduino to network...");
  Serial.println();  

  delay(1000);
  
  // Connect to network amd obtain an IP address using DHCP
  if ((Ethernet.begin(mac)) == 0)
  {
    Serial.println("DHCP Failed, reset Arduino to try again");
    Serial.println();
    PrintNoDHCP();
  }
  else
  {
    Serial.println("Arduino connected to network using DHCP");
    Serial.println();
  }
  
  delay(1000);
}


void PrintData() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_GREEN); tft.setTextSize(3); tft.println(F("Phase Offset"));
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(4); tft.print(phaseOffset); tft.println(F(" us"));
  //tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(3); tft.println(F("Humidity"));
  //tft.setTextColor(ST77XX_WHITE); tft.setTextSize(4); tft.print(humidity); tft.println(F(" %"));
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(3); tft.println(F("Amplitude"));
//  float StripWidth = 8.02;
//  float PendLength = 1140;
//  float FpenPass = penPass.toFloat();
//  float Velocity = (StripWidth/1000)/(FpenPass/1000000);
//  float HeightDiff = (Velocity*Velocity)/(2*9.81);
//  float RadAngle = acos((PendLength/1000-HeightDiff)/(PendLength/1000));
//  float Amplitude = (RadAngle * 4068/71);
  float Amplitude = penPass.toFloat();
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(4); tft.print(Amplitude); tft.println(F(" deg"));
  tft.setTextColor(ST77XX_BLUE); tft.setTextSize(3); tft.println(F("Temperature"));
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(4); tft.print(temperature); tft.println(F(" degC"));
  tft.setTextColor(ST77XX_RED); tft.setTextSize(3); tft.println(F("Pressure"));
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(4); tft.print(bme_pres); tft.println(F(" mB"));
}

void PrintWaiting() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
//  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.println(F("Awaiting GPS"));
}

void PrintLostGPS() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.println("Lost GPS...");
}

void PrintNoDHCP() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.println("DHCP Fail...");
}

void PrintNoSD() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.println("SD card Fail...");
}

void PrintConnecting() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.println("Connecting to");
  tft.println("ThingSpeak...");
}

void PrintWelcome() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.println(F("Hazel and "));
  tft.println(F("Mike's clock"));
  tft.println(F("timer"));
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(3);
  tft.println("For world");
  tft.println("records only!");
}

void BMECompensatedMeasurements(void) {
  float temp, humidity,  pressure, pressureMoreAccurate;
  double tempMostAccurate, humidityMostAccurate, pressureMostAccurate;
  char buffer[80];

  temp      = BME280.getTemperature();
  humidity  = BME280.getHumidity();
  pressure  = BME280.getPressure();
  
  pressureMoreAccurate = BME280.getPressureMoreAccurate();  // t_fine already calculated from getTemperaure() above
  
  tempMostAccurate     = BME280.getTemperatureMostAccurate();
  humidityMostAccurate = BME280.getHumidityMostAccurate();
  pressureMostAccurate = BME280.getPressureMostAccurate();
  
  bme_temp = tempMostAccurate;
  bme_hum = humidityMostAccurate;
  bme_pres = pressureMostAccurate;
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
