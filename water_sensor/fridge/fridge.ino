//#define COM

#ifdef COM
#define SEND(message) {\
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
char printBuff[100] ;

typedef unsigned int uint;

#define ACTUATOR_COUNT 2

int fanPin[ACTUATOR_COUNT] = {4, 5};
int waterSensorPin[ACTUATOR_COUNT] = {8, 9};
bool fanOn[ACTUATOR_COUNT] = {true, true};

void enableFan(uint id){
  if(id >=  ACTUATOR_COUNT)
  {
    SEND("Invalid Fan id to enable");
    return;
  }
  if(!fanOn[id]){
    digitalWrite(fanPin[id], LOW);
    fanOn[id] = true;
  }
}

void disableFan(uint id){
  if(id >=  ACTUATOR_COUNT)
  {
    SEND("Invalid Fan id to disable");
    return;
  }  
  if(fanOn[id]){
    digitalWrite(fanPin[id], HIGH);
    fanOn[id] = false;
  }
}
void setup() {
#ifdef COM
  // initialize serial communication @ 9600 baud:
  Serial.begin(9600); 
#endif

  for(uint i = 0; i < ACTUATOR_COUNT; i++){
    pinMode(waterSensorPin[i], INPUT);   
  }

  for(uint i = 0; i < ACTUATOR_COUNT; i++){
    pinMode(fanPin[i], OUTPUT);
    disableFan(i);  
  }
}
void loop() {
  // read the sensor on analog A0:
  for(uint i = 0; i < ACTUATOR_COUNT; i++){
    int sensorReading  = digitalRead(waterSensorPin[i]);    
    if(sensorReading){ 
      FSEND("Water No Detected on %d", i);
      disableFan(i);
    }
    else {
      FSEND("Water Detected on %d", i);
      enableFan(i);
    }
  }
 
  delay(1000);  // delay between reads
}
