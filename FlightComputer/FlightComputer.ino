#include <SD.h>
#include <Servo.h>
#include <SPI.h>

#define MAX_PACKET_SIZE 16

File myFile;
long millisNow = 0;
long millisLast = 0;
int SOFTKILLDECREMENT = 10;

Servo AV1;//AV1-M
Servo AV2;//AV2-M
Servo AV3;//AV3-M
Servo AV4;//AV4-M

int defaultValue[] = {
  1000,1000,1000,1000};
int currentValue[] = {
  1000,1000,1000,1000};
int checkValue[] = {
  1000,1000,1000,1000};
int abortValue[] = {
  2000,2000,2000,2000};
int fuelDumpValue[] = {
  1500,1500,1500,1500};
int servoValueRead[] = {
  0,0,0,0};
int softKill[] = {
  1000,1000,1000,1000};

int MAX_MOTORS = 4;
int MAX_PWM = 4;
long int filePos = 0;
long loopTime = 0;
long loopTimeCounter = 0;
long MICRO_STEPS = 100;

boolean profileCheck = false;
boolean start = false;
boolean softKillBool = false;
boolean abortBool = false;
boolean fuelDump = false;
int softKilledCount = 0;

int sendPacketCounter = 0;

void setup()
{
  Serial.begin(9600);//USB connection
  Serial2.begin(9600);//communication with HealthMonitoring Arduino
  AV1.attach(6);
  AV2.attach(5);
  AV3.attach(4);
  AV4.attach(3);

for (int j = 0; j < MAX_PWM; j++){//sets servos to initial values
  AV1.writeMicroseconds(defaultValue[0]);
  AV2.writeMicroseconds(defaultValue[1]);
  AV3.writeMicroseconds(defaultValue[2]);
  AV4.writeMicroseconds(defaultValue[3]);
}

  pinMode(53, OUTPUT);
  delay(100);
  if (!SD.begin(53)){
    Serial.println("initialization failed");
    start = false;
  }
  else {
    Serial.println("initialized");
  }

  //print out if file is unavailable
  if (!SD.exists("pwmcoord.txt")) {
    Serial.println("pwmcoord.txt is unavailable");
  }
}

void loop()
{
  //read Serial2 data to know if we need to abort.


  if (Serial2.available()){
    char inByte = Serial2.read();
    Serial.write(inByte);

    if (inByte == 'r'){//reset filePos to beginning
      filePos = 0;
      softKillBool = false;
      abortBool = false;
      profileCheck = false;
      start = false;
      //Serial.print("r");
      //Serial2.clearing();
    }
    if (inByte == 'a'){//abort
      start = false;
      softKillBool = false;
      abortBool = true;
      //Serial.print("a");
      //myFile.close();
      //Serial2.clearing();
    }
    if (inByte == 'o'){//begin
      start = true;
      profileCheck = false;
      //Serial.print("o");
      //Serial2.clearing();
    }
    if (inByte == 'k'){//softkill
      softKillBool = true;
      for (int j = 0; j < MAX_PWM; j++){
        softKill[j] = currentValue[j];//grabs the beginning values for softkill 
      } 
      start = false;
      //Serial.println("s"); 
      //Serial2.clearing();
    }
    if (inByte == 'p'){
      start = true;
      profileCheck = true;
      //Serial2.clearing();
    }
    if(inByte == 'b '){
      start = false;
      fuelDump = true;
    }
  }

  myFile = SD.open("PWMCOORD.TXT");

  //read file
  if (start){
    if (myFile){
      myFile.seek(filePos);
      for (int i = 0; i < MAX_MOTORS; i++){
        for (int j = 0; j < MAX_PWM; j++){
          if (myFile.available()){
            char c = myFile.read();
            servoValueRead[j] = c - '0';
            //Serial.print(servoValueRead[j]);
            filePos++;
          }
          else{
            start = false;
          }
        }
        //assign reads to four int
        currentValue[i] = (servoValueRead[0]*1000) + (servoValueRead[1]*100) + (servoValueRead[2]*10) + (servoValueRead[3]);
      }
    }
  }

  if(profileCheck){
    for(int j=0; j<MAX_PWM; j++){
      checkValue[j] = currentValue[j];
    }
  }

  if ((!start) || profileCheck){
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = defaultValue[j];
    }
  }

  if(fuelDump){
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = fuelDumpValue[j];
    }
  }
  
  if (softKillBool){//decreases the current servo values by SOFTKILLDECREMENT each loop until they reach 1000 (min position).
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = softKill[j] - SOFTKILLDECREMENT;
      softKill[j] = currentValue[j];
      if (currentValue[j] < 1000){
        currentValue[j] = 1000;
        softKilledCount++;
      }
      if(softKilledCount == 4){
        softKilledCount = 0;
        softKillBool = false;
        start = false;
      }
    }
  }

  if (abortBool) {
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = abortValue[j];
    }
  }

  String packetToSend = "m:";
  if(!profileCheck){
    packetToSend += String(currentValue[0]);
    packetToSend += String(currentValue[1]);
    packetToSend += String(currentValue[2]);
    packetToSend += String(currentValue[3]);
  }
  else{
    packetToSend += String(checkValue[0]);
    packetToSend += String(checkValue[1]);
    packetToSend += String(checkValue[2]);
    packetToSend += String(checkValue[3]);
  }

  if (start || profileCheck || softKillBool || abortBool) {//sends packet every 2 loops (200ms)
    //   Serial.println(packetToSend);
    if (sendPacketCounter == 1) {
      Serial2.println(packetToSend);
      sendPacketCounter = 0;
    }
    else {
      sendPacketCounter++;
    }
  }

  //make sure each loop is 100ms
  while (loopTime < loopTimeCounter){
    loopTime = millis();
  }

  loopTime = millis();
  loopTimeCounter = millis()+100;
  
  //output to motors
  AV1.writeMicroseconds(currentValue[0]);
  AV2.writeMicroseconds(currentValue[1]);
  AV3.writeMicroseconds(currentValue[2]);
  AV4.writeMicroseconds(currentValue[3]);

  myFile.close();
}
