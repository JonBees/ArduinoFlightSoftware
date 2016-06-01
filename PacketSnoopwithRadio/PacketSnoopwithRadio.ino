int curpacketnum = 0;
int nextpacketnum = 0;

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

String readBatch(){
  long timeEval = millis();
  long time = timeEval + 200;
  String packetString = "";
  while(timeEval < time /*&& Serial.available()*/){
    timeEval = millis();
    if(Serial.available()){
      char c = Serial.read();
      /*if(c == '|'){
        String curpacketstring = "";
        while(c != '-'){
          c = Serial.read();
          curpacketstring += c;
        }
        curpacketnum = curpacketstring.toInt();
        if(!(curpacketnum == nextpacketnum)){
          Serial2.println("PACKET LOST");
        }
        nextpacketnum = curpacketnum + 1;
      }*/
      /*else*/ if(c != 'h' && c != ':'){
      packetString += String(c);
      }
      else if(c == 'h'){
        packetString += (/*String(counter)+*/"-->");
        //counter++;
      }
    }
  }
  return packetString;
}

