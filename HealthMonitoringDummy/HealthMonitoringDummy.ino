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
#define AV5_M 12
#define AV6_M 11
#define FC_U 45
#define FO_U 44
#define POWERBOX_FAN 7

//Servo
Servo AV5_M_servo;
Servo AV6_M_servo;

//SD Card
File recordingFile;
#define SD_CHIP_SELECT 53 //needs to be set to LOW when the SD card is being used and HIGH otherwise.

//Power Relay
#define power_relay_digital_on 8
#define power_relay_digital_off 10

//Pressure Transducers
#define PT4_M 0
#define PT1_M 1
#define PT3_M 2
#define PT1_U 3
#define PT2_U 4
#define PT2_M 5
#define PT5_M 6

//thermocouple
#define THERMOCOUPLE_CHIP_SELECT 24 //needs to be set to LOW when the Thermocouple shield is being used and HIGH otherwise.
MAX31855 TC(THERMOCOUPLE_CHIP_SELECT); 

//Max Pressure Values
int SOFTKILL_MAX_PRESSURE[] = {
  900,900,900,10000,900,900,900};
int ABORT_MAX_PRESSURE[] = {
  1000,1000,1000,10000,1000,1000,1000};
int MAX_SOFTKILL_PRESSURE_OVERAGES[] = {
  5,5,5,5,5,5,5};//5 corresponds to 5 cycles off. 1s
int softkill_pressure_overages[] = {
  0,0,0,0,0,0,0};
int MAX_ABORT_PRESSURE_OVERAGES[] = {
  5,5,5,5,5,5,5};
int abort_pressure_overages[] = {
  0,0,0,0,0,0,0};

//Max Temperature Values
//TCpw-1,2,3,4, TCp-1,2
//Celsius Values
/*double SOFTKILL_MAX_TEMPERATURE[] = {
  65.5,1093,65.5,1093,1093,1093};*/ 

//Fahrenheit Values
double SOFTKILL_MAX_TEMPERATURE[]={
  1400,1400,1400,1400,140,140};
double ABORT_MAX_TEMPERATURE[]={
  14000,14000,14000,14000,200,200};

int MAX_SOFTKILL_TEMPERATURE_OVERAGES[] = {
  5,5,5,5,5,5};//5 corresponds to 5 readings of the thermocouples. 1s
int softkill_temperature_overages[] = {
  0,0,0,0,0,0};
int MAX_ABORT_TEMPERATURE_OVERAGES[] = {
  5,5,5,5,5,5};
int abort_temperature_overages[] = {
  0,0,0,0,0,0};

int thermoCounter = 1;//we start at thermocouple 2 (zero indexed)

double boxTemp = 0;
int boxTempAbort = 0;


//Voltage Sensor
#define voltage_sensor_analog 7
int MIN_VOLTAGE = 487;//14.5v
int MAX_VOLTAGE = 850;//17v
int MIN_VOLTAGE_SOFTKILL = 25;//25 cycles -- 5s
int MAX_VOLTAGE_ABORT = 5;//5 cycles -- 1s
int voltage_softkill_underages = 0; 
int voltage_abort_overages = 0;

//Xbee input flags 
struct flags
{
bool safety : 
  1;  // When true, disallow takeoff
bool AV5_M_open : 
  1;  // Isolation valve
bool AV6_M_open : 
  1;  // Fuel dump valve
bool abort : 
  1;  // If true, abort
bool flight_computer_on : 
  1;  // When true, turn on power relay
bool flight_computer_reset : 
  1;  // When true, reset flight computer
bool FO_U_dump : 
  1;  // When true, opens the valve to dump ullage
bool take_off : 
  1;  // When true, (and safety is false) begins takeoff
bool soft_kill : 
  1;  // True starts soft kill procedure
bool FC_U_open : 
  1;  //When true, opens valve to the ullage tank
bool sddump : 
  1;
bool fuel_dump : 
  1; //when true, sets av5 open, av6 closed, fo-u closed, fc-u open, av1-4 50%
bool fan_off :
  1; //when true, overrides fan control and forces fan off
  flags() {
    safety = true;          
    AV5_M_open = false;    
    AV6_M_open = false;  
    abort = false;            
    flight_computer_on = false;  
    flight_computer_reset = false;  
    FO_U_dump = true;      
    take_off = false;        
    soft_kill = false;       
    FC_U_open = false;  
    sddump = false;
    fuel_dump = false;
    fan_off = false;
  };
};

