
/*
Healt Mointering Sketch

Overall Inputs Acepted:
  Pressure Transducers - analog inputs pins 0 - 6
  Thermocouple - digital pints 9, 7, 4, 5, 6
  Power Realy - digital pin 8  
  
Overall Output:
  Xbee Radio - 
  Micro SD Card -
  
  (kpa?)
  Max Pressure - 300
  
  (Celcuis)
  Max Temperature = 200
  
  Ullage-iso = 13
  Tank-iso = 11
  Servo Dump Valve = 12
  Fail Closed Valve = 41
  Fail Open Valve = 40
  
  default_0 means no communication + not armed
  default_1 means no communication + armed
  
*/

//libraries
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <ctype.h>
#include <EEPROM.h>
#include <MAX31855.h>
#include <Servo.h>

//Pinouts
#define ULLAGE_ISO 13
#define TANK_ISO 12
#define DUMP_VALVE 11
#define FAIL_CLOSED 41
#define FAIL_OPEN 42

//Servo
Servo ullage_iso_servo;
Servo tank_iso_servo;
Servo dump_valve_servo;

//Servo Defaults
#define ullage_iso_servo_default_0 1000
#define tank_iso_servo_default_0   1000
#define dump_valve_servo_default_0 1000

#define ullage_iso_servo_default_1 2000
#define tank_iso_servo_default_1   2000
#define dump_valve_servo_default_1 2000


//SD Card
File recordingFile;

//Power Relay
#define power_relay_digital_on 8
#define power_relay_digital_off 13

//Pressure Transducer
#define pressure_transducer_one_analog 0
#define pressure_transducer_two_analog 1
#define pressure_transducer_three_analog 2
#define pressure_transducer_four_analog 3
#define pressure_transducer_five_analog 4
#define pressure_transducer_six_analog 5
#define pressure_transducer_seven_analog 6

//Max/Min Pressure Values
#define MAX_PRESSURE 300
#define MIN_PRESSURE 0

//Max/Min Temperature Values
#define MAX_TEMPERATURE 200
#define MIN_TEMPERATURE -20

//Voltage Sensor
#define voltage_sensor_analog 7


//Xbee input flags 
  struct flags
  {
    bool safety : 1;          // Doesn't respond to any commands when true
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
    bool spike : 1; // becomes true if pressure/temperature/voltage spike
    bool sddump : 1;
    flags() {
      safety = false;          // Doesn't respond to any commands when true
      tank_iso_open = false;    // Isolation valve
      dump_valve_open = false;  // Fuel dump valve
      abort = false;            // If true, abort
      flight_computer_on = false;  // True means on
      flight_computer_reset = false;  // On change, will reset computer
      ullage_dump = false;      // On true, dump ullage
      pixhawk_enable = false;  // If true, enable PixHawk
      take_off = false;        // Take off on true
      soft_kill = false;       // True starts soft kill procedure
      ullage_main_tank = false;  // I don't know
      spike = false; // becomes true if pressure/temperature/voltage spike
      sddump = false;
    };
  };
  struct craft
  {
    bool temperature : 1;//temp abort
    bool pressure : 1;//pressure abort
    bool voltage : 1;//voltage abort
    bool time : 1;//time abort
  };

// Data structure for XBee health packet
struct health_packet
{
  double pressure_values[7];
  double temp_values[6];
  double voltage;
  unsigned int motor_values[4];
  unsigned long elapsed;
  flags state;
  craft errorflags;
  String stateString;
};

//Thermocouple 

#define THERMOCOUPLE_CHIP_SELECT 9
MAX31855 TC(THERMOCOUPLE_CHIP_SELECT);

template <int N>
class print_err;

unsigned long lastFlagReadTime = 0;
unsigned long currentTime = 0;

health_packet current_health_packet;
String lastStateString = "";


void sendDataOverSerial2(char input);
void stateFunctionEvaluation(health_packet& data);
void resetState();
void stateEvaluation(health_packet& data);
void resetErrorFlags(health_packet& data);
void PressureTransducerRead(health_packet& data);
void voltage_sensor(health_packet& data);
void readThermocouples(health_packet& data);
void sendHealthPacket(health_packet& data);
void healthPacketToString(health_packet& data, String& str);


void setup() {
  Serial.begin(9600);
  resetState();//reset bool states
  //Motors/Valves init
  ullage_iso_servo.attach(ULLAGE_ISO);
  tank_iso_servo.attach(TANK_ISO);
  dump_valve_servo.attach(DUMP_VALVE);
  pinMode(power_relay_digital_on, OUTPUT);
  pinMode(power_relay_digital_off, OUTPUT);
  
  ullage_iso_servo.writeMicroseconds(ullage_iso_servo_default_0);
  tank_iso_servo.writeMicroseconds(tank_iso_servo_default_0);
  dump_valve_servo.writeMicroseconds(dump_valve_servo_default_0);
  digitalWrite(power_relay_digital_on,LOW);
  digitalWrite(power_relay_digital_off,HIGH);
  
  //SD Card
  //SD card initiation
  pinMode(53, OUTPUT);

  //SD card feed back, do not remove this feedback it is neccasry for function of SD card
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    return;
  }

  recordingFile = SD.open("FlightData.txt", FILE_WRITE);
  recordingFile.close();

  //more SD card feed back
  if(SD.exists("FlightData.txt")) {
    Serial.println("Flight Data file exist");
  }
  else {
    Serial.println("Flight Data file does not exist");
  } 
}


