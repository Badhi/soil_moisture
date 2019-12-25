#include <EEPROM.h>

#define HOLD_COUNT 100

//#define COM

#define EEPROM_SOIL_MOISTURE_LOW_ADDR 0
#define EEPROM_SOIL_MOISTURE_HIGH_ADDR 1
//#define TEST

#ifdef COM
#define SEND(message) \
{\
  Serial.println(message); \
}
#else
#define SEND(message)
#endif 

#define FSEND(...)\
{\
  sprintf(printBuff, __VA_ARGS__);\
  SEND(printBuff)\
}
const int motor0 = 8;
const int fan0 = 9;
bool motor0High = false;
bool fan0High = false;

const int pushButton = 13;

int soilMoistureA0 = 0;
char printBuff[100] ;

int soilMoistureLowVal = 341;
int soilMoistureHighVal = 587;

int soilMoisture0_pumpThreshold = 60;
int soilMoisture0_fanThreshold = 40;

#ifndef TEST
inline bool waterThresholdReached(float value){
  int ratio = 100 * ((value - soilMoistureLowVal)/(soilMoistureHighVal - soilMoistureLowVal));
  FSEND("Water : Ratio : %d, thresh : %d", (int) ratio, (int) (soilMoisture0_pumpThreshold));
  return ratio > soilMoisture0_pumpThreshold ;
}
#else
inline bool waterThresholdReached(float value){
  return (value > 550) ? true : false; 
}
#endif

inline bool fanThresholdReached(float value){
  int ratio = 100 * ((value - soilMoistureLowVal)/(soilMoistureHighVal - soilMoistureLowVal));
  FSEND("Fan : Ratio : %d, thresh : %d", (int) ratio, (int) (soilMoisture0_fanThreshold));
  return ratio < soilMoisture0_fanThreshold ;
}

inline int readEEPROM(int address32){
  int value = 0;
  for(int i = 0; i < 4; i++){
    value |= (EEPROM.read(address32 * 4 + i)) << (i * 8);
  }
  return value;
}
inline void writeEEPROM(int address32, int value){
  for(int i= 0; i < 4; i++){
    EEPROM.write(address32 * 4 + i, (value >> i * 8) & 0xff);
  }
}
bool checkPressed(const int pin)
{
  long pressedCount = 0;
  for(int i =0; i < HOLD_COUNT; i++)
  {
    pressedCount += digitalRead(pin);
    delay(1);
  }
  if(pressedCount > ((float) HOLD_COUNT * 2/3) ) return true;
  return false;
}

int getValueOnPressButton(int pButton, int readPin)
{
  int sensorVal = 0;
  long holdTime = 0;
  
  bool highDetected = false;
  while(true){
    int val = digitalRead(pButton);
    holdTime = (val > 0) ? val : ( (holdTime > 0) ? -1 * val : 0);

    if(holdTime > 0) highDetected = true;
    
    if(val){
      sensorVal = analogRead(readPin);
    }

    if(highDetected && holdTime == 0) break;
  }
  return sensorVal;
}

void calibrateMoistureSensor()
{
  delay(2000);
  SEND("Calibrating Moisture Sensor");

  SEND("Put the sensor to the highest moisture level and press button");
  soilMoistureHighVal = getValueOnPressButton(pushButton, A0);
  FSEND("Calibration done, High = %d", soilMoistureHighVal);
  
  SEND("Put the sensor to the lowest moisture and press button");
  soilMoistureLowVal = getValueOnPressButton(pushButton, A0);
  FSEND("Calibration done, Low = %d", soilMoistureLowVal);

  SEND("Updating the ROM");
  writeEEPROM(EEPROM_SOIL_MOISTURE_LOW_ADDR, soilMoistureLowVal);
  writeEEPROM(EEPROM_SOIL_MOISTURE_HIGH_ADDR, soilMoistureHighVal);

  SEND("Press button to proceed");
  while(!checkPressed(pushButton));
}

void setup()
{

#ifdef COM
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }
#endif
 
  pinMode(motor0, OUTPUT);
  pinMode(fan0, OUTPUT);
  digitalWrite(motor0, LOW);
  digitalWrite(fan0, LOW);
  
  pinMode(pushButton, INPUT);

  soilMoistureLowVal = readEEPROM(EEPROM_SOIL_MOISTURE_LOW_ADDR);
  soilMoistureHighVal = readEEPROM(EEPROM_SOIL_MOISTURE_HIGH_ADDR);

  FSEND("EEPROM Read Moisture High : %d, Moisture Low : %d", soilMoistureLowVal, soilMoistureHighVal);
  SEND("Setup completed");
}

void checkForPump(int sensorValue){
  if(waterThresholdReached(sensorValue)){
    if(!motor0High){
      motor0High = true;
      SEND("Setting the Motor 0 to high");
      digitalWrite(motor0, HIGH);
    }
  }
  else{
    if(motor0High){
      motor0High = false;
      SEND("Setting the Motor 0 to low");
      digitalWrite(motor0, LOW);
    }
  }  
}

void checkForFan(int sensorValue){
  if(fanThresholdReached(sensorValue)){
    if(!motor0High){
      fan0High = true;
      SEND("Setting the Fan 0 to high");
      digitalWrite(fan0, HIGH);
    }
  }
  else{
    if(motor0High){
      fan0High = false;
      SEND("Setting the Fan 0 to low");
      digitalWrite(fan0, LOW);
    }
  }    
}

void loop() // run over and over
{
#ifdef COM
  if(checkPressed(pushButton)){
    SEND("Calibrating");
    calibrateMoistureSensor();
  }
#endif
    
  soilMoistureA0 = analogRead(A0);

  FSEND("Moisture Read Value on A0 : %d", soilMoistureA0);
  
  checkForPump(soilMoistureA0);
  checkForFan(soilMoistureA0);
  
  delay(100);
}
