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
  1500,1500,1500,1500};
int fuelDumpValue[] = {
  1500,1500,1500,1500};
int servoValueRead[] = {
  0,0,0,0};
int softKill[] = {
  1000,1000,1000,1000};

int testValue[] = {
2000,2000,2000,2000};

char profileString[] = "1020102010201020104010401040104010601060106010601080108010801080110011001100110011201120112011201140114011401140116011601160116011801180118011801200120012001200122012201220122012401240124012401260126012601260128012801280128013001300130013001320132013201320134013401340134013601360136013601380138013801380140014001400140014201420142014201440144014401440146014601460146014801480148014801500150015001500152015201520152015401540154015401560156015601560158015801580158016001600160016001620162016201620164016401640164016601660166016601680168016801680170017001700170017201720172017201740174017401740176017601760176017801780178017801800180018001800182018201820182018401840184018401860186018601860188018801880188019001900190019001920192019201920194019401940194019601960196019601980198019801980200020002000200019801980198019801960196019601960194019401940194019201920192019201900190019001900188018801880188018601860186018601840184018401840182018201820182018001800180018001780178017801780176017601760176017401740174017401720172017201720170017001700170016801680168016801660166016601660164016401640164016201620162016201600160016001600158015801580158015601560156015601540154015401540152015201520152015001500150015001480148014801480146014601460146014401440144014401420142014201420140014001400140013801380138013801360136013601360134013401340134013201320132013201300130013001300128012801280128012601260126012601240124012401240122012201220122012001200120012001180118011801180116011601160116011401140114011401120112011201120110011001100110010801080108010801060106010601060104010401040104010201020102010201000100010001000";
int psLength = 0;

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
  
  psLength = sizeof(profileString);

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
    if(inByte == 'b'){
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
            Serial.print(servoValueRead[j]);
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

/*if (start){
      for (int i = 0; i < MAX_MOTORS; i++){
        for (int j = 0; j < MAX_PWM; j++){
          if (filePos < psLength){
            char c = profileString[filePos];
            servoValueRead[j] = c - '0';
            Serial.print(servoValueRead[j]);
            filePos++;
          }
          else{
            start = false;
          }
        }
        currentValue[i] = (servoValueRead[0]*1000) + (servoValueRead[1]*100) + (servoValueRead[2]*10) + (servoValueRead[3]);
      }
  }*/
  
  /*if(start){
     for (int j = 0; j < MAX_PWM; j++){
      currentValue[j] = testValue[j];
    }
  }*/

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

  

  //make sure each loop is 100ms
  while (loopTime < loopTimeCounter){
    loopTime = millis();
  }

//if (start || profileCheck || softKillBool || abortBool) {//sends packet every 2 loops (200ms)
    if (sendPacketCounter == 3) {
      Serial2.println(packetToSend);
      Serial.println(packetToSend);
      sendPacketCounter = 0;
    }
    else {
      sendPacketCounter++;
    }
//  }

  loopTime = millis();
  loopTimeCounter = millis()+100;
  
  //output to motors
  AV1.writeMicroseconds(currentValue[0]);
  AV2.writeMicroseconds(currentValue[1]);
  AV3.writeMicroseconds(currentValue[2]);
  AV4.writeMicroseconds(currentValue[3]);

  myFile.close();
}
