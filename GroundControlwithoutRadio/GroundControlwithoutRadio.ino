#include <ctype.h>
#include <SPI.h>


struct flags
{
bool safety : 
  1;           // Doesn't respond to any commands when true
bool tank_iso_open : 
  1;    // Isolation valve
bool dump_valve_open : 
  1;  // Fuel dump valve
bool abort : 
  1;            // If true, abort
bool flight_computer_on : 
  1;  // True means on
bool flight_computer_reset : 
  1;  // On change, will reset computer
bool ullage_dump : 
  1;      // On true, dump ullage
bool pixhawk_enable : 
  1;  // If true, enable PixHawk
bool take_off : 
  1;        // Take off on true
bool soft_kill : 
  1;       // True starts soft kill procedure
bool ullage_main_tank : 
  1;  // I don't know
bool sddump : 
  1;
bool spike : 
  1; // If the craft spiked
};

struct health_packet
{
  int pressure_values[7];
  double temp_values[6];
  int voltage;
  unsigned int elapsed;
  flags state;
  String stateString;
  String incomingString;
};

health_packet current_health_packet;
String health_StateString = "";

String packetToString(health_packet& hp);
String labview_stateString = "";

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);

  pinMode(2, INPUT);
  pinMode(13, OUTPUT);
}

void loop() {

  //XBee Readin Packets 
  boolean packetReceived = false;
  long time = millis() + 220;
  long timeEval = millis();
  while (!packetReceived && (time >= timeEval) && (timeEval-time) < 200){
    timeEval = millis();
    char check = NULL;
    if (Serial2.available()){
      check = Serial2.read();
    }
    if (check == 'p'){
      String incomingString = "";
      while(check != '?' && (time >= timeEval) && (timeEval-time) < 200){
        timeEval = millis();
        if (Serial2.available()){
          incomingString +=check;
          check = Serial2.read();
        }
      }
      if (check == '?'){
        String checkSum = "";
        while(check != '|' && (time >= timeEval) && (timeEval-time) < 200){
          timeEval = millis();
          if (Serial2.available()){
            check = Serial2.read();
            if (check != '|'){
              checkSum +=check;
            }
          }
        }
        int checkSumInt = checkSum.toInt();

        if (checkSumInt==incomingString.length()) {
          health_StateString = incomingString;
          packetReceived = true;
        }
      }
      //while (Serial2.available() && (time >= timeEval)){
      // Serial2.read();
      // timeEval = millis();
      //}
    }
  }
  //labview

  if (!Serial.available()) {
    if (digitalRead(2) == LOW) {
      labview_stateString = "h:a;1|"; 
    }
    else {
      labview_stateString = ""; 
    }
  }
  else {

    packetReceived = false;
    //30
    time = millis() + 50;
    timeEval = millis();
    while ((Serial.available()) && (!packetReceived) && (time >= timeEval) && ((timeEval-time) < 1000)) {
      timeEval = millis();
      char check = NULL;
      if (Serial.available())
        check = Serial.read();
      if (check == 'h'){
        String incomingString = "";
        while(check != ';' && Serial.available() && (time >= timeEval)){
          timeEval = millis();
          incomingString +=check;
          check = Serial.read();
        }
        if (check == ';'){
          if (digitalRead(2) == LOW) {
            if (incomingString.indexOf('a') == -1){
              incomingString += String('a');
            }
            digitalWrite(13, HIGH);
          }
          incomingString +=check;
          labview_stateString = incomingString;
          int packetLength = labview_stateString.length()-3;
          labview_stateString += String(packetLength);
          labview_stateString += String("|");
        }
        packetReceived = true;
      }
    }

  }

  Serial2.print(labview_stateString);
  Serial.println(health_StateString);

  //Then send flag packet to health monitor
  //send new_state

}
