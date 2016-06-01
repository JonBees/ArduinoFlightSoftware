#include <ctype.h>
#include <SPI.h>


struct flags
{
  bool safety : 1;           // Doesn't respond to any commands when true
  bool tank_iso_open : 1;    // Isolation valve
  bool dump_valve_open : 1;  // Fuel dump valve
  bool abort : 1;            // If true, abort
  bool flight_computer_on : 1;  // True means on
  bool flight_computer_reset : 1;  // On change, will reset computer
  bool ullage_dump : 1;      // On true, dump ullage
  bool pixhawk_enable : 1;  // If true, enable PixHawk
  bool take_off : 1;        // Take off on true
  bool soft_kill : 1;       // True starts soft kill procedure
  bool ullage_main_tank : 1;  // I don't know
  bool sddump : 1;
  bool spike : 1; // If the craft spiked
};

struct health_packet
{
  double pressure_values[7];
  double temp_values[6];
  double voltage;
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
}

void loop() {
  
  //XBee Readin Packets 
  unsigned int time = millis();
  while (Serial2.available()){// && (time+50) >= millis()){
    char check = Serial2.read();
    if (check == 'p'){
      String incomingString = "";
      while(check != '?'){
        if (Serial2.available()){
          incomingString +=check;
          check = Serial2.read();
        }
      }
      if (check == '?'){
        health_StateString = incomingString;
        Serial2.flush();
      }
      //Serial2.flush();
    }
  }
  //labview
  time = millis();/*
  while (Serial.available() && (time+20) >= millis()){
    char check = Serial.read();
    if (check == 'h'){
      String incomingString = "";
      while(check != ';' && Serial.available()){
        incomingString +=check;
        check = Serial.read();
      }
      incomingString +=check;
      labview_stateString = incomingString;
      //Serial.flush();
    }
  }
  */
  
  Serial2.println(labview_stateString);
  Serial.println(health_StateString);
  delay(100);
  
  //Then send flag packet to health monitor
  //send new_state
  
}
