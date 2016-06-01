
/*
Healt Mointering Sketch
 
 Overall Inputs Acepted:
 Pressure Transducers - analog inputs pins 0 - 6
 Thermocouple - digital pints 24,26,27,28,29
 Power Realy - digital pin 8  
 
 Overall Output:
 Xbee Radio - 
 Micro SD Card -
 
 (kpa?)
 Max Pressure - 300
 
 (Celcuis)
 Max Temperature = 200
 
 Tank-iso = 11
 Servo Dump Valve = 12
 Fail Closed Valve = 45
 Fail Open Valve = 44
 
 default_0 means no communication + not armed
 default_1 means no communication + armed
 
 */

//libraries
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <WString.h>
#include <ctype.h>
#include <EEPROM.h>
#include <MAX31855.h>
#include <Servo.h>
#include <stdio.h>
#include <Adafruit_CC3000.h>
#include "utility/debug.h"
#include "utility/socket.h"
#include <Adafruit_CC3000.h>
#include <string.h>
#include <stdlib.h>

//Wifi
#define WLAN_SSID       "XT1058"        // cannot be longer than 32 characters!
#define WLAN_PASS       "lunarlion"

#define WLAN_SECURITY   WLAN_SEC_WPA2

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    6
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIVIDER); // you can change this clock speed

uint32_t ip = 3232246735;

Adafruit_CC3000_Client client;


//Pinouts
#define TANK_ISO 11
#define DUMP_VALVE 12
#define FAIL_CLOSED 45
#define FAIL_OPEN 44

//Servo
Servo tank_iso_servo;
Servo dump_valve_servo;

//SD Card
File recordingFile;

//Power Relay
#define power_relay_digital_on 8
#define power_relay_digital_off 10

//Pressure Transducer
#define pressure_transducer_one_analog 0
#define pressure_transducer_two_analog 1
#define pressure_transducer_three_analog 2
#define pressure_transducer_four_analog 3
#define pressure_transducer_five_analog 4
#define pressure_transducer_six_analog 5
#define pressure_transducer_seven_analog 6

//thermocouple
#define THERMOCOUPLE_CHIP_SELECT 24
MAX31855 TC(THERMOCOUPLE_CHIP_SELECT); 

//Max/Min Pressure Values
int MAX_PRESSURE[] = {
  1025,1025,1025,1025,1025,1025,1025};
int MAX_PRESSURE_ABORT[] = {
  5,5,5,5,5,5,5};//5 corresponds to 5 cycles off. 500millis
int pressure_abort[] = {
  0,0,0,0,0,0,0};

//Max/Min Temperature Values
double MAX_TEMPERATURE[] = {
  200,200,200,200,200,200};
int MAX_TEMPERATURE_ABORT[] = {
  5,5,5,5,5,5,5};//5 corresponds to 5 readings of the thermo couples. 8.333seconds
int temperature_abort[] = {
  0,0,0,0,0,0,0};
int thermoCounter = 0;//we start at thermocouple 1
//Voltage Sensor
#define voltage_sensor_analog 7
int MAX_VOLTAGE = 800;
int MAX_VOLTAGE_ABORT = 5;
int voltage_abort = 0; //5 corresponds to 5 cycles or 500 millis.


//Xbee input flags 
struct flags
{
bool safety : 
  1;          // Doesn't respond to any commands when true
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
bool spike : 
  1; // becomes true if pressure/temperature/voltage spike
bool sddump : 
  1;
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
bool temperature : 
  1;//temp abort
bool pressure : 
  1;//pressure abort
bool voltage : 
  1;//voltage abort
bool time : 
  1;//time abort
};

// Data structure for XBee health packet
struct health_packet
{
  int pressure_values[7];
  double temp_values[6];
  int voltage;
  unsigned int motor_values[4];
  unsigned long elapsed;
  flags state;
  flags state_activated;
  craft errorflags;
  String stateString;
};


