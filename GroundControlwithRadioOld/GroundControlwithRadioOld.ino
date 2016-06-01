long time = 0;
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial2.begin(115200);
  //Serial.flush();
  //Serial2.flush();
}

void loop() // run over and over
{
  if (Serial2.available())
    Serial.write(Serial2.read());
  if (Serial.available())
    Serial2.write(Serial.read());
  else{
    if (time > millis()+1000){
      Serial.clearing();
      Serial2.clearing();
      time = millis();
    }
  }
}
