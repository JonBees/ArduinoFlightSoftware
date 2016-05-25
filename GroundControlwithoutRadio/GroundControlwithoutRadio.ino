#include <ctype.h>
#include <SPI.h>

String health_StateString = "";
String labview_stateString = "";

void setup() {
  Serial.begin(9600);//USB connection to PC with Labview
  Serial2.begin(9600);//Connection to GroundControlwithRadio

  pinMode(6, INPUT);
  pinMode(13, OUTPUT);
}

void loop() {

  //XBee Readin Packets 
  boolean packetReceived = false;
  long time = millis() + 220;
  long timeEval = millis();
  while (!packetReceived && (time >= timeEval) && (timeEval-time) < 200){
    timeEval = millis();
    char check = NULL;
    if (Serial2.available()){
      check = Serial2.read();
    }
    if (check == 'p'){
      String incomingString = "";
      while(check != '?' && (time >= timeEval) && (timeEval-time) < 200){
        timeEval = millis();
        if (Serial2.available()){
          incomingString +=check;
          check = Serial2.read();
        }
      }
      if (check == '?'){
        String checkSum = "";
        while(check != '|' && (time >= timeEval) && (timeEval-time) < 200){
          timeEval = millis();
          if (Serial2.available()){
            check = Serial2.read();
            if (check != '|'){
              checkSum +=check;
            }
          }
        }
        int checkSumInt = checkSum.toInt();

        if (checkSumInt==incomingString.length()) {
          health_StateString = incomingString;
          packetReceived = true;
        }
      }
    }
  }

  if (!Serial.available()) {
    if (digitalRead(6) == HIGH) {//hardkill button pressed
      labview_stateString = "h:a;1|"; 
      digitalWrite(13,HIGH);//LED on arduino
    }
    else {
      labview_stateString = ""; 
      digitalWrite(13,LOW);
    }
  }

  //Read packets from labview
  else {
    packetReceived = false;
    //30
    time = millis() + 50;
    timeEval = millis();
    while ((Serial.available()) && (!packetReceived) && (time >= timeEval) && ((timeEval-time) < 1000)) {
      timeEval = millis();
      char check = NULL;
      if (Serial.available())
        check = Serial.read();
      if (check == 'h'){
        String incomingString = "";
        while(check != ';' && Serial.available() && (time >= timeEval)){
          timeEval = millis();
          incomingString +=check;
          check = Serial.read();
        }
        if (check == ';'){
          if (digitalRead(6) == HIGH) {//hardkill button pressed
            if (incomingString.indexOf('a') == -1){
              incomingString += String('a');
            }
            digitalWrite(13, HIGH);//LED on arduino
          }
          else {
           digitalWrite(13,LOW); 
          }
          incomingString +=check;
          labview_stateString = incomingString;
          int packetLength = labview_stateString.length()-3;
          labview_stateString += String(packetLength);
          labview_stateString += String("|");
        }
        packetReceived = true;
      }
    }
  }
  Serial2.print(labview_stateString);
  Serial.println(health_StateString);
}