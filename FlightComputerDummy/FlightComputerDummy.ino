#include <SD.h>
#include <Servo.h>
#include <SPI.h>

#define MAX_PACKET_SIZE 16

File myFile;
long millisNow = 0;
long millisLast = 0;
int SOFTKILLDECREMENT = 10;

//analog pin assignments
#define ALTIMETER_ANALOG 0
#define sensor13 1
#define sensor12 2
#define sensor15 3
#define sensor14 4

Servo AV1;//AV1-M
Servo AV2;//AV2-M
Servo AV3;//AV3-M
Servo AV4;//AV4-M

int defaultValue[] = {
  1000, 1000, 1000, 1000
};
int currentValue[] = {
  1000, 1000, 1000, 1000
};
int checkValue[] = {
  1000, 1000, 1000, 1000
};
int abortValue[] = {
  1267, 1400, 1400, 1444
};
int fuelDumpValue[] = {
  1267, 1400, 1400, 1444
};
int airBleedValue[] = {
  1230, 1375, 1326, 1410
};
int servoValueRead[] = {
  0, 0, 0, 0
};
int softKill[] = {
  1000, 1000, 1000, 1000
};

int openValue[] = {
  2000, 2000, 2000, 2000
};

char profileString[] = "12671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444126714001341144412671400134114441267140013411444";
int psLength = 800;

int MAX_MOTORS = 4;
int MAX_PWM = 4;
long int filePos = 0;
long loopTime = 0;
long loopEndTime = 0;
long MICRO_STEPS = 100;

boolean profileCheck = false;
boolean start = false;
boolean fullOpen = false;
boolean softKillBool = false;
boolean abortBool = false;
boolean fuelDump = false;
int softKilledCount = 0;

boolean profiles[] = {
  false, false, false, false, false, false, false, false
};
String filenames[] = {
  "PWM1.TXT", "PWM2.TXT", "PWM3.TXT", "PWM4.TXT", "PWM5.TXT", "PWM6.TXT", "PWM7.TXT", "PWM8.TXT"
};
int profileSelection = 0;

int sendPacketCounter = 0;
long curTime;
long lastConnection;

void setup()
{
  Serial.begin(9600);//USB connection
  Serial2.begin(9600);//communication with HealthMonitoring Arduino
  AV1.attach(3);
  AV2.attach(4);
  AV3.attach(5);
  AV4.attach(6);

  //psLength = sizeof(profileString);

  for (int j = 0; j < MAX_PWM; j++) { //sets servos to initial values
    AV1.writeMicroseconds(defaultValue[0]);
    AV2.writeMicroseconds(defaultValue[1]);
    AV3.writeMicroseconds(defaultValue[2]);
    AV4.writeMicroseconds(defaultValue[3]);
  }

  pinMode(53, OUTPUT);
  delay(100);
  /*if (!SD.begin(53)) {
    Serial.println("initialization failed");
    start = false;
  }
  else {
    Serial.println("initialized");
  }

  //print out which files are available
  for (int i = 0; i < 8; i++) {
    if (SD.exists(filenames[i])) {
      Serial.print(filenames[i]);
      Serial.println(" available.");
      profiles[i] = true;
    }
  }*/
}

