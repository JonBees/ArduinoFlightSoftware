 void setup(){
  Serial.begin(9600);
  Serial2.begin(9600);
 }

void loop(){
  if(Serial2.available()){
    char c = char(Serial2.read());
    if(c != '|'){
      Serial.print(c);
    }
    else{
      Serial.println('|');
    }
  }
  //Serial.println(readFlags());
  //Serial.print("_\n");
}



String readFlags() 
{
  long timeEval = millis();
  long time = timeEval + 200;
  boolean packetReceived = false;
  String packetString = "";
  while (!packetReceived && (time >= timeEval)){
    timeEval = millis();
    char check = NULL;
    if (Serial2.available()){
      check = Serial2.read();
    }
    if (check == 'h'){
      check = Serial2.read();
      if (check == ':'){
        //here is our health packet
        String incomingString = "";
        while(check != ';' && Serial2.available() && (time >= timeEval)){
          timeEval = millis();
          check = Serial2.read();
          if(check != ';'){
            incomingString +=check;
          } 
        }
        String checkSum = "";
        while(check != '|' && (time >= timeEval)){
          timeEval = millis();
          if (Serial2.available()){
            check = Serial2.read();
            if (check != '|'){
              checkSum +=check;
            }
          }
        }
        int checkSumInt = checkSum.toInt();
        if (checkSumInt == incomingString.length()){
          packetString = String(incomingString + "^");
          packetReceived = true;
        }
        while (Serial2.available() && (time >= timeEval)){
          Serial2.read();
          timeEval = millis();
        }
      }
    }
  }
  return String(packetString);
}
