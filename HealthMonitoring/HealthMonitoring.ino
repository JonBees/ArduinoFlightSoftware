
/*
Health Monitoring Sketch
 
 Inputs:
 Pressure Transducers - analog input pins 0 - 6
 Thermocouple - digital pins 24,26,27,28,29,50,52
 Power Relay - digital pins 8,10  
 
 Output:
 Xbee Radio - Serial1
 Micro SD Card -  52 to CLK
                  50 to MISO
                  51 to MOSI
                  53 to CS
 
 Tank-iso = 12
 Servo Dump Valve = 11
 Fail Closed Valve = 45
 Fail Open Valve = 44

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

//Pinouts
#define TANK_ISO 12
#define DUMP_VALVE 11
#define FAIL_CLOSED 45
#define FAIL_OPEN 44
#define POWERBOX_FAN 7

//Servo
Servo tank_iso_servo;
Servo dump_valve_servo;

//SD Card
File recordingFile;
#define SD_CHIP_SELECT 53 //needs to be set to LOW when the SD card is being used and HIGH otherwise.

//Power Relay
#define power_relay_digital_on 8
#define power_relay_digital_off 10

//Pressure Transducers
#define pressure_transducer_one_analog 0
#define pressure_transducer_two_analog 1
#define pressure_transducer_three_analog 2
#define pressure_transducer_four_analog 3
#define pressure_transducer_five_analog 4
#define pressure_transducer_six_analog 5
#define pressure_transducer_seven_analog 6

//thermocouple
#define THERMOCOUPLE_CHIP_SELECT 24 //needs to be set to LOW when the Thermocouple shield is being used and HIGH otherwise.
MAX31855 TC(THERMOCOUPLE_CHIP_SELECT); 

//Max Pressure Values
int MAX_PRESSURE[] = {
  901,907,903,903,901,912,910};
int MAX_PRESSURE_ABORT[] = {
  5,5,5,5,5,5,5};//5 corresponds to 5 cycles off. 1s
int pressure_abort[] = {
  0,0,0,0,0,0,0};

//Max Temperature Values

//Celsius Values
/*double MAX_TEMPERATURE[] = {
  65.5,1093,65.5,1093,1093,1093};*/ 

//Fahrenheit Values
double MAX_TEMPERATURE[]={
  150,2000,150,2000,2000,2000};

int MAX_TEMPERATURE_ABORT[] = {
  5,5,5,5,5,5};//5 corresponds to 5 readings of the thermocouples. 1s
int temperature_abort[] = {
  0,0,0,0,0,0};
int thermoCounter = 1;//we start at thermocouple 2 (zero indexed)

double boxTemp = 0;
int boxTempAbort = 0;


//Voltage Sensor
#define voltage_sensor_analog 7
int MIN_VOLTAGE = 177;
int MIN_VOLTAGE_ABORT = 5;
int voltage_abort = 0; //5 corresponds to 5 cycles or 1s.


//Xbee input flags 
struct flags
{
bool safety : 
  1;  // When true, disallow takeoff
bool tank_iso_open : 
  1;  // Isolation valve
bool dump_valve_open : 
  1;  // Fuel dump valve
bool abort : 
  1;  // If true, abort
bool flight_computer_on : 
  1;  // When true, turn on power relay
bool flight_computer_reset : 
  1;  // When true, reset flight computer
bool ullage_dump : 
  1;  // When true, opens the valve to dump ullage
bool take_off : 
  1;  // When true, (and safety is false) begins takeoff
bool soft_kill : 
  1;  // True starts soft kill procedure
bool ullage_main_tank : 
  1;  //When true, opens valve to the ullage tank
bool sddump : 
  1;
  flags() {
    safety = true;          
    tank_iso_open = false;    
    dump_valve_open = false;  
    abort = false;            
    flight_computer_on = false;  
    flight_computer_reset = false;  
    ullage_dump = false;      
    take_off = false;        
    soft_kill = false;       
    ullage_main_tank = false;  
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
long loopTime = 0;
long loopTimeCounter = 0;
int relayTimer = 0;

health_packet current_health_packet;
String lastStateString = "";

void checkMotors(health_packet& data);
String createHealthPacket(health_packet& data);
void errorFlagsEvaluation(health_packet& data);
void addFlagToString(health_packet& data);
void stateFunctionEvaluation(health_packet& data);
void resetState();
void stateEvaluation(health_packet& data);
void resetErrorFlags(health_packet& data);
void PressureTransducerRead(health_packet& data);
void voltage_sensor(health_packet& data);
void readThermocouples(health_packet& data);
void sendHealthPacket(String& str);
void SDcardWrite(String& str);
void valveChecks(health_packet& data);


void setup() {
  Serial.begin(9600);//USB connection
  Serial1.begin(9600);//connection with XBee
  Serial2.begin(9600);//connection with FlightComputer
  Serial3.begin(9600);//currently disconnected
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
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);
  for (int i=0; i <6;i++){
    current_health_packet.temp_values[i] = 0.00;
  }
  
  //SD card initiation
  pinMode(SD_CHIP_SELECT, OUTPUT);

  //SD card feedback - apparently necessary for initialization
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("initialization failed!");
    return;
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
}


void loop() {
  loopTime = millis();
  loopTimeCounter = loopTime+200;
  readFlags(); //read input
  currentTime = millis();
  resetErrorFlags(current_health_packet);

    //if no read check time 5 min handshake process has failed. Turn on appropriate abort based on last health packet

    if (lastFlagReadTime < (currentTime - 300000)){
    current_health_packet.errorflags.time = true;
//    Serial1.clearing();
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
  //Send Health Packet, we don't send flag if time out... this will cause the device to freeze
  String outgoingPacket = createHealthPacket(current_health_packet);
  SDcardWrite(outgoingPacket);
  sendHealthPacket(outgoingPacket);
  //Adjust fan speed based on power box's temperature
  adjustFanSpeed();


  if (relayTriggered){
    if (relayTimer == 2){
      relayTriggered = false;
      relayTimer = 0;
      digitalWrite(power_relay_digital_on,LOW);
      digitalWrite(power_relay_digital_off,LOW);
    }
    relayTimer++;

  }
  if (!current_health_packet.stateString.equals(lastStateString)){
    stateEvaluation(current_health_packet);
    stateFunctionEvaluation(current_health_packet);
  }

  //if abort, change valve states
  if (current_health_packet.state.abort) {
    current_health_packet.state.tank_iso_open = false;
    if (current_health_packet.stateString.indexOf('t') != -1) {
      current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('t'), 1);
    }
    current_health_packet.state.ullage_dump = true;
    if (current_health_packet.stateString.indexOf('d') == -1) {
      current_health_packet.stateString += String('d');
    }
    current_health_packet.state.ullage_main_tank = false;
    if (current_health_packet.stateString.indexOf('i') != -1) {
      current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('i'), 1);
    }
  }