template <int N>
class print_err;

boolean relayTriggered = false;

long lastFlagReadTime = 0;
long currentTime = 0;
long lastSerialReset = 0;
long loopTime = 0;
long loopTimeCounter = 0;
int relayTimer = 0;

health_packet current_health_packet;
String lastStateString = "";

void checkMotors(health_packet& data);
String createHealthPacket(health_packet& data);
void errorFlagsEvaluation(health_packet& data);
void addFlagToString(health_packet& data);
void sendDataOverSerial2(char input);
void stateFunctionEvaluation(health_packet& data);
void resetState();
void stateEvaluation(health_packet& data);
void resetErrorFlags(health_packet& data);
void PressureTransducerRead(health_packet& data);
void voltage_sensor(health_packet& data);
void readThermocouples(health_packet& data);
void sendHealthPacket(String& str);
void SDcardWrite(String& str);
void healthPacketToString(health_packet& data, String& str);
void valveChecks(health_packet& data);


char arrayToSend[110];

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);
  resetState();//reset bool states
  //Motors/Valves init
  tank_iso_servo.attach(TANK_ISO);
  dump_valve_servo.attach(DUMP_VALVE);
  pinMode(power_relay_digital_on, OUTPUT);
  pinMode(power_relay_digital_off, OUTPUT);
  pinMode(FAIL_CLOSED, OUTPUT);
  pinMode(FAIL_OPEN, OUTPUT);


  tank_iso_servo.writeMicroseconds(1000);
  dump_valve_servo.writeMicroseconds(1000);
  digitalWrite(power_relay_digital_on,LOW);
  digitalWrite(power_relay_digital_off,HIGH);
  delay(100);
  digitalWrite(power_relay_digital_on,LOW);
  digitalWrite(power_relay_digital_off,LOW);
  digitalWrite(FAIL_CLOSED, LOW);
  digitalWrite(FAIL_OPEN, HIGH);

  //temperature shield init.
  TC.begin();
  TC.setMUX(thermoCounter);//call the thermocouple in the start up since this is going to be evaulated first loop.
  delayMicroseconds(50);
  TC.setMUXADJUSTED();
  for (int i=0; i <6;i++){
    current_health_packet.temp_values[i] = 40.00;
  }
  //SD Card
  //SD card initiation
  pinMode(53, OUTPUT);

  //SD card feed back, do not remove this feedback it is neccasry for function of SD card
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    //return;
  }
  else{
    Serial.println("initialized");
  }

  //removes existing file and creates a new blank one
  if (SD.exists("datalog.txt")) {
    SD.remove("datalog.txt");
    Serial.println("file deleted");
  }
  recordingFile = SD.open("datalog.txt");
  recordingFile.close();

  Serial.println("file created");


  //Serial.println("restart");

  //Initialise CC3000 module
  
 
 
  Serial.println(F("\nInitializing CC3000..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));

  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) { //Obtain IP addeess
    delay(100);
  } 

  while (! displayConnectionDetails())
  {
    delay(1000);
  }
  

}

