/*

  52 to CLK      | 13
  50 to MISO     | 12 
  51 to MOSI     | 11
  53 to CS       | 10

*/

#include <SPI.h>
#include <SD.h>

//create testFile object
File testFile;

void setup() {

  Serial.begin(9600);
  pinMode(53, OUTPUT);    //53  |  10

  //feedback regarding initlization. DO NOT REMOVE THIS it is required for some reason
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    return;
  }
  
  //create a file named a.txt
  testFile = SD.open("a.txt", FILE_WRITE);
  testFile.close();
  
  //checks whether a.txt eists
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
    
    if(SD.exists("a.txt")){
      Serial.print("Line "); Serial.print (i); Serial.println(" was written to SD card");  
      i++;
    }
    else{
      Serial.print("Read Failed for ");
      Serial.println(i);
      i++;
    }
    
  }
}
