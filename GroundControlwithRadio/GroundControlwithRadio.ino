int count = 0;
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);//USB connection to PC with LabView
  Serial1.begin(9600);//Connection to XBee
  Serial2.begin(9600);//Connection to GroundControlWithoutRadio
}

void loop() // run over and over
{
  if(Serial2.available()){
    char c = Serial2.read();
    if (c == 'h'){
      count++;
      Serial1.print(count);
    }
      Serial1.write(c);
      Serial.write(c);
  }
  else{
      Serial2.begin(9600);
  }
  
  if(Serial1.available()){
    char d = Serial1.read();
    Serial2.write(d);
  }
  else{
    Serial1.begin(9600);
  }
}
