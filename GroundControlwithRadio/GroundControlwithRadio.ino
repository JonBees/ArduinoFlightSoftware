long time = 0;
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);//USB connection to PC with LabView
  Serial1.begin(9600);//Connection to XBee
  Serial2.begin(9600);//Connection to GroundControlWithoutRadio
  //Serial.flush();
  //Serial2.flush();
}

void loop() // run over and over
{
  if (Serial2.available()){
    char c = Serial2.read();

    if (c == '~'){
      Serial1.clearing();
     
      Serial2.clearing();
    }
    else{
      Serial1.write(c);
      Serial.write(c);
    }
  }
  if (Serial1.available()){
    char d = Serial1.read();

    Serial2.write(d);
  }
}