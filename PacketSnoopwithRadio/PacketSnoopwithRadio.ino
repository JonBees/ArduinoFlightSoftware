int counter = 0;

void setup()  
{
  Serial.begin(9600);//Connection to XBee
  Serial2.begin(9600);//Connection to PacketSnoopwithoutRadio
}

void loop() // run over and over
{

 if (Serial.available()){
    String s = String(readBatch());
    Serial2.print(s);
    Serial.print(createHealthPacket(s));
  }
  else{
    createHealthPacket("fail");
  }
  /*Serial.flush();
  Serial2.flush();*/
}


String createHealthPacket(String s)
{
  String outgoingPacket = "p:";
  for (int i = 0; i < 6; i++){
    outgoingPacket += String(1);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(1);
  outgoingPacket += String(";t:");
  for (int i = 0; i < 5; i++){
    outgoingPacket += String(1);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(1);
  outgoingPacket += String(";v:");
  outgoingPacket += String(1);
  outgoingPacket += String(";m:");
  for (int i = 0; i < 3; i++){
    outgoingPacket += String(1);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(1);
  outgoingPacket += String(";h:");
  outgoingPacket += String(s);
  outgoingPacket += String(";s:");
  outgoingPacket += String(200);
  outgoingPacket += String(";?");
  int packetLength = outgoingPacket.length()-1;
  outgoingPacket += String(packetLength);
  outgoingPacket += String("|");
  return outgoingPacket;
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
    if (Serial.available()){
      check = Serial.read();
    }
    if (check == 'h'){
      check = Serial.read();
      if (check == ':'){
        //here is our health packet
        String incomingString = "";
        while(check != ';' && Serial.available() && (time >= timeEval)){
          timeEval = millis();
          check = Serial.read();
          if(check != ';'){
            incomingString +=check;
          } 
        }
        String checkSum = "";
        while(check != '|' && (time >= timeEval)){
          timeEval = millis();
          if (Serial.available()){
            check = Serial.read();
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
        while (Serial.available() && (time >= timeEval)){
          Serial.read();
          timeEval = millis();
        }
      }
    }
  }
  return String(packetString);
}

String readBatch(){
  long timeEval = millis();
  long time = timeEval + 200;
  String packetString = "";
  while(timeEval < time /*&& Serial.available()*/){
    timeEval = millis();
    if(Serial.available()){
      char c = Serial.read();
      if(c != 'h' && c != ':'){
      packetString += String(c);
      }
      else if(c == 'h'){
        packetString += (String(counter)+"-->");
        counter++;
      }
    }
  }
  return packetString;
}