  valveChecks(current_health_packet);

  while (loopTime < loopTimeCounter && !((loopTime-loopTimeCounter) > 200)){
    loopTime = millis();
    delay(1);
  }
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);//this resets the thermocouple shield to be called again. allowing us to run the program while the thermocouple shield computes the data.
}

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
  if (data.state.ullage_main_tank){  //FAIL_CLOSED is closed when it has no power
    digitalWrite(FAIL_CLOSED, HIGH);
  }
  else {
    digitalWrite(FAIL_CLOSED, LOW);
  }
  if (data.state.ullage_dump){  //FAIL_OPEN is open when it has no power
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
    while (Serial2.available() && loopCounterCheckMotor < 14 && !failed && !done){
      //motors are being read
      loopCounterCheckMotor++;
      char c = Serial2.read();
      if (c == 'm'){
        if (Serial2.available())
          c = Serial2.read();
        if (c == ':'){ 
          int numbers[] = {
            0,0,0,0                                                                                                                                                                                              };
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
              data.motor_values[i] = 1000;
            }
          }
          done = true;
        }
      }
    }
  }
  else{
    for (int i = 0; i < 4; i++){
      data.motor_values[i] = 1000;
    }
  }
}
//check current error flags
void errorFlagsEvaluation(health_packet& data){

  if (current_health_packet.errorflags.temperature){
    //first append craft abort
    addFlagToString(current_health_packet);
    //temperature abort
    data.state.soft_kill = true;
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.pressure){
    //first append craft abort
    addFlagToString(current_health_packet);
    //pressure abort

    data.state.soft_kill = true;
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.voltage){
    //first append craft abort
    addFlagToString(current_health_packet);
    //voltage abort

    data.state.soft_kill = true;
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.time){
    //timeout abort
    data.state.abort = true;
    if (data.stateString.indexOf('a') == -1){
      data.stateString += String('a');
    }
  }
  if (data.state.soft_kill || softkillGlobal) {
    addFlagToString(current_health_packet);
  }
}
void addFlagToString(health_packet& data){
  if (data.stateString.indexOf('e') == -1){
    data.stateString += String('e');
  }
}