void loop() {
  // Initialize the health packet
  resetErrorFlags(current_health_packet);
  
  //read in xbee
  readFlags(); //read input
  currentTime = millis();
  //if no read
    //no read use last health_packet
  
    //if no read check time >300ms handshake process has failed. Turn on appropriate abort based on last health packet
    if (lastFlagReadTime < (currentTime - 300)){
      current_health_packet.errorflags.time = true;
    }
  //ThermoCouple check temp
  readThermocouples(current_health_packet);
    
  //Pressure Transducers check pressure
  PressureTransducerRead(current_health_packet);
      
  //Voltage Sensor check voltage
  voltage_sensor(current_health_packet);
  
  //Send Health Packer 
  sendHealthPacket(current_health_packet);
  
  //check current errorflags
  errorFlagsEvaluation();
  
  ////////////////////////////////////////////////////////////////////////
  if (current_health_packet.stateString.equals(lastStateString)){
      stateEvaluation(current_health_packet);
      stateFunctionEvaluation(current_health_packet);
  }
}

///////////////////////////////////////////////////
///////////////Fucntions///////////////////////////
///////////////////////////////////////////////////

//check current error flags
void errorFlagsEvaluation(health_packet& data){
  if (current_health_packet.errorflags.temperature){
    //first append craft abort
    addFlagToString(current_health_packet);
    //temperature abort
    if (current_health_packet.state.safety){
      //assuming safety is off
      //flight system is life
      data.state.soft_kill = true;
      if (data.stateString.indexOf('k') == -1){
          data.stateString += String('k');
      }
    }
    else{
      //craft is off
    }
  }
  if (current_health_packet.errorflags.pressure){
    //first append craft abort
    addFlagToString(current_health_packet);
    //pressure abort
    if (current_health_packet.state.safety){
      //assuming safety is off
      //flight system is life
      data.state.soft_kill = true;
      if (data.stateString.indexOf('k') == -1){
          data.stateString += String('k');
      }
    }
    else{
      //craft is off
    }
  }
  if (current_health_packet.errorflags.voltage){
    //first append craft abort
    addFlagToString(current_health_packet);
    //voltage abort
    if (current_health_packet.state.safety){
      //assuming safety is off
      //flight system is life
      data.state.soft_kill = true;
      if (data.stateString.indexOf('k') == -1){
          data.stateString += String('k');
      }
    }
    else{
      //craft is off
    }
  }
  if (current_health_packet.errorflags.time){
    //first append craft abort
    addFlagToString(current_health_packet);
    //timeout abort
    if (current_health_packet.state.safety){
      //assuming safety is off
      //flight system is life
      data.state.soft_kill = true;
      if (data.stateString.indexOf('k') == -1){
          data.stateString += String('k');
      }
    }
    else{
      //craft is off
    }
  }
}

//send char over serial2
void sendDataOverSerial2(char input){
  bool exit = true;
  while (exit){
    Serial2.write(input);
    delay(5);
    char temp = Serial2.read();
    if (temp == input){
      //exit
      exit = false;
    }
  }
}

//evaluates logic functions
void stateFunctionEvaluation(health_packet& data){
  
  //flight states
  /////////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.abort){
    //abort sequence
    sendDataOverSerial2('a');
  }
  else if (data.state.soft_kill){       
    // True starts soft kill procedure
    sendDataOverSerial2('k');
  }
  else if (data.state.sddump){
    Serial2.print('p');
    while(data.state.sddump){//you've entered the sddump
      //we are going to get all the data off the sd card
      char temp = ' ';
      if (Serial2.available()){
        temp = Serial2.read();
        Serial.write(temp);
      }
      if (temp == '!')
        data.state.sddump = false;
    } 
  }
  else if (data.state.take_off){        //ensures take_off is not enable unless no abort is in effect
    // Take off on true
    sendDataOverSerial2('o');
  }
  
  
  //Valve States
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.tank_iso_open){    
    // Isolation valve
    tank_iso_servo.writeMicroseconds(tank_iso_servo_default_1);
  }
  else{
    tank_iso_servo.writeMicroseconds(tank_iso_servo_default_0);
  }
  if (data.state.dump_valve_open){  
    // Fuel dump valve
    dump_valve_servo.writeMicroseconds(dump_valve_servo_default_1);
  }
  else{
    dump_valve_servo.writeMicroseconds(dump_valve_servo_default_0);
  }
  if (data.state.ullage_dump){      
    // On true, dump ullage
    digitalWrite(power_relay_digital_on,HIGH);
  }
  else{
    digitalWrite(power_relay_digital_on,HIGH);
  }
  if (data.state.ullage_main_tank){  
    // I don't know
    ullage_iso_servo.writeMicroseconds(ullage_iso_servo_default_1);
  }
  else{
    ullage_iso_servo.writeMicroseconds(ullage_iso_servo_default_0);
  }
  
  //Computer System Enable States
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_reset){  
    // On change, will reset computer
    sendDataOverSerial2('r');
    //also reset millis timer
    //*********ADD please***********//
  }
  if (data.state.pixhawk_enable){  
    // If true, enable PixHawk
    Serial3.write('o');
  }
  
  
  //on off various states
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_on){  
    // True means turn on the flight controller
    digitalWrite(power_relay_digital_off,LOW);
    digitalWrite(power_relay_digital_on,HIGH);
  }
  else{
    //make sure flight computer is off
    digitalWrite(power_relay_digital_on,LOW);
    digitalWrite(power_relay_digital_off,HIGH);
  }

}