void loop() {

  client = cc3000.connectTCP(ip, 80);
  
  //unenable cc3000

  
  loopTime = millis();
  loopTimeCounter = loopTime+100;
  //read in xbee
  if (client.connected()) {
    readFlags(); //read input
  }
  currentTime = millis();
  // Initialize the health packet
  resetErrorFlags(current_health_packet);
  if (lastFlagReadTime < (currentTime - 300)){
    Serial.clearing();
  }


  //if ((currentTime+500) >= lastSerialReset){
  //Serial.flush();
  //delay(5);
  //lastSerialReset = currentTime;
  //}
  //if no read
  //no read use last health_packet

  //if no read check time >500ms handshake process has failed. Turn on appropriate abort based on last health packet
  if (lastFlagReadTime < (currentTime - 1000)){
    current_health_packet.errorflags.time = true;
    Serial.clearing();
    //Serial.flush();
  }

  //ThermoCouple check temp

  readThermocouples(current_health_packet);

  //Pressure Transducers check pressure
  PressureTransducerRead(current_health_packet);

  //Voltage Sensor check voltage
  voltage_sensor(current_health_packet);
  //check current errorflags
  errorFlagsEvaluation(current_health_packet);
  //check motors
  checkMotors(current_health_packet);
  //Send Health Packer, we don't send flag if time out... this will cause the device to freeze
  String outgoingPacket = createHealthPacket(current_health_packet);
  SDcardWrite(outgoingPacket);
  //if (current_health_packet.errorflags.time){
  sendHealthPacket(outgoingPacket);
  //}
  if (relayTriggered){
    if (relayTimer == 2){
      relayTriggered = false;
      relayTimer = 0;
      digitalWrite(power_relay_digital_on,LOW);
      digitalWrite(power_relay_digital_off,LOW);
    }
    relayTimer++;
  }
  ////////////////////////////////////////////////////////////////////////
  if (!current_health_packet.stateString.equals(lastStateString)){
    stateEvaluation(current_health_packet);
    stateFunctionEvaluation(current_health_packet);
  }
  valveChecks(current_health_packet);

  //checking Serial2
  if (Serial2.available()) {
    Serial.println("Serial 2 connected"); 
  }


  while (loopTime < loopTimeCounter && !((loopTime-loopTimeCounter) > 100)){
    loopTime = millis();
    delay(1);
  }
  TC.setMUXADJUSTED();//this resets the thermocouple shield to be called again. allowing us to run the program while the thermocouple shield computes the data.
}

///////////////////////////////////////////////////
///////////////Fucntions///////////////////////////
///////////////////////////////////////////////////

void valveChecks(health_packet& data){
  if (data.state.tank_iso_open){    
    // Isolation valve
    tank_iso_servo.writeMicroseconds(2000);
  }
  else {
    tank_iso_servo.writeMicroseconds(1000);
  }

  if (data.state.dump_valve_open){  
    // Fuel dump valve
    dump_valve_servo.writeMicroseconds(2000);
  }
  else {
    dump_valve_servo.writeMicroseconds(1000);
  }

  if (data.state.ullage_main_tank){  
    digitalWrite(FAIL_CLOSED, HIGH);
  }
  else {
    digitalWrite(FAIL_CLOSED, LOW);
  }

  if (data.state.ullage_dump){  
    digitalWrite(FAIL_OPEN, LOW);
  }
  else {
    digitalWrite(FAIL_OPEN, HIGH);
  }
}


//get motor functions
void checkMotors(health_packet& data){
  boolean failed = false;
  boolean done = false;
  int loopCounterCheckMotor = 0;
  if(Serial2.available()){
    Serial.println("Serial 2 Connected");
    while (Serial2.available() && loopCounterCheckMotor < 14 && !failed && !done){
      //motors are being read
      loopCounterCheckMotor++;
      char c = Serial2.read();
      if (c == 'm'){
        if (Serial2.available())
          c = Serial2.read();
        if (c == ':'){ 
          int numbers[] = {
            0,0,0,0                                                                      };
          for (int i = 0; i < 4; i++){
            for (int j = 0; j < 4; j++){
              if (Serial2.available()){
                c = Serial2.read();
                numbers[j] = c - '0';
              }                                                           
              if (c == 'm'){
                failed = true;
              }
            }
            //assign reads to four int
            data.motor_values[i] = (numbers[0]*1000) + (numbers[1]*100) + (numbers[2]*10) + (numbers[3]);
          }
          if (failed){
            for (int i = 0; i < 4; i++){
              data.motor_values[i] = 1200;
            }
          }
          done = true;
        }
      }
    }
  }
  else{
    for (int i = 0; i < 4; i++){
      data.motor_values[i] = 1500;
    }
  }
}
//check current error flags
void errorFlagsEvaluation(health_packet& data){

  //for testing, remove after
  if (data.stateString.indexOf('z') == -1){
    data.stateString += String('z');
  }
  if (data.stateString.indexOf('c') == -1){
    data.stateString += String('c');
  }
  if (data.stateString.indexOf('o') == -1){
    data.stateString += String('o');
  }
  if (data.stateString.indexOf('s') == -1){
    data.stateString += String('s');
  }
  if (data.stateString.indexOf('i') == -1){
    data.stateString += String('i');
  }
  if (data.stateString.indexOf('t') == -1){
    data.stateString += String('t');
  }



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

  /*
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
   */
}

