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
}
