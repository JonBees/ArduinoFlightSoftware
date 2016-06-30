#include <ctype.h>
#include <SPI.h>

String health_StateString = "";
String labview_stateString = "";
int count = 0;

int loopCount = 0;

void setup() {
  Serial.begin(9600);//USB connection to PC with Labview
  Serial1.begin(9600);//Connection to XBee
  pinMode(6, INPUT);
  pinMode(13, OUTPUT);//Abort LED (on Arduino)
  pinMode(12, OUTPUT);//Green craft communication LED
  pinMode(10,OUTPUT);//Yellow Labview communication LED
}

void loop() {
  digitalWrite(12, LOW);
  digitalWrite(10, LOW);

  if(Serial.available()){
    digitalWrite(10, HIGH);
    //Serial.print(Serial.available());
    }
  if(Serial1.available()){
    digitalWrite(12, HIGH);
    }

  //XBee Readin Packets 
  boolean packetReceived = false;
  long time = millis() + 220;
  long timeEval = millis();
  while (!packetReceived && (time >= timeEval) && (timeEval-time) < 200){
    timeEval = millis();
    char check = NULL;
    if (Serial1.available()){
      check = Serial1.read();
    }
    if (check == 'p'){
      String incomingString = "";
      while(check != '?' && (time >= timeEval) && (timeEval-time) < 200){
        timeEval = millis();
        if (Serial1.available()){
          incomingString +=check;
          check = Serial1.read();
        }
      }
      if (check == '?'){
        String checkSum = "";
        while(check != '|' && (time >= timeEval) && (timeEval-time) < 200){
          timeEval = millis();
          if (Serial1.available()){
            check = Serial1.read();
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
    while ((Serial.available()) && (!packetReceived) && (time >= timeEval) && ((timeEval-time) < 1000)){
      timeEval = millis();
      char check = NULL;
      if (Serial.available())
        check = Serial.read();
      if (check == 'h'){
        count++;
        Serial1.print(count);
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
  Serial1.print(labview_stateString);
  Serial.println(health_StateString);
  loopCount++;
}