//used to set all bools false
void resetState()
{
  flags tempFlag;
  current_health_packet.state = tempFlag;
}

//Reset error flags
void stateEvaluation(health_packet& data){
  String tempString = data.stateString;
  int stringLength = tempString.length();
  resetState();
  for (int i = 0; i < stringLength;i++){
    char ramp = tempString[i];
    switch(ramp){
      case 's':
        data.state.safety = true;
        break;
      case 't':
        data.state.tank_iso_open = true;
        break;
      case 'v':
        data.state.dump_valve_open = true;
        break;
      case 'a':
        data.state.abort = true;
        break;
      case 'z':
        data.state.flight_computer_on = true;
        break;
      case 'c':
        data.state.flight_computer_on = true;
        break;
      case 'r':
        data.state.flight_computer_reset = true;
        break;
      case 'd':
        data.state.ullage_dump = true;
        break;
      case 'y':
        data.state.pixhawk_enable = true;
        break;
      case 'o':
        data.state.take_off = true;
        break;
      case 'k':
        data.state.soft_kill = true;
        break;
      case 'i':
        data.state.ullage_main_tank = true;
        break;
      case 'p':
        data.state.sddump = true;
        break;
      default:
        break;
    }
  }
  lastStateString = data.stateString;
}


//Reset error flags
void resetErrorFlags(health_packet& data){
  data.errorflags.temperature = false;
  data.errorflags.pressure = false;
  data.errorflags.voltage = false;
  data.errorflags.time = false;
}
  
  
  
//PresureTransducerRead
void PressureTransducerRead(health_packet& data){
  
    data.pressure_values[0] = analogRead(pressure_transducer_one_analog);
    data.pressure_values[1] = analogRead(pressure_transducer_two_analog);
    data.pressure_values[2] = analogRead(pressure_transducer_three_analog);
    data.pressure_values[3] = analogRead(pressure_transducer_four_analog);
    data.pressure_values[4] = analogRead(pressure_transducer_five_analog);
    data.pressure_values[5] = analogRead (pressure_transducer_six_analog);
    data.pressure_values[6] = analogRead (pressure_transducer_seven_analog);
    
    for (int i=0;i<7;i++){
      if (data.pressure_values[i] > MAX_PRESSURE)//change this to an array of max pressure
        if (analogRead(i) > MAX_PRESSURE)
          data.errorflags.pressure = true;
    }
}
 

//Voltage Sensor Read    
void voltage_sensor(health_packet& data){
    
   data.voltage = analogRead(voltage_sensor_analog);
}

//ThermoCouple Read
void initThermocoupleMonitor()
{
  TC.begin();
}

double readThermocouple(int index, byte& error_code)
{
  double result, dummy;
  index += 1;
  TC.setMUX(index);
  TC.getTemp(result, dummy, 0, error_code);
  
  return result;
}

void readThermocouples(health_packet& data)
{
  for(int i = 0; i < 6; ++i)
  {
    byte dummy;
    data.temp_values[i] = readThermocouple(i, dummy);
  }
  for (int i=0;i<6;i++){
    if (data.temp_values[i] > MAX_TEMPERATURE)
      if (analogRead(i) > MAX_TEMPERATURE)
        data.errorflags.temperature = true;
  }
}


void sendHealthPacket(health_packet& data)
{
  String outgoingPacket = "p:";
  for (int i = 0; i < 6; i++){
    outgoingPacket += String(data.pressure_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.pressure_values[6]);
  outgoingPacket += String(";t:");
  for (int i = 0; i < 5; i++){
    outgoingPacket += String(data.temp_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.temp_values[5]);
  outgoingPacket += String(";v:");
  outgoingPacket += String(data.voltage);
  outgoingPacket += String(";h:");
  outgoingPacket += String(data.stateString);
  Serial.print(outgoingPacket);
  
}

void readFlags() 
{
   unsigned long time = millis();
   while (Serial.available() && (time+15) >= millis()){
     char check = Serial.read();
     if (check == 'h'){
       check = Serial.read();
       if (check == ':'){
         //here is our health packet
         String incomingString = "";
         while(check != ';' && Serial.available()){
           incomingString +=check;
           check = Serial.read(); 
         }
         current_health_packet.stateString = incomingString;
         lastFlagReadTime = millis();
         Serial.flush();        
       }
     }
   }
   Serial.flush();
}