struct craft
{
bool temperature_softkill : 
  1; 
bool temperature_abort : 
  1; 
bool pressure_softkill : 
  1; 
bool pressure_abort : 
  1; 
bool voltage_softkill : 
  1;
bool voltage_abort :
  1;
bool time : 
  1; 
};


// Data structure for XBee health packet
struct health_packet
{
  int pressure_values[7];
  double temp_values[6] = {1200,1201,1202,1203,121,122};
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
int dumpTimer = 0;

int looplength = 200;

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
void readPressureTransducers(health_packet& data);
void checkVoltage(health_packet& data);
void readThermocouples(health_packet& data);
void sendHealthPacket(String& str);
void SDcardWrite(String& str);
void checkValves(health_packet& data);


void setup() {
  Serial.begin(9600);//USB connection
  Serial.begin(9600);//connection with XBee
  Serial2.begin(9600);//connection with FlightComputer
  Serial3.begin(9600);//currently disconnected
  resetState();//reset bool states
  //Motors/Valves init
  AV5_M_servo.attach(AV5_M);
  AV6_M_servo.attach(AV6_M);
  pinMode(power_relay_digital_on, OUTPUT);
  pinMode(power_relay_digital_off, OUTPUT);
  pinMode(FC_U, OUTPUT);
  pinMode(FO_U, OUTPUT);


  AV5_M_servo.writeMicroseconds(1000);
  AV6_M_servo.writeMicroseconds(1000);
  digitalWrite(power_relay_digital_on,LOW);
  digitalWrite(power_relay_digital_off,HIGH);
  delay(100);
  digitalWrite(power_relay_digital_on,LOW);
  digitalWrite(power_relay_digital_off,LOW);
  digitalWrite(FC_U, HIGH); //these two make the solenoids click on powerup to ensure functionality. 
  digitalWrite(FO_U, HIGH);

  //temperature shield init.
  TC.begin();
  TC.setMUX(thermoCounter);//call the thermocouple in the start up since this is going to be evaulated first loop.
  delayMicroseconds(50);
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);
  for (int i=0; i <6;i++){
    //current_health_packet.temp_values[i] = 0.00;
  }
  
  //SD card initiation
  pinMode(SD_CHIP_SELECT, OUTPUT);

  //SD card feedback - apparently necessary for initialization
  /*if (!SD.begin(SD_CHIP_SELECT)) {
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

  Serial.println("file created");*/
}


