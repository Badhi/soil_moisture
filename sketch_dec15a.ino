#define HOLD_COUNT 100

#define SEND(message) \
{\
  Serial.println(message); \
}
const int motor0 = 7;
const int pushButton = 13;

int soilMoistureA0 = 0;
char printBuff[100] ;

int soilMoistureLowVal = 0;
int soilMoistureHighVal = 550;

float soilMoisture0_threshold = 0.5;

inline bool thresholdReached(float value){
  return ((value - soilMoistureLowVal)/(soilMoistureHighVal - soilMoistureLowVal)) > soilMoisture0_threshold;
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
  SEND("Put the sensor to the lowest moisture and press button");
 
  soilMoistureLowVal = getValueOnPressButton(pushButton, A0);
  sprintf(printBuff, "Calibration done, Low = %d", soilMoistureLowVal);
  SEND(printBuff);

  SEND("Put the sensor to the highest moisture level and press button");

  soilMoistureHighVal = getValueOnPressButton(pushButton, A0);

  sprintf(printBuff, "Calibration done, High = %d", soilMoistureHighVal);
  SEND(printBuff);

  SEND("Press button to proceed");
  while(!checkPressed(pushButton));
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }
  pinMode(motor0, OUTPUT);
  pinMode(pushButton, INPUT);
  SEND("Setup completed");
}

void loop() // run over and over
{
  if(checkPressed(pushButton)){
    SEND("Calibrating");
    calibrateMoistureSensor();
  }
    
  soilMoistureA0 = analogRead(A0);

  sprintf(printBuff, "Moisture Read Value on A0 : %d", soilMoistureA0);  
  SEND(printBuff);

  if(thresholdReached(soilMoistureA0)){
    SEND("Setting the Motor 0 to high");
    digitalWrite(motor0, HIGH);
  }
  else{
    SEND("Setting the Motor 0 to low");
    digitalWrite(motor0, LOW);
  }
  
   delay(100);
}