void addFlagToString(health_packet& data){
  if (data.stateString.indexOf('e') == -1){
    data.stateString += String('e');
  }
}

//send char over serial2
void sendDataOverSerial2(char input){
  Serial2.write(input);
}

//evaluates logic functions
void stateFunctionEvaluation(health_packet& data){

  //flight states
  /////////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.abort){
    //abort sequence
    //Serial2.write('a');
    Serial2.write('a');
  }
  if (data.state.soft_kill && !data.state_activated.abort){       
    // True starts soft kill procedure
    //Serial2.write('k');
    Serial2.write('k');
  }
  if (data.state.sddump && !data.state_activated.abort){
    //Serial2.write('p');
    Serial2.write('p');
    /*
    while(data.state.sddump){//you've entered the sddump
     //we are going to get all the data off the sd card
     String tempString = "";
     char tempChar = NULL;
     loopTime = millis();
     if (Serial2.available()){
     tempChar = Serial2.read();
     tempString += String(tempChar);
     }
     if (tempChar == ';'){
     data.state.sddump = false;
     tempString += String("?");
     int packetLength = tempString.length()-1;
     tempString += String(packetLength);
     tempString += String("|");
     for (int i=0; i<10; i++){
     Serial.print(tempString);
     delay(100);
     }
     }
     if (loopTime < loopTimeCounter){
     loopTimeCounter = loopTime + 100;
     String outgoingPacket = createHealthPacket(current_health_packet);
     sendHealthPacket(outgoingPacket);
     }
     } 
     */
  }
  if (data.state.take_off && data.state_activated.safety && !data.state_activated.abort){// && !data.state_activated.abort){        //ensures take_off is not enable unless no abort is in effect
    // Take off on true
    //Serial2.write('o');
    Serial2.write('o');
    data.state_activated.take_off = data.state.take_off;
  }

  //activated every time
  //Valve States
  ///////////////////////////////////////////////////////////////////////////////////////////////

  //Computer System Enable States
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_reset && !data.state_activated.abort){  
    // On change, will reset computer
    //Serial2.write('r');
    Serial2.write('r');
    //also reset millis timer
    //*********ADD please***********//
  }
  if (data.state.pixhawk_enable && !data.state_activated.abort){  
    // If true, enable PixHawk
    Serial3.write('o');
  }

  //last state not effected.
  //on off various states
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_on){  
    // True means turn on the flight controller
    digitalWrite(power_relay_digital_off,LOW);
    digitalWrite(power_relay_digital_on,HIGH);
    relayTriggered = true;
  }
  else{
    //make sure flight computer is off
    digitalWrite(power_relay_digital_on,LOW);
    digitalWrite(power_relay_digital_off,HIGH);
    relayTriggered = true;
  }
  //last state assignment
  //////////////////////////////////////////////////////////////////////////////////////////////////
  data.state_activated = data.state;
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
    case 'b':
      data.state.abort = true;

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
    if (data.pressure_values[i] > MAX_PRESSURE[i]){//change this to an array of max pressure
      pressure_abort[i]++;
      if (pressure_abort[i] >= MAX_PRESSURE_ABORT[i])
        data.errorflags.pressure = true;
    }
    else{
      pressure_abort[i] = 0;
    }
  }
}