void loop() {
  loopTime = millis();
  loopTimeCounter = loopTime+looplength;
  readFlags(); //read input
  currentTime = millis();
  resetErrorFlags(current_health_packet);

    //if no read check time 30 sec handshake process has failed. Turn on appropriate abort based on last health packet
  if (lastFlagReadTime < (currentTime - 30000)){
    current_health_packet.errorflags.time = true;
  }

  //ThermoCouple check temp
  readThermocouples(current_health_packet);
  //Pressure Transducers check pressure
  readPressureTransducers(current_health_packet);
  //Voltage Sensor check voltage
  checkVoltage(current_health_packet);
  //check current errorflags
  errorFlagsEvaluation(current_health_packet);
  //check motors
  checkMotors(current_health_packet);
  //Send Health Packet, we don't send flag if time out... this will cause the device to freeze
  String outgoingPacket = createHealthPacket(current_health_packet);
  SDcardWrite(outgoingPacket);
  sendHealthPacket(outgoingPacket);
  //Adjust fan speed based on power box's temperature
  adjustFanSpeed(current_health_packet);


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
    current_health_packet.state.AV5_M_open = false;
    if (current_health_packet.stateString.indexOf('t') != -1) {
      current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('t'), 1);
    }
    current_health_packet.state.FO_U_dump = true;
    if (current_health_packet.stateString.indexOf('d') != -1) {
      current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('d'), 1);
    }
    current_health_packet.state.FC_U_open = false;
    if (current_health_packet.stateString.indexOf('i') != -1) {
      current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('i'), 1);
    }
  }
  
  if(current_health_packet.state.fuel_dump){
    if(dumpTimer == 0){
      current_health_packet.state.AV5_M_open = true;
      if(current_health_packet.stateString.indexOf('t') == -1){
        current_health_packet.stateString += String('t');
      }
      current_health_packet.state.AV6_M_open = false;
      if(current_health_packet.stateString.indexOf('v') != -1){
        current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('v'),1);
      }
    }
    if(dumpTimer == 3){
      current_health_packet.state.FO_U_dump = false;
      if (current_health_packet.stateString.indexOf('d') == -1) {
        current_health_packet.stateString += String('d');
      }
      current_health_packet.state.FC_U_open = true;
      if (current_health_packet.stateString.indexOf('i') == -1) {
        current_health_packet.stateString += String('i');
      }
      dumpTimer = -1;
    }
    dumpTimer++;
  }

  checkValves(current_health_packet);

  while (loopTime < loopTimeCounter && !((loopTime-loopTimeCounter) > looplength)){
    loopTime = millis();
    //delay(1);
  }
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);//this resets the thermocouple shield to be called again. allowing us to run the program while the thermocouple shield computes the data.
}

