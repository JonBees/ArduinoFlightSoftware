#include <SD.h>
#include <Servo.h>
#include <SPI.h>

#define MAX_PACKET_SIZE 16

File myFile;
long millisNow = 0;
long millisLast = 0;
int SOFTKILLDECREMENT = 10;

Servo servoFrontLeft;//AV5-1
Servo servoFrontRight;//AV5-2
Servo servoBackLeft;//AV5-3
Servo servoBackRight;//AV5-4

int defaultValue[] = {
  1000,1000,1000,1000};
int currentValue[] = {
  1000,1000,1000,1000};
int abortValue[] = {
  2000,2000,2000,2000};
int numbers[] = {
  0,0,0,0};
int softKill[] = {
  1000,1000,1000,1000};

int MAX_MOTORS = 4;
int MAX_PWM = 4;
long int filePos = 0;
long loopTime = 0;
long loopTimeCounter = 0;
long MICRO_STEPS = 100;

boolean check = false;
boolean start = false;
boolean failed = false;
boolean softKillBool = false;
boolean abortBool = false;

int sendPacketCounter = 0;

void setup()
{
  Serial.begin(9600);//USB connection
  Serial2.begin(9600);//communication with HealthMonitoring Arduino
  servoFrontLeft.attach(3);
  servoFrontRight.attach(4);
  servoBackLeft.attach(5);
  servoBackRight.attach(6);

for (int j = 0; j < MAX_PWM; j++){//sets servos to initial values
  servoFrontLeft.writeMicroseconds(defaultValue[0]);
  servoFrontRight.writeMicroseconds(defaultValue[1]);
  servoBackLeft.writeMicroseconds(defaultValue[2]);
  servoBackRight.writeMicroseconds(defaultValue[3]);
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
      check = false;
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
      check = false;
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
      check = true;
      //Serial2.clearing();

    }
  }

  myFile = SD.open("PWMCOORD.TXT");

  //read file
  if (start ){
    if (myFile){
      myFile.seek(filePos);
      for (int i = 0; i < MAX_MOTORS; i++){
        for (int j = 0; j < MAX_PWM; j++){
          if (myFile.available()){
            char c = myFile.read();
            numbers[j] = c - '0';
            //Serial.print(numbers[j]);
            filePos++;
          }
          else{
            failed = true;
            start = false;
          }
        }
        //assign reads to four int
        currentValue[i] = (numbers[0]*1000) + (numbers[1]*100) + (numbers[2]*10) + (numbers[3]);
      }
    }
  }

  if ((!start) || failed || check){
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = defaultValue[j];
    }
  }

  if (softKillBool){//decreases the current servo values by SOFTKILLDECREMENT each loop until they reach 1000 (min position).
    for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = softKill[j] - SOFTKILLDECREMENT;
      softKill[j] = currentValue[j];
      if (currentValue[j] < 1000){
        currentValue[j] = 1000;
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
  packetToSend += String(currentValue[0]);
  packetToSend += String(currentValue[1]);
  packetToSend += String(currentValue[2]);
  packetToSend += String(currentValue[3]);

  if (start || check || softkillBool) {//sends packet every 2 loops (200ms) if not aborting
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
  servoFrontLeft.writeMicroseconds(currentValue[0]);
  servoFrontRight.writeMicroseconds(currentValue[1]);
  servoBackLeft.writeMicroseconds(currentValue[2]);
  servoBackRight.writeMicroseconds(currentValue[3]);
  failed = false;

  myFile.close();
}