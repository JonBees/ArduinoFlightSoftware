int curpacketnum = 0;
int nextpacketnum = 0;

long curTime;
long loopStartTime;
long loopEndTime;
boolean packetReceived = false;

void setup()  
{
  Serial.begin(9600);//Connection to XBee
  Serial2.begin(9600);//Connection to PacketSnoopwithoutRadio
}

void loop() // run over and over
{
  curTime = millis();
  loopStartTime = curTime;
  loopEndTime = curTime + 200;
  packetReceived = false;
  
 if (Serial.available()){
    String s = String(readBatch());
    Serial2.print(s);
    Serial.print(createHealthPacket(s));
  }
  else{
    createHealthPacket("fail");
  }
  /*while(curTime < loopEndTime){
    curTime = millis();
  }*/
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
  outgoingPacket += String(millis() - loopStartTime);
  outgoingPacket += String(";?");
  int packetLength = outgoingPacket.length()-1;
  outgoingPacket += String(packetLength);
  outgoingPacket += String("|");
  return outgoingPacket;
}

String readBatch(){
  /*long timeEval = millis();
  long time = timeEval + 200;*/
  curTime = millis();
  long readTime = curTime + 200;
  String packetString = "";
  while(curTime < readTime /*&& !packetReceived*/){
    curTime = millis();
    if(Serial.available()){
      char c = Serial.read();
    if(c != 'h' && c != ':'){
      packetString += String(c);
      }
      else if(c == 'h'){
        packetString += ("-->");
      }
      /*else if (c == '|'){
        packetString += String(c);
        packetReceived = true;
      }*/
    }
  }
  return packetString;
}