void checkValves(health_packet& data){
  if (data.state.AV5_M_open){    
    // Isolation valve
    AV5_M_servo.writeMicroseconds(2000);
  }
  else {
    AV5_M_servo.writeMicroseconds(1000);
  }
  if (data.state.AV6_M_open){  
    // Fuel dump valve
    AV6_M_servo.writeMicroseconds(2000);
  }
  else {
    AV6_M_servo.writeMicroseconds(1000);
  }
  if (data.state.FC_U_open){  //FC_U is closed when it has no power
    digitalWrite(FC_U, HIGH);
  }
  else {
    digitalWrite(FC_U, LOW);
  }
  if (data.state.FO_U_dump){  //FO_U is open when it has no power
    digitalWrite(FO_U, LOW);
  }
  else {
    digitalWrite(FO_U, HIGH);
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
            0,0,0,0};
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
              data.motor_values[i] = 3100;
            }
          }
          done = true;
        }
      }
    }
  }
  else{
    for (int i = 0; i < 4; i++){
      data.motor_values[i] = 3200;
    }
  }
}
//check current error flags
void errorFlagsEvaluation(health_packet& data){

  if (current_health_packet.errorflags.temperature_softkill){
    addFlagToString(current_health_packet);
    data.state.soft_kill = true;
    Serial.println(" Temperature Softkill ");
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if(current_health_packet.errorflags.temperature_abort){
    addFlagToString(current_health_packet);
    data.state.abort = true;
    Serial.println(" Temperature Abort ");
    if (data.stateString.indexOf('a') == -1){
      data.stateString += String('a');
    }
  }
  if (current_health_packet.errorflags.pressure_softkill){
    addFlagToString(current_health_packet);
    data.state.soft_kill = true;
    Serial.println(" Pressure Softkill ");
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if(current_health_packet.errorflags.pressure_abort){
    addFlagToString(current_health_packet);
    data.state.abort = true;
    Serial.println(" Pressure Abort ");
    if (data.stateString.indexOf('a') == -1){
      data.stateString += String('a');
    }
  }
  if(current_health_packet.errorflags.voltage_abort){
    addFlagToString(current_health_packet);
    data.state.abort = true;
    Serial.println(" Voltage Abort ");
    if (data.stateString.indexOf('a') == -1){
      data.stateString += String('a');
    }
  }
  if (current_health_packet.errorflags.voltage_softkill){
    addFlagToString(current_health_packet);
    data.state.soft_kill = true;
    Serial.println(" Voltage Softkill ");
    if (data.stateString.indexOf('k') == -1){
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.time){
    //timeout abort
    data.state.abort = true;
    Serial.println(" Time Abort ");
    if (data.stateString.indexOf('a') == -1){
      data.stateString += String('a');
    }
  }
  if (data.state.soft_kill) {
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
    Serial2.write('a');
  }
  if ((data.state.soft_kill)&& !data.state_activated.abort){       
    // True starts soft kill procedure
    Serial2.write('k');
  }
  if (data.state.sddump && !data.state_activated.abort){
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
  if(data.state.fuel_dump){
    Serial2.write('b');
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
      data.state.AV5_M_open = true;
      break;
    case 'v':
      data.state.AV6_M_open = true;
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
      data.state.FO_U_dump = false;
      break;
    case 'o':
      data.state.take_off = true;
      break;
    case 'k':
      data.state.soft_kill = true;
      break;
    case 'i':
      data.state.FC_U_open = true;
      break;
    case 'p':
      data.state.sddump = true;
      break;
    case 'b':
      data.state.fuel_dump = true;
      break;
    case 'f':
      data.state.fan_off = true;

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
  data.errorflags.temperature_softkill = false;
  data.errorflags.temperature_abort = false;
  data.errorflags.pressure_softkill = false;
  data.errorflags.pressure_abort = false;
  data.errorflags.voltage_softkill = false;
  data.errorflags.voltage_abort = false;
  data.errorflags.time = false;
}

//PresureTransducerRead
void readPressureTransducers(health_packet& data){

  data.pressure_values[0] = 840 /*analogRead(PT4_M)*/;
  data.pressure_values[1] = 841 /*analogRead(PT1_M)*/;
  data.pressure_values[2] = 842 /*analogRead(PT3_M)*/;
  data.pressure_values[3] = 1000 /*analogRead(PT1_U)*/;
  data.pressure_values[4] = 844 /*analogRead(PT2_U)*/;
  data.pressure_values[5] = 845 /*analogRead (PT2_M)*/;
  data.pressure_values[6] = 846 /*analogRead (PT5_M)*/;

  for (int i=0;i<7;i++){
    if(data.pressure_values[i] > ((ABORT_MAX_PRESSURE[i]*1.27)-250)){
      abort_pressure_overages[i]++;
    }
    else if (data.pressure_values[i] > ((SOFTKILL_MAX_PRESSURE[i]*1.27)-250)){
      softkill_pressure_overages[i]++;
    }
    else{
      softkill_pressure_overages[i] = 0;
      abort_pressure_overages[i] = 0;
    }

    if(abort_pressure_overages[i] >= MAX_ABORT_PRESSURE_OVERAGES[i]){
      data.errorflags.pressure_abort = true;
    }
    else if (softkill_pressure_overages[i] >= MAX_SOFTKILL_PRESSURE_OVERAGES[i]){
      data.errorflags.pressure_softkill = true;
    }
  }
}


//Voltage Sensor Read    
void checkVoltage(health_packet& data){
  data.voltage = 570/*analogRead(voltage_sensor_analog)*/;
  if (data.voltage < MIN_VOLTAGE){
    voltage_softkill_underages++;
  }
  else{
    voltage_softkill_underages = 0;
  }
  if(data.voltage > MAX_VOLTAGE){
    voltage_abort_overages++;
  }
  else{
    voltage_abort_overages = 0;
  }

  if(voltage_softkill_underages >= MIN_VOLTAGE_SOFTKILL) {
    data.errorflags.voltage_softkill = true;
  }
  if(voltage_abort_overages >= MAX_VOLTAGE_ABORT) {
    data.errorflags.voltage_abort = true;
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
  /*byte dummy;
    int thermoCounterTemp = thermoCounter-1;
    if (thermoCounter == 7){
      boxTemp = readThermocouple(thermoCounter, dummy);
      if (boxTemp >= 165) {
        boxTempAbort++;
      }
      else {
        boxTempAbort = 0;
      }
      if (boxTempAbort >= 5) {
        data.errorflags.temperature_softkill = true; 
      }
      thermoCounter = 1;
    }
    else{
      data.temp_values[thermoCounterTemp] = readThermocouple(thermoCounter, dummy);
      if(data.temp_values[thermoCounterTemp] > ABORT_MAX_TEMPERATURE[thermoCounterTemp]){
        abort_temperature_overages[thermoCounterTemp]++;
      }
      else if (data.temp_values[thermoCounterTemp] > SOFTKILL_MAX_TEMPERATURE[thermoCounterTemp]){
        softkill_temperature_overages[thermoCounterTemp]++;
      }
      else{
        softkill_temperature_overages[thermoCounterTemp] = 0;
        abort_temperature_overages[thermoCounterTemp] = 0;
      }

      if(abort_temperature_overages[thermoCounterTemp] >= MAX_ABORT_TEMPERATURE_OVERAGES[thermoCounterTemp]){
        data.errorflags.temperature_abort = true;
      }
      else if (softkill_temperature_overages[thermoCounterTemp] >= MAX_SOFTKILL_TEMPERATURE_OVERAGES[thermoCounterTemp]){
        data.errorflags.temperature_softkill = true;
      }
      thermoCounter++;
    }*/
  TC.setMUX(thermoCounter);
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);//disables connection from TC board
}

void adjustFanSpeed(health_packet& data){
  if(!data.state.fan_off){
      /*double temp = TC.intTemp(2);//gets temperature of internal thermocouple (in F)
    int fanValue = (temp - 135)*8;//scales from 0 to 250
    if(temp>135 && temp < 160){
      analogWrite(POWERBOX_FAN, fanValue);
    }
    else if(temp < 135){
      analogWrite(POWERBOX_FAN, 0);
    }
    else if(temp > 160){*/
      analogWrite(POWERBOX_FAN, 200);
  //}
  }
  else{
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
  //outgoingPacket += String(Serial.available());
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
  Serial.println(str);
  //Serial.println(str);
  //Serial.println(Serial.available());
}

void SDcardWrite(String& str){
  digitalWrite(THERMOCOUPLE_CHIP_SELECT, HIGH);
  digitalWrite(SD_CHIP_SELECT, LOW);
  /*File recordingFile = SD.open("datalog.txt", FILE_WRITE);
  delay(1);
  if (recordingFile){
    recordingFile.print(loopTime);
    recordingFile.print("|");
    recordingFile.print(str);
    recordingFile.print("b:");
    recordingFile.println(boxTemp);
    recordingFile.close();
  }*/
  digitalWrite(SD_CHIP_SELECT, HIGH);
}



void readFlags() 
{
  long timeEval = millis();
  long time = timeEval + looplength;
  boolean packetReceived = false;
  while (!packetReceived && (time >= timeEval)){
    timeEval = millis();
    char check;
    if (Serial.available()){
      check = Serial.read();
    }
    if (check == 'h'){
      check = Serial.read();
      if (check == ':'){
        //here is our health packet
        String incomingString = "";
        while(check != ';' && Serial.available() && (time >= timeEval)  && !((loopTime-loopTimeCounter) > looplength)){
          timeEval = millis();
          check = Serial.read();
          if(check != ';'){
            incomingString +=check;
          } 
        }
        String checkSum = "";
        while(check != '|' && (time >= timeEval) && !((loopTime-loopTimeCounter) > looplength)){
          timeEval = millis();
          if (Serial.available()){
            check = Serial.read();
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
        while (Serial.available() && (time >= timeEval) && !((loopTime-loopTimeCounter) > looplength)){
          Serial.read();
          timeEval = millis();
        }
      }
    }
  }
}