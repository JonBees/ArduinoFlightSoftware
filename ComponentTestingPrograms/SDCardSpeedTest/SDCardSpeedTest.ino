/*
Fat 32 recomended format type for the card 

  52 to CLK      | 13
  50 to DO       | 12 
  51 to DI       | 11
  53 to CS       | 10

*/

#include <SPI.h>
#include <SD.h>

//create testFile object
File testFile;

void setup() {

  Serial.begin(9600);
  pinMode(53, OUTPUT);    //53  |  10

  //feed back regarding initlization. DO NOT REMOVE THIS it is reuired for some reason
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    return;
  }
  
  //create a file named testFile.txt
  testFile = SD.open("a.txt", FILE_WRITE);
  testFile.close();
  
  //feed back regarding the existance of a.txt
  if(SD.exists("a.txt")) {
    Serial.println("a.txt exists");
  }
  else {
    Serial.println("a.txt does not exist");
  }
  
}

int i = 0;

void loop() {
  
  while(i < 101) { 
  
    testFile = SD.open("a.txt", FILE_WRITE);
    testFile.println(millis());
    testFile.close();
  
    Serial.print("Line "); Serial.print (i); Serial.println(" was written to SD card");  
    i++;
  }
  
}
