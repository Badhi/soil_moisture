#include <EEPROM.h>

#define HOLD_COUNT 100

#define COM

//#define TEST

#ifdef COM
#define SEND(message) \
{\
  Serial.print(message); \
}
#else
#define SEND(message)
#endif 

#define FSEND(...)\
{\
  sprintf(printBuff, __VA_ARGS__);\
  SEND(printBuff)\
}

const unsigned long long LED_BLINK_PERIOD = 5;
const unsigned long long  FUNC_LED_BLINK_PERIOD = 5;

const int motor0 = 9;
const int fan0 = 8;
bool motor0High = false;
bool fan0High = false;

const int pushButton = 12;
const int ledPort = 13;

const int soilInputPins[] = {A0, A1, A2, A3};
const int refOffsets [] = { 0, 300, 0, 0};

const int EEPROM_SOIL_MOISTURE_LOW_ADDR[] = {0, 1, 2, 3};
const int EEPROM_SOIL_MOISTURE_HIGH_ADDR[] = { 4, 5, 6, 7};

int soilMoistureLowVal [] = {341, 341, 341, 341};
int soilMoistureHighVal [] = {587, 587, 587, 587};

const int inputCount = sizeof(soilInputPins)/sizeof(int);

char printBuff[100] ;

int soilMoisture0_pumpThreshold = 60;
int soilMoisture0_fanThreshold = 40;

inline int readInputs()
{
  int values = 0;
  
  for(int i = 0; i < inputCount; ++i)
  {
    int analogVal = analogRead(soilInputPins[i]);
    int val = 100 * ((double) ( analogVal - soilMoistureLowVal[i])/(soilMoistureHighVal[i] - soilMoistureLowVal[i]));
    FSEND("A%d : %d ", i, val); 
    values += val;
  }
  FSEND("avg : %d", values/inputCount);
  FSEND("\n");
  return values/inputCount;
}

#ifndef TEST
inline bool waterThresholdReached(float value){
  FSEND("Water : Ratio : %d, thresh : %d\n", (int) value, (int) (soilMoisture0_pumpThreshold));
  return value > soilMoisture0_pumpThreshold ;
}
#else
inline bool waterThresholdReached(float value){
  return (value > 1000) ? true : false; 
}
#endif

inline bool fanThresholdReached(float value){
  //int ratio = 100 * ((value - soilMoistureLowVal[inputId])/(soilMoistureHighVal[inputId] - soilMoistureLowVal[inputId]));
  //FSEND("Fan : Ratio : %d, thresh : %d\n", (int) ratio, (int) (soilMoisture0_fanThreshold));
  return value < soilMoisture0_fanThreshold ;
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

void getValueOnPressButton(int pButton, const int* readPin, int pinC, int* outVals)
{
  long holdTime = 0;
  
  bool highDetected = false;
  while(true){
    int val = digitalRead(pButton);
    holdTime = (val > 0) ? val : ( (holdTime > 0) ? -1 * val : 0);

    if(holdTime > 0) highDetected = true;
    
    if(val){
      for(int i =0; i < pinC; i++)
      {
        outVals[i] = analogRead(readPin[i]);
      }
    }

    if(highDetected && holdTime == 0) break;
  }
}

void blinkFastTwice()
{
  digitalWrite(ledPort, HIGH);
  delay(500);
  digitalWrite(ledPort, LOW);
  delay(1000);
  digitalWrite(ledPort, HIGH);
  delay(500);
  digitalWrite(ledPort, LOW);
  delay(1000);
}

void calibrateMoistureSensor()
{
  blinkFastTwice();
  
  SEND("Calibrating Moisture Sensor\n");

  digitalWrite(ledPort, HIGH);
  FSEND("Put all sensor to the lowest moisture level and press button\n");
  getValueOnPressButton(pushButton, soilInputPins, inputCount, soilMoistureHighVal);
  digitalWrite(ledPort, LOW);
  
  for(int s = 0; s < inputCount; ++s)
  {
    FSEND("Calibration done, %d : Low = %d\n", s, soilMoistureLowVal[s]);
    SEND("Updating the ROM\n");
    writeEEPROM(EEPROM_SOIL_MOISTURE_LOW_ADDR[s], soilMoistureLowVal[s]);
  }
  
  delay(500);
  digitalWrite(ledPort, HIGH);
  FSEND("Put the sensors to the highest moisture and press button\n");
  getValueOnPressButton(pushButton, soilInputPins, inputCount, soilMoistureLowVal);
  digitalWrite(ledPort, LOW);

  for(int s = 0; s < inputCount; ++s)
  {
    FSEND("Calibration done, %d :  High = %d\n", s, soilMoistureHighVal[s]);
    SEND("Updating the ROM\n");
    writeEEPROM(EEPROM_SOIL_MOISTURE_HIGH_ADDR[s], soilMoistureHighVal[s]);
  }
  
  
  SEND("Press button to proceed");
  unsigned long long ledBlinkCounter = 0;
  
  digitalWrite(ledPort, HIGH);
  while(!checkPressed(pushButton))
  {
    ++ledBlinkCounter;
    if(ledBlinkCounter == LED_BLINK_PERIOD)
    {
      digitalWrite(ledPort, LOW);
    }
    else if(ledBlinkCounter == 2 * LED_BLINK_PERIOD)
    {
      digitalWrite(ledPort, HIGH);
      ledBlinkCounter = 0;
    }
  }

   digitalWrite(ledPort, LOW);
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
  digitalWrite(fan0, HIGH);
  
  pinMode(pushButton, INPUT);
  pinMode(ledPort, OUTPUT);

  for(int s = 0; s < inputCount; s++)
  {
    soilMoistureLowVal[s] = readEEPROM(EEPROM_SOIL_MOISTURE_LOW_ADDR[s]);
    soilMoistureHighVal[s] = readEEPROM(EEPROM_SOIL_MOISTURE_HIGH_ADDR[s]);
    FSEND("EEPROM Read Moisture High : %d, Moisture Low : %d\n", soilMoistureLowVal[s], soilMoistureHighVal[s]);
  }
  //SEND("Setup completed");
}

void checkForPump(int sensorValue){
  if(waterThresholdReached(sensorValue)){
     if(motor0High){
      motor0High = false;
      SEND("Setting the Motor 0 to low");
      digitalWrite(motor0, LOW);
    }
  }
  else{
    if(!motor0High){
      motor0High = true;
      SEND("Setting the Motor 0 to high");
      digitalWrite(motor0, HIGH);
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

bool  operationalLED(unsigned long long cVal )
{
  if(cVal == FUNC_LED_BLINK_PERIOD)
  {
    digitalWrite(ledPort, HIGH);
    return false;
  }
  else if ( cVal == 3*FUNC_LED_BLINK_PERIOD)
  { 
    digitalWrite(ledPort, LOW);
    return true;
  }
  return false;
}

int inputValue = 0;
unsigned long long cVal = 0;
void loop() // run over and over
{
  if(operationalLED(cVal)) { cVal = 0; } else { ++cVal; }
    
  if(checkPressed(pushButton)){
    SEND("Calibrating\n");
    calibrateMoistureSensor();
  }
    
  inputValue = readInputs();

  checkForPump(inputValue);
  //checkForFan(soilMoistureA0);
  
  delay(100);
}
