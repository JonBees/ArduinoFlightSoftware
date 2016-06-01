int count = 0;
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);//USB connection to PC with LabView
  Serial1.begin(9600);//Connection to XBee
  Serial2.begin(9600);//Connection to GroundControlWithoutRadio
  digitalWrite(13,LOW);
}

void loop() // run over and over
{
  if(Serial2.available()){
    digitalWrite(13,HIGH);
    char c = Serial2.read();
    if(c == '~'){    }
    
    if (c == 'h'){
      count++;
      Serial1.print(count);
    }
      Serial1.write(c);
      Serial.write(c);
  }
  else{
      digitalWrite(13,LOW);
      //Serial2.begin(9600);
  }
  
  if(Serial1.available()){
    char d = Serial1.read();
    Serial2.write(d);
    digitalWrite(13,HIGH);
  }
  else{
    digitalWrite(13,LOW);
    //Serial1.begin(9600);
  }
}