void loop()
{
  //read Serial2 data to know if we need to abort.

  if (Serial2.available()) {
    char inByte = Serial2.read();

    if(inByte != '-'){
     Serial.print("Command: ");
     Serial.println(inByte);
    }

    fullOpen = false;

    if (inByte == 'r') { //reset filePos to beginning
      filePos = 0;
      softKillBool = false;
      abortBool = false;
      profileCheck = false;
      start = false;
      fuelDump = false;
      //Serial.print("r");
    }
    if (inByte == 'a') { //abort
      start = false;
      softKillBool = false;
      abortBool = true;
      //Serial.print("a");
      //myFile.close();
      //Serial2.clearing();
    }

    if (inByte == '0') {
      //filePos = 0;
      profileSelection = 0;
      Serial.print("not operating");
    }
    if (inByte == '1' && profiles[0]) {
      //filePos = 0;
      profileSelection = 1;
      Serial.print("opening 1");
    }
    if (inByte == '2' && profiles[1]) {
      //filePos = 0;
      profileSelection = 2;
      Serial.print("opening 2");
    }
    if (inByte == '3' && profiles[2]) {
      //filePos = 0;
      profileSelection = 3;
      Serial.print("opening 3");
    }
    if (inByte == '4' && profiles[3]) {
      //filePos = 0;
      profileSelection = 4;
      Serial.print("opening 4");
    }
    if (inByte == '5' && profiles[4]) {
      //filePos = 0;
      profileSelection = 5;
      Serial.print("opening 5");
    }
    if (inByte == '6' && profiles[5]) {
      //filePos = 0;
      profileSelection = 6;
      Serial.print("opening 6");
    }
    if (inByte == '7' && profiles[6]) {
      //filePos = 0;
      profileSelection = 7;
      Serial.print("opening 7");
    }
    if (inByte == '8' && profiles[7]) {
      ///filePos = 0;
      profileSelection = 8;
      Serial.print("opening 8");
    }
    if (inByte == '9') {
      //filePos = 0;
      profileSelection = 9;
      Serial.print("opening fully");
    }

    if (inByte == 'o') { //begin
      start = true;
      profileCheck = false;
      //Serial.print("o");
    }
    if (inByte == 'k') { //softkill
      softKillBool = true;
      for (int j = 0; j < MAX_PWM; j++) {
        softKill[j] = currentValue[j];//grabs the beginning values for softkill
      }
      start = false;
      //Serial.println("s");
      //Serial2.clearing();
    }
    if (inByte == 'p') {
      start = true;
      profileCheck = true;
      //Serial2.clearing();
    }
    if (inByte == 'b') {
      start = false;
      fuelDump = true;
    }

    /*if (inByte == '-') {
      Serial.println("-");
    }*/
    lastConnection = millis();
  }

  curTime = millis();

  if (curTime - lastConnection > 30000) {
    Serial.println("Time Abort");
    start = false;
    softKillBool = false;
    abortBool = true;
  }

  if (profileSelection == 0) {
    start = false;
  }
  else if (profileSelection > 0 && profileSelection < 9) {
    //myFile = SD.open(filenames[profileSelection - 1]);
  }
  else if (profileSelection == 9) {
    fullOpen = true;
  }


  //read file
  if (start) {
    if (filePos < psLength) {
      //myFile.seek(filePos);
      for (int i = 0; i < MAX_MOTORS; i++) {
        for (int j = 0; j < MAX_PWM; j++) {
          //if (myFile.available()) {
            char c = profileString[filePos];/*myFile.read()*/;
            //servoValueRead[j] = c - '0';
            Serial.print(servoValueRead[j]);
            filePos++;
          /*}
          else {
            start = false;
          }*/
        }
        //assign reads to four int
        currentValue[i] = (servoValueRead[0] * 1000) + (servoValueRead[1] * 100) + (servoValueRead[2] * 10) + (servoValueRead[3]);
      }
    }
  }

  if (fullOpen) {
    Serial.println("Full Open");
    for (int j = 0; j < MAX_PWM; j++) {
      currentValue[j] = openValue[j];
    }
  }

  if (profileCheck) {
    Serial.println("Profile Check");
    for (int j = 0; j < MAX_PWM; j++) {
      checkValue[j] = currentValue[j];
    }
  }

  if ((!start) || profileCheck) {
    for (int j = 0; j < MAX_PWM; j++) {
      currentValue[j] = defaultValue[j];
    }
  }

  if (fuelDump) {
    Serial.println("Fuel Dump");
    for (int j = 0; j < MAX_PWM; j++) {
      currentValue[j] = fuelDumpValue[j];
    }
  }

  if (softKillBool) { //decreases the current servo values by SOFTKILLDECREMENT each loop until they reach 1000 (min position).
    Serial.println("Softkill");
    for (int j = 0; j < MAX_PWM; j++) {
      currentValue[j] = softKill[j] - SOFTKILLDECREMENT;
      softKill[j] = currentValue[j];
      if (currentValue[j] < 1000) {
        currentValue[j] = 1000;
        softKilledCount++;
      }
      if (softKilledCount == 4) {
        softKilledCount = 0;
        softKillBool = false;
        start = false;
      }
    }
  }

  if (abortBool) {
    //Serial.println("Abort");
    for (int j = 0; j < MAX_PWM; j++) {
      currentValue[j] = abortValue[j];
    }
  }

  String packetToSend = "m:";
  if (!profileCheck) {
    packetToSend += String(currentValue[0]);
    packetToSend += String(currentValue[1]);
    packetToSend += String(currentValue[2]);
    packetToSend += String(currentValue[3]);
  }
  else {
    packetToSend += String(checkValue[0]);
    packetToSend += String(checkValue[1]);
    packetToSend += String(checkValue[2]);
    packetToSend += String(checkValue[3]);
  }



  //make sure each loop is 100ms
  while (loopTime < loopEndTime) {
    loopTime = millis();
  }

  //if (start || profileCheck || softKillBool || abortBool) {//sends packet every 2 loops (200ms)
  if (sendPacketCounter == 3) {
    Serial2.println(packetToSend);
    Serial.println(packetToSend);
    Serial.println(filePos);
    sendPacketCounter = 0;
  }
  else {
    sendPacketCounter++;
  }
  //  }

  loopTime = millis();
  loopEndTime = millis() + 100;

  //output to motors
  AV1.writeMicroseconds(currentValue[0]);
  AV2.writeMicroseconds(currentValue[1]);
  AV3.writeMicroseconds(currentValue[2]);
  AV4.writeMicroseconds(currentValue[3]);

  //myFile.close();
}