void stateFunctionEvaluation(health_packet& data){

  //flight states
  if (data.state.abort){
    //abort sequence
    //Serial2.write('a');
    Serial2.write('a');
  }
  if ((data.state.soft_kill)&& !data.state_activated.abort){       
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
  if (data.state.take_off && !data.state_activated.safety && !data.state_activated.abort && !data.state_activated.soft_kill){// && !data.state_activated.abort){        //ensures take_off is not enable unless no abort is in effect
    // Take off on true
    //Serial2.write('o');
    Serial2.write('o');
    data.state_activated.take_off = data.state.take_off;
  }

  //Flight computer state adjustments
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_reset && !data.state_activated.abort && !data.state_activated.safety){  
    // On change, will reset computer
    //Serial2.write('r');
    Serial2.print('r');
  }

  if (data.state.flight_computer_on && !data.state_activated.safety){  
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

  //store currently used state
  data.state_activated = data.state;
}


void stateEvaluation(health_packet& data){
  String tempString = data.stateString;
  int stringLength = tempString.length();
  resetState();
  for (int i = 0; i < stringLength;i++){
    char ramp = tempString[i];
    switch(ramp){
    case 's':
      data.state.safety = false;
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

//used to set all bools false
void resetState()
{
  flags tempFlag;
  current_health_packet.state = tempFlag;
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
    if (data.pressure_values[i] > MAX_PRESSURE[i]){
      pressure_abort[i]++;
    }
    else{
      pressure_abort[i] = 0;
    }
    if (pressure_abort[i] >= MAX_PRESSURE_ABORT[i]){
      data.errorflags.pressure = true;
    }
  }
}


//Voltage Sensor Read    
void voltage_sensor(health_packet& data){
  data.voltage = analogRead(voltage_sensor_analog);
  if (data.voltage < MIN_VOLTAGE){
    voltage_abort++;
  }
  else{
    voltage_abort = 0;
  }

  if (voltage_abort >= MIN_VOLTAGE_ABORT) {
    data.errorflags.voltage = true;
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

  TC.getTemp(result, dummy, 2, error); // ExternalTempDoubleVariable, InternalTempDoubleVariable, SCALE, ErrorByteVariable) --- SCALE: 0 for Celsius/Centigrade, 1 for Kelvin, 2 for Fahrenheit, and 3 for Rankine.
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
  digitalWrite(SD_CHIP_SELECT, HIGH);//disables connection to SD card while we're reading TC values
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, LOW);//enables connection from TC board
  byte dummy;
  int thermoCounterTemp = thermoCounter-1;

  if (thermoCounter == 7){
    boxTemp = readThermocouple(thermoCounter, dummy);
    if (boxTemp >= 122) {
      boxTempAbort++;
    }
    else {
      boxTempAbort = 0;
    }
    if (boxTempAbort >= 5) {
      data.errorflags.temperature = true; 
    }
    thermoCounter = 1;
  }
  else{
    data.temp_values[thermoCounterTemp] = readThermocouple(thermoCounter, dummy);
    if (data.temp_values[thermoCounterTemp] > MAX_TEMPERATURE[thermoCounterTemp]){
      temperature_abort[thermoCounterTemp]++;
    }
    else{
      temperature_abort[thermoCounterTemp] = 0;
    }
    if (temperature_abort[thermoCounterTemp] >= MAX_TEMPERATURE_ABORT[thermoCounterTemp]){
      data.errorflags.temperature = true;
    }
    thermoCounter++;
  }

  TC.setMUX(thermoCounter);
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);//disables connection from TC board
}

void adjustFanSpeed(){
  double temp = TC.intTemp(2);//gets temperature of internal thermocouple (in F)
  int fanValue = (temp - 135)*10;//scales from 0 to 250
  if(temp>135 && temp < 160){
    analogWrite(POWERBOX_FAN, fanValue);
  }
  else if(temp < 135){
    analogWrite(POWERBOX_FAN, 0);
  }
  else if(temp > 160){
    analogWrite(POWERBOX_FAN, 255);
  }
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
  return outgoingPacket;
}

void sendHealthPacket(String& str){
  Serial1.println(str);
  Serial.println(str);
}

void SDcardWrite(String& str){
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);
  digitalWrite(SD_CHIP_SELECT, LOW);
  File recordingFile = SD.open("datalog.txt", FILE_WRITE);
  delay(1);
  if (recordingFile){
    recordingFile.print(loopTime);
    recordingFile.print("|");
    recordingFile.print(str);
    recordingFile.print("b:");
    recordingFile.println(boxTemp);
    recordingFile.close();
  }
  digitalWrite(SD_CHIP_SELECT, HIGH);
}



void readFlags() 
{
  long timeEval = millis();
  long time = timeEval + 200;
  boolean packetReceived = false;
  while (!packetReceived && (time >= timeEval)){
    timeEval = millis();
    char check = NULL;
    if (Serial1.available()){
      check = Serial1.read();
    }
    if (check == 'h'){
      check = Serial1.read();
      if (check == ':'){
        //here is our health packet
        String incomingString = "";
        while(check != ';' && Serial1.available() && (time >= timeEval)  && !((loopTime-loopTimeCounter) > 200)){
          timeEval = millis();
          check = Serial1.read();
          if(check != ';'){
            incomingString +=check;
          } 
        }
        String checkSum = "";
        while(check != '|' && (time >= timeEval) && !((loopTime-loopTimeCounter) > 200)){
          timeEval = millis();
          if (Serial1.available()){
            check = Serial1.read();
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
        while (Serial1.available() && (time >= timeEval) && !((loopTime-loopTimeCounter) > 200)){
          Serial1.read();
          timeEval = millis();
        }
      }
    }
  }
}