//Voltage Sensor Read    
void voltage_sensor(health_packet& data){
  data.voltage = analogRead(voltage_sensor_analog);
  if (data.voltage > MAX_VOLTAGE){//change this to an array of max pressure
    voltage_abort++;
    if (voltage_abort >= MAX_VOLTAGE_ABORT)
      data.errorflags.voltage = true;
  }
  else{
    voltage_abort = 0;
  }
}

//ThermoCouple Read
void initThermocoupleMonitor()
{
  TC.begin();
}

double readThermocouple(int index, byte& error)
{
  double result, dummy;

  TC.getTemp(result, dummy, 0, error);
  if (error & 0x01){
    result = -1.0;
  } 
  else if (error & 0x02){
    result = -2.0;
  } 
  else if (error & 0x04){
    result = -3.0;
  }

  return result;
}

void readThermocouples(health_packet& data)
{
  byte dummy;
  int thermoCounterTemp = thermoCounter;
  data.temp_values[thermoCounterTemp] = readThermocouple(thermoCounter, dummy);
  if (data.temp_values[thermoCounterTemp] > MAX_TEMPERATURE[thermoCounterTemp]){//change this to an array of max temperature
    temperature_abort[thermoCounterTemp]++;
    if (temperature_abort[thermoCounterTemp] >= MAX_TEMPERATURE_ABORT[thermoCounterTemp])
      data.errorflags.temperature = true;
  }
  else{
    temperature_abort[thermoCounterTemp] = 0;
  }
  if (thermoCounter == 5){
    thermoCounter = 0;
  }
  else{
    thermoCounter++;
  }

  TC.setMUX(thermoCounter);
}


String createHealthPacket(health_packet& data)
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
  outgoingPacket += String(";m:");
  for (int i = 0; i < 3; i++){
    outgoingPacket += String(data.motor_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.motor_values[3]);
  outgoingPacket += String(";h:");
  outgoingPacket += String(data.stateString);
  outgoingPacket += String(";s:");
  outgoingPacket += String(loopTime);
  outgoingPacket += String(";?");
  int packetLength = outgoingPacket.length()-1;
  outgoingPacket += String(packetLength);
  outgoingPacket += String("|");
  //Serial.flush();
  return outgoingPacket;
}

void sendHealthPacket(String& str){
  str.toCharArray(arrayToSend, 110);
  client.fastrprintln(arrayToSend);
}

void SDcardWrite(String& str){
  ///
  File recordingFile = SD.open("datalog.txt", FILE_WRITE);
  delay(1);
  if (recordingFile){
    recordingFile.print(loopTime);
    recordingFile.print("|");
    recordingFile.println(str);
    recordingFile.close();
  }
}



void readFlags() 
{
  long timeEval = millis();
  long time = timeEval + 50;
  boolean packetReceived = false;
  while (!packetReceived && (time >= timeEval)){
    timeEval = millis();
    char check = NULL;
    if (client.available()){
      check = client.read();
    }
    if (check == 'h'){
      check = client.read();
      if (check == ':'){
        //here is our health packet
        String incomingString = "";
        while(check != ';' && client.available() && (time >= timeEval)){
          timeEval = millis();
          check = client.read();
          if(check != ';'){
            incomingString +=check;
          } 
        }
        String checkSum = "";
        while(check != '|' && (time >= timeEval)){
          timeEval = millis();
          if (client.available()){
            check = client.read();
            if (check != '|'){
              checkSum +=check;
            }
          }
        }
        int checkSumInt = checkSum.toInt();
        if (checkSumInt == incomingString.length()){
          current_health_packet.stateString = incomingString;
          lastFlagReadTime = millis();
          packetReceived = true;
        }

        //Serial.flush();
        while (client.available() && (time >= timeEval)){
          client.read();
          timeEval = millis();
        }
      }
    }
  }
  //Serial.flush();
}

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); 
    cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); 
    cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); 
    cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); 
    cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}








