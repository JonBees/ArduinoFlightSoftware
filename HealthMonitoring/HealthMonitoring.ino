/*
  Health Monitoring Sketch

  Inputs:
  Pressure Transducers - analog input pins 0 - 6
  Thermocouple - digital pins 24,26,27,28,29,50,52
  Power Relay - digital pins 8,10

  Output:
  Xbee Radio - Serial1
  Micro SD Card - 52 to CLK
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
#define AV5_M 13
#define AV6_M 11
#define FC_U 45
#define FO_U 44
#define FC2 42
#define FC3 43
#define POWERBOX_FAN_DIGITAL 7

//Servo
Servo AV5_M_servo;
Servo AV6_M_servo;

//SD Card
File recordingFile;
#define SD_CHIP_SELECT 53 //needs to be set to LOW when the SD card is being used and HIGH otherwise.

//Power Relay
#define POWER_RELAY_DIGITAL 8

//Pressure Transducers
#define PT4_M 0
#define PT1_M 1
#define PT3_M 2
#define PT1_U 3
#define PT2_U 4
#define PT2_M 5
#define PT5_M 6

//other sensors
#define sensor9_analog 9
#define sensor8_analog 10
#define sensor11_analog 11
#define sensor10_analog 12

#define POWER_ERROR_DIGITAL 40


//thermocouple
#define THERMOCOUPLE1_CHIP_SELECT 24 //needs to be set to LOW when the Thermocouple shield is being used and HIGH otherwise.
MAX31855 TC(THERMOCOUPLE1_CHIP_SELECT);
#define THERMOCOUPLE2_CHIP_SELECT 25

//Max Pressure Values
int SOFTKILL_MAX_PRESSURE[] = {
  900, 900, 900, 10000, 900, 900, 900
};
int ABORT_MAX_PRESSURE[] = {
  1000, 1000, 1000, 10000, 1000, 1000, 1000
};
int MAX_SOFTKILL_PRESSURE_OVERAGES[] = {
  5, 5, 5, 5, 5, 5, 5
};//5 corresponds to 5 cycles off. 1s
int softkill_pressure_overages[] = {
  0, 0, 0, 0, 0, 0, 0
};
int MAX_ABORT_PRESSURE_OVERAGES[] = {
  5, 5, 5, 5, 5, 5, 5
};
int abort_pressure_overages[] = {
  0, 0, 0, 0, 0, 0, 0
};

//Max Temperature Values
//TCpw-1,2,3,4, TCp-1,2
//Celsius Values
/*double SOFTKILL_MAX_TEMPERATURE[] = {
  65.5,1093,65.5,1093,1093,1093};*/

//Fahrenheit Values
double SOFTKILL_MAX_TEMPERATURE[] = {
  1400, 1400, 1400, 1400, 140, 140
};
double ABORT_MAX_TEMPERATURE[] = {
  14000, 14000, 14000, 14000, 200, 200
};

int MAX_SOFTKILL_TEMPERATURE_OVERAGES[] = {
  5, 5, 5, 5, 5, 5
};//5 corresponds to 5 readings of the thermocouples. 1s
int softkill_temperature_overages[] = {
  0, 0, 0, 0, 0, 0
};
int MAX_ABORT_TEMPERATURE_OVERAGES[] = {
  5, 5, 5, 5, 5, 5
};
int abort_temperature_overages[] = {
  0, 0, 0, 0, 0, 0
};

int thermoCounter = 1;//we start at thermocouple 2 (zero indexed)

double boxTemp = 0;
int boxTempAbort = 0;

#define CURRENT_SENSOR_ANALOG 7


//Voltage Sensor
#define voltage_sensor_analog 8
int ABSMIN_VOLTAGE = 758; // 14 V
int MIN_VOLTAGE = 786; // 14.5 V
int MAX_VOLTAGE = 924; // 17 V
int MIN_VOLTAGE_SOFTKILL = 25; // 25 cycles -- 5s
int ABSMIN_VOLTAGE_ABORT = 5;
int MAX_VOLTAGE_ABORT = 5; // 5 cycles -- 1s
int voltage_softkill_underages = 0;
int voltage_abort_overages = 0;
int voltage_abort_underages = 0;

//Xbee input flags
struct flags
{
bool safety :
  1;  // When true, disallow takeoff
bool AV5_M_open :
  1;  // Isolation valve
bool AV6_M_open :
  1;  // Fuel load valve
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
bool profile_check :
  1;
bool fuel_load_open :
  1; //when true, sets av5 open, av6 closed, fo-u closed, fc-u open, av1-4 50%
bool fan_off :
  1; //when true, overrides fan control and forces fan off
bool fp_0 :
  1;//fp_0-9 are the flight profile selctors
bool fp_1 :
  1;
bool fp_2 :
  1;
bool fp_3 :
  1;
bool fp_4 :
  1;
bool fp_5 :
  1;
bool fp_6 :
  1;
bool fp_7 :
  1;
bool fp_8 :
  1;
bool fp_9 :
  1;

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
    profile_check = false;
    fuel_load_open = false;
    fan_off = false;
    fp_0 = false;
    fp_1 = false;
    fp_2 = false;
    fp_3 = false;
    fp_4 = false;
    fp_5 = false;
    fp_6 = false;
    fp_7 = false;
    fp_8 = false;
    fp_9 = false;
  };
};

struct errors
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
bool fc_time :
  1;
bool power :
  1;
};

bool write_success = false;

// Data structure for XBee health packet
struct health_packet
{
  int pressure_values[7];
  double temp_values[6];
  int voltage;
  int current;
  unsigned int motor_values[4];
  unsigned long elapsed;
  flags state;
  flags state_activated;
  errors errorflags;
  String stateString;
};


template <int N>
class print_err;

boolean relayTriggered = false;
boolean FCCommandSent = false;

long lastFlagReadTime = 0;
long lastFCCommunication = 0;
long currentTime = 0;
long loopStartTime = 0;
long loopEndTime = 0;
int looplength = 200;

int dumpTimer = 0;

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
void readVoltage(health_packet& data);
void checkCurrent(health_packet& data);
void readThermocouples(health_packet& data);
void sendHealthPacket(String& str);
void SDcardWrite(String& str);
void checkValves(health_packet& data);

int loops = 0;
int voltageAbortLoops = 0;

void setup() {
  Serial.begin(9600);//USB connection
  Serial1.begin(9600);//connection with XBee
  Serial2.begin(9600);//connection with FlightComputer
  Serial3.begin(9600);//currently disconnected
  resetState();//reset bool states
  //Motors/Valves init
  AV5_M_servo.attach(AV5_M);
  AV6_M_servo.attach(AV6_M);
  pinMode(POWER_RELAY_DIGITAL, OUTPUT);

  pinMode(THERMOCOUPLE2_CHIP_SELECT, OUTPUT);
  digitalWrite(THERMOCOUPLE2_CHIP_SELECT, HIGH);
  pinMode(FC_U, OUTPUT);
  pinMode(FO_U, OUTPUT);


  AV5_M_servo.writeMicroseconds(1000);
  AV6_M_servo.writeMicroseconds(1000);
  digitalWrite(POWER_RELAY_DIGITAL, LOW);
  delay(100);
  //digitalWrite(FC_U, HIGH); //these two make the solenoids click on powerup to ensure functionality.
  //digitalWrite(FO_U, HIGH);

  //temperature shield init.
  TC.begin();
  TC.setMUX(thermoCounter);//call the thermocouple in the start up since this is going to be evaulated first loop.
  delayMicroseconds(50);
  digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);
  for (int i = 0; i < 6; i++) {
    current_health_packet.temp_values[i] = 0.00;
  }

  pinMode(POWER_ERROR_DIGITAL, INPUT);

  //SD card initiation
  pinMode(SD_CHIP_SELECT, OUTPUT);

  digitalWrite(SD_CHIP_SELECT,LOW);
  //SD card feedback - apparently necessary for initialization
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("initialization failed!");
    digitalWrite(SD_CHIP_SELECT,HIGH);
    return;
  }
  else {
    Serial.println("initialized");
  }

  //removes existing file and creates a new blank one
  if (SD.exists("datalog.txt")) {
    SD.remove("datalog.txt");
    Serial.println("file deleted");
  }
  recordingFile = SD.open("datalog.txt");
  recordingFile.close();

  write_success = true;
  Serial.println("file created");
  digitalWrite(SD_CHIP_SELECT,HIGH);
}


void loop() {
  loopStartTime = millis();
  loopEndTime = loopStartTime + looplength;

  FCCommandSent = false;
  
  //reads characters from XBee(Serial1) and overwrites previous stateString
  readFlags(); 
  currentTime = millis();

  //sets all error flags to false
  resetErrorFlags(current_health_packet);

  //if no read check time 1m 30 sec handshake process has failed. Turn on appropriate abort based on last health packet
  if (lastFlagReadTime < (currentTime - 90000)) {
    current_health_packet.errorflags.time = true;
  }
  if(!current_health_packet.state.flight_computer_on){
    lastFCCommunication = millis();
  }
  if (current_health_packet.state.flight_computer_on && (currentTime - lastFCCommunication) > 30000) {
    current_health_packet.errorflags.fc_time = true;
  }

  if(digitalRead(POWER_ERROR_DIGITAL) == HIGH){
    current_health_packet.errorflags.power = true;
  }

  //calls readThermocouple (which uses the MAX31855 library) on current thermocouple, aborts/softkills if necessary 
  readThermocouples(current_health_packet);
  //analogReads from pressure transducer pins, aborts/softkills if necessary
  readPressureTransducers(current_health_packet);
  //anlogReads from voltage pin, aborts/softkills if necessary
  checkVoltage(current_health_packet);
  checkCurrent(current_health_packet);
  //checks for soft/hardkills, adds an error flag to the statestring if any are enabled
  errorFlagsEvaluation(current_health_packet);
  //sets flight profile based on GCS
  //setFlightProfile(current_health_packet);
  //reads motor values from flight computer(Serial2) and adds them to the health packet
  checkMotors(current_health_packet);
  //Send Health Packet, we don't send flag if time out... this will cause the device to freeze

  //Enables/disables fan based on LabVIEW button state
  //setFanState(current_health_packet);


  if (!current_health_packet.stateString.equals(lastStateString)) {
    stateEvaluation(current_health_packet);
    setFlightProfile(current_health_packet);
    stateFunctionEvaluation(current_health_packet);
  }

  if (current_health_packet.state.fuel_load_open && !current_health_packet.state.soft_kill) {
    dumpTimer++;

    Serial2.write('b');
    FCCommandSent = true;
  }
  else {
    dumpTimer = 0;
  }
  if (dumpTimer > 0) {
    if (dumpTimer == 1) {
      current_health_packet.state.AV5_M_open = true;
      if (current_health_packet.stateString.indexOf('t') == -1) {
        current_health_packet.stateString += String('t');
      }
    }
    if (dumpTimer == 7) {
      current_health_packet.state.AV6_M_open = false;
      if (current_health_packet.stateString.indexOf('v') != -1) {
        current_health_packet.stateString.remove(current_health_packet.stateString.indexOf('v'), 1);
      }
    }
    if (dumpTimer == 13) {
      current_health_packet.state.FO_U_dump = false;
      if (current_health_packet.stateString.indexOf('d') == -1) {
        current_health_packet.stateString += String('d');
      }
      current_health_packet.state.FC_U_open = true;
      if (current_health_packet.stateString.indexOf('i') == -1) {
        current_health_packet.stateString += String('i');
      }
    }
  }

  //if abort, change valve states
  if (current_health_packet.state.abort) {
    if (current_health_packet.stateString.indexOf('a') == -1) {
      current_health_packet.stateString += String('a');
    }

    
    current_health_packet.state.AV5_M_open = true;
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

    Serial2.write('a');
    FCCommandSent = true;
  }
  else if(current_health_packet.state.soft_kill){
    Serial.println(" Softkill ");
    if (current_health_packet.stateString.indexOf('k') == -1) {
      current_health_packet.stateString += String('k');
    }

    Serial2.write('k');
    FCCommandSent = true;
  }


  if (!FCCommandSent) {
    Serial2.write('-');
  }




  checkValves(current_health_packet);


  String outgoingPacket = createHealthPacket(current_health_packet);
  SDcardWrite(outgoingPacket);
  sendHealthPacket(outgoingPacket);


  while (currentTime < loopEndTime) {
    currentTime = millis();
    //delay(1);
  }
  digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);//this resets the thermocouple shield to be called again. allowing us to run the program while the thermocouple shield computes the data.
  //Serial.println(loops);
  currentTime = millis();
  Serial.println(currentTime - loopStartTime);
  loops++;
}

void checkValves(health_packet& data) {
  if (data.state.AV5_M_open) {
    // Isolation valve
    AV5_M_servo.writeMicroseconds(2000);
  }
  else {
    AV5_M_servo.writeMicroseconds(1000);
  }
  if (data.state.AV6_M_open) {
    // Fuel load valve
    AV6_M_servo.writeMicroseconds(2000);
  }
  else {
    AV6_M_servo.writeMicroseconds(1000);
  }
  if (data.state.FC_U_open) { //FC_U is closed when it has no power
    digitalWrite(FC_U, HIGH);
  }
  else {
    digitalWrite(FC_U, LOW);
  }
  if (data.state.FO_U_dump) { //FO_U is open when it has no power
    digitalWrite(FO_U, LOW);
  }
  else {
    digitalWrite(FO_U, HIGH);
  }
}

//get motor functions
void checkMotors(health_packet& data) {
  boolean failed = false;
  boolean done = false;
  int loopCounterCheckMotor = 0;
  if (Serial2.available()) {
    lastFCCommunication = millis();
    while (Serial2.available() && loopCounterCheckMotor < 14 && !failed && !done) {
      //motors are being read
      loopCounterCheckMotor++;
      char c = Serial2.read();
      //Serial.print(c);
      if (c == 'm') {
        if (Serial2.available())
          c = Serial2.read();
        if (c == ':') {
          int numbers[] = {
            0, 0, 0, 0
          };
          for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
              if (Serial2.available()) {
                c = Serial2.read();
                numbers[j] = c - '0';
              }
              if (c == 'm') {
                failed = true;
              }
            }
            //assign reads to four int
            data.motor_values[i] = (numbers[0] * 1000) + (numbers[1] * 100) + (numbers[2] * 10) + (numbers[3]);
            //Serial.println(data.motor_values[i]);
          }
          if (failed) {
            for (int i = 0; i < 4; i++) {
              data.motor_values[i] = 1000;
            }
          }
          done = true;
        }
      }
    }
  }
  else {
    for (int i = 0; i < 4; i++) {
      data.motor_values[i] = 1000;
    }
  }
}
//check current error flags
void errorFlagsEvaluation(health_packet& data) {

  if (current_health_packet.errorflags.temperature_softkill) {
    addFlagToString(current_health_packet, 'u');
    data.state.soft_kill = true;
    Serial.println(" Temperature Softkill ");
    if (data.stateString.indexOf('k') == -1) {
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.temperature_abort) {
    addFlagToString(current_health_packet, 'u');
    data.state.abort = true;
    Serial.println(" Temperature Abort ");
    if (data.stateString.indexOf('a') == -1) {
      data.stateString += String('a');
    }
  }
  if (current_health_packet.errorflags.pressure_softkill) {
    addFlagToString(current_health_packet, 'n');
    data.state.soft_kill = true;
    Serial.println(" Pressure Softkill ");
    if (data.stateString.indexOf('k') == -1) {
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.pressure_abort) {
    addFlagToString(current_health_packet, 'n');
    data.state.abort = true;
    Serial.println(" Pressure Abort ");
    if (data.stateString.indexOf('a') == -1) {
      data.stateString += String('a');
    }
  }
  if (current_health_packet.errorflags.voltage_abort) {
    addFlagToString(current_health_packet, 'g');
    data.state.abort = true;
    Serial.println(" Voltage Abort ");
    if (data.stateString.indexOf('a') == -1) {
      data.stateString += String('a');
    }
  }
  if (current_health_packet.errorflags.voltage_softkill) {
    addFlagToString(current_health_packet, 'g');
    data.state.soft_kill = true;
    Serial.println(" Voltage Softkill ");
    if (data.stateString.indexOf('k') == -1) {
      data.stateString += String('k');
    }
  }
  if (current_health_packet.errorflags.time) {
    addFlagToString(current_health_packet, 'x');
    //timeout abort
    data.state.abort = true;
    Serial.println(" Time Abort ");
    if (data.stateString.indexOf('a') == -1) {
      data.stateString += String('a');
    }
  }
  if(current_health_packet.errorflags.fc_time){
    addFlagToString(current_health_packet, 'q');
    data.state.abort = true;
    Serial.println(" Arduino Communication Abort");
    if (data.stateString.indexOf('a') == -1) {
      data.stateString += String('a');
    }
  }
  if(current_health_packet.errorflags.power){
    addFlagToString(current_health_packet, 'j');
    Serial.println(" Power Error ");
  }
  if(!write_success){
    addFlagToString(current_health_packet, 'w');
    Serial.println(" Failed to write to SD ");
  }
}
void addFlagToString(health_packet& data, char flagchar) {
  if(data.stateString.indexOf(flagchar) == -1){
    data.stateString += String(flagchar);
  }
  /*if (data.stateString.indexOf('e') == -1) {
    data.stateString += String('e');
  }*/
}

void stateFunctionEvaluation(health_packet& data) {

  //flight states
  if (data.state.abort) {
    //abort sequence
    Serial2.write('a');
    FCCommandSent = true;
  }
  if ((data.state.soft_kill) && !data.state_activated.abort) {
    // True starts soft kill procedure
    Serial2.write('k');
    FCCommandSent = true;
  }
  if (data.state.profile_check && !data.state_activated.abort) {
    Serial2.write('p');
    FCCommandSent = true;
  }
  if (data.state.fuel_load_open /*&& dumpTimer == 10*/) {
    Serial.write('b');
    Serial2.write('b');
    FCCommandSent = true;
  }
  if (data.state.take_off && !data.state_activated.safety && !data.state_activated.abort && !data.state_activated.soft_kill) { //ensures take_off is not enabled during an abort
    // Take off on true
    Serial.write('o');
    Serial2.write('o');
    FCCommandSent = true;
    data.state_activated.take_off = data.state.take_off;
  }

  //Flight computer state adjustments
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (data.state.flight_computer_reset && !data.state_activated.abort && !data.state_activated.safety) {
    // On change, will reset computer
    Serial.write('r');
    Serial2.write('r');
    FCCommandSent = true;
    dumpTimer = 0;
  }

  if (data.state.flight_computer_on && !data.state_activated.safety && !relayTriggered) { //send 1ms pulse to relay when packet sets FC to on but relay isn't triggerd
    flipRelay();
    relayTriggered = true;
  }
  else if (!data.state.flight_computer_on && relayTriggered){ //turn off flight computer if the packet says to but it's still on
    flipRelay();
    relayTriggered = false;
  }
  else { //make sure the relay doesn't receive a flip command
    digitalWrite(POWER_RELAY_DIGITAL, LOW);
  }
  //store currently used state
  data.state_activated = data.state;
}


void stateEvaluation(health_packet& data) {
  String tempString = data.stateString;
  int stringLength = tempString.length();
  resetState();
  for (int i = 0; i < stringLength; i++) {
    char ramp = tempString[i];
    switch (ramp) {
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
      case '0':
        data.state.fp_0 = true;
        break;
      case '1':
        data.state.fp_1 = true;
        break;
      case '2':
        data.state.fp_2 = true;
        break;
      case '3':
        data.state.fp_3 = true;
        break;
      case '4':
        data.state.fp_4 = true;
        break;
      case '5':
        data.state.fp_5 = true;
        break;
      case '6':
        data.state.fp_6 = true;
        break;
      case '7':
        data.state.fp_7 = true;
        break;
      case '8':
        data.state.fp_8 = true;
        break;
      case '9':
        data.state.fp_9 = true;
        break;
      case 'k':
        data.state.soft_kill = true;
        break;
      case 'i':
        data.state.FC_U_open = true;
        break;
      case 'p':
        data.state.profile_check = true;
        break;
      case 'b':
        data.state.fuel_load_open = true;
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
void resetErrorFlags(health_packet& data) {
  data.errorflags.temperature_softkill = false;
  data.errorflags.temperature_abort = false;
  data.errorflags.pressure_softkill = false;
  data.errorflags.pressure_abort = false;
  data.errorflags.voltage_softkill = false;
  data.errorflags.voltage_abort = false;
  data.errorflags.time = false;
  data.errorflags.power = false;
}

//PresureTransducerRead
void readPressureTransducers(health_packet& data) {

  data.pressure_values[0] = analogRead(PT4_M);
  data.pressure_values[1] = analogRead(PT1_M);
  data.pressure_values[2] = analogRead(PT3_M);
  data.pressure_values[3] = analogRead(PT1_U);
  data.pressure_values[4] = analogRead(PT2_U);
  data.pressure_values[5] = analogRead (PT2_M);
  data.pressure_values[6] = analogRead (PT5_M);

  for (int i = 0; i < 7; i++) {
    if (data.pressure_values[i] > ((ABORT_MAX_PRESSURE[i] * 1.27) - 250)) {
      abort_pressure_overages[i]++;
    }
    else if (data.pressure_values[i] > ((SOFTKILL_MAX_PRESSURE[i] * 1.27) - 250)) {
      softkill_pressure_overages[i]++;
    }
    else {
      softkill_pressure_overages[i] = 0;
      abort_pressure_overages[i] = 0;
    }

    if (abort_pressure_overages[i] >= MAX_ABORT_PRESSURE_OVERAGES[i]) {
      data.errorflags.pressure_abort = true;
    }
    else if (softkill_pressure_overages[i] >= MAX_SOFTKILL_PRESSURE_OVERAGES[i]) {
      data.errorflags.pressure_softkill = true;
    }
  }
}


//Voltage Sensor Read
void checkVoltage(health_packet& data) {
  data.voltage = analogRead(voltage_sensor_analog);
  if (data.voltage < MIN_VOLTAGE) {
    voltage_softkill_underages++;
  }
  else {
    voltage_softkill_underages = 0;
  }
  if (data.voltage > MAX_VOLTAGE) {
    voltage_abort_overages++;
  }
  else {
    voltage_abort_overages = 0;
    voltageAbortLoops = 0;
  }
  if (data.voltage < ABSMIN_VOLTAGE) {
    voltage_abort_underages++;
  }
  else {
    voltage_abort_underages = 0;
    voltageAbortLoops = 0;
  }


  if (voltage_softkill_underages >= MIN_VOLTAGE_SOFTKILL) {
    data.errorflags.voltage_softkill = true;
  }
  if (voltage_abort_overages >= MAX_VOLTAGE_ABORT) {
    data.errorflags.voltage_abort = true;
    voltageAbortLoops++;
  }
  if (voltage_abort_underages >= ABSMIN_VOLTAGE_ABORT) {
    data.errorflags.voltage_abort = true;
    voltageAbortLoops++;
  }

  if (voltageAbortLoops > 15 && relayTriggered) {
      flipRelay();
      relayTriggered = false;
  }
}

void checkCurrent(health_packet& data){
  data.current = analogRead(CURRENT_SENSOR_ANALOG);
}

//ThermoCouple Read
void initThermocoupleMonitor()
{
  TC.begin();
}

double readThermocouple(int index, byte& error)
{
  double result, dummy;

  TC.getTemp(result, dummy, 2, error); // ExternalfTempDoubleVariable, InternalTempDoubleVariable, SCALE, ErrorByteVariable) --- SCALE: 0 for Celsius/Centigrade, 1 for Kelvin, 2 for Fahrenheit, and 3 for Rankine.
  if (error & 0x01) {
    result = -1.0;
  }
  else if (error & 0x02) {
    result = -2.0;
  }
  else if (error & 0x04) {
    result = -3.0;
  }
  return result;
}

void readThermocouples(health_packet& data)
{
  digitalWrite(SD_CHIP_SELECT, HIGH);//disables connection to SD card while we're reading TC values
  digitalWrite(THERMOCOUPLE1_CHIP_SELECT, LOW);//enables connection from TC board

  byte dummy;
  int thermoCounterTemp = thermoCounter - 1;
  if (thermoCounter == 7) {
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
  else {
    data.temp_values[thermoCounterTemp] = readThermocouple(thermoCounter, dummy);
    if (data.temp_values[thermoCounterTemp] > ABORT_MAX_TEMPERATURE[thermoCounterTemp]) {
      abort_temperature_overages[thermoCounterTemp]++;
    }
    else if (data.temp_values[thermoCounterTemp] > SOFTKILL_MAX_TEMPERATURE[thermoCounterTemp]) {
      softkill_temperature_overages[thermoCounterTemp]++;
    }
    else {
      softkill_temperature_overages[thermoCounterTemp] = 0;
      abort_temperature_overages[thermoCounterTemp] = 0;
    }
    for (int i = 0; i < 6; i++) {
      if (abort_temperature_overages[i] >= MAX_ABORT_TEMPERATURE_OVERAGES[i]) {
        data.errorflags.temperature_abort = true;
      }
      else if (softkill_temperature_overages[i] >= MAX_SOFTKILL_TEMPERATURE_OVERAGES[i]) {
        data.errorflags.temperature_softkill = true;
        if (i >= 4) {
          data.state.FO_U_dump = true;
          if (data.stateString.indexOf('d') != -1) {
            data.stateString.remove(data.stateString.indexOf('d'), 1);
          }
          data.state.FC_U_open = false;
          if (data.stateString.indexOf('i') != -1) {
            data.stateString.remove(data.stateString.indexOf('i'), 1);
          }
        }
      }
    }

    thermoCounter++;
  };
  TC.setMUX(thermoCounter);
  digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);//disables connection from TC board
}

void setFanState(health_packet& data) {
  if (data.state.fan_off) {
    digitalWrite(POWERBOX_FAN_DIGITAL, LOW);
  }
  else {
    digitalWrite(POWERBOX_FAN_DIGITAL, HIGH);
  }
}

void flipRelay(){
  digitalWrite(POWER_RELAY_DIGITAL, HIGH);
  delay(1);
  digitalWrite(POWER_RELAY_DIGITAL, LOW);
}


void setFlightProfile(health_packet& data) {
  if (data.state.fp_0) {
    Serial2.write('0');
    Serial.println("0");
    FCCommandSent = true;
  }
  else if (data.state.fp_1) {
    Serial2.write('1');
    Serial.println("1");
    FCCommandSent = true;
  }
  else if (data.state.fp_2) {
    Serial2.write('2');
    Serial.println("2");
    FCCommandSent = true;
  }
  else if (data.state.fp_3) {
    Serial2.write('3');
    Serial.println("3");
    FCCommandSent = true;
  }
  else if (data.state.fp_4) {
    Serial2.write('4');
    Serial.println("4");
    FCCommandSent = true;
  }
  else if (data.state.fp_5) {
    Serial2.write('5');
    Serial.println("5");
    FCCommandSent = true;
  }
  else if (data.state.fp_6) {
    Serial2.write('6');
    Serial.println("6");
    FCCommandSent = true;
  }
  else if (data.state.fp_7) {
    Serial2.write('7');
    Serial.println("7");
    FCCommandSent = true;
  }
  else if (data.state.fp_8) {
    Serial2.write('8');
    Serial.println("8");
    FCCommandSent = true;
  }
  else if (data.state.fp_9) {
    Serial2.write('9');
    Serial.println("9");
    FCCommandSent = true;
  }
}

String createHealthPacket(health_packet& data)
{
  String outgoingPacket = "p:";
  for (int i = 0; i < 6; i++) {
    outgoingPacket += String(data.pressure_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.pressure_values[6]);
  outgoingPacket += String(";t:");
  //outgoingPacket += String(Serial1.available());
  for (int i = 0; i < 5; i++) {
    outgoingPacket += String(data.temp_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.temp_values[5]);
  outgoingPacket += String(";v:");
  outgoingPacket += String(data.voltage);
  outgoingPacket += String(",");
  outgoingPacket += String(data.current);
  outgoingPacket += String(";m:");
  for (int i = 0; i < 3; i++) {
    outgoingPacket += String(data.motor_values[i]);
    outgoingPacket += String(",");
  }
  outgoingPacket += String(data.motor_values[3]);
  outgoingPacket += String(";h:");
  outgoingPacket += String(data.stateString);
  /*outgoingPacket += String(";s:");
  outgoingPacket += String(loops);*/
  outgoingPacket += String(";?");
  int packetLength = outgoingPacket.length() - 1;
  outgoingPacket += String(packetLength);
  outgoingPacket += String("|");
  return outgoingPacket;
}

void sendHealthPacket(String& str) {
  Serial1.println(str);
  Serial.println(str);
  //Serial.println(Serial1.available());
}

void SDcardWrite(String& str) {
  digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);
  digitalWrite(SD_CHIP_SELECT, LOW);
  File recordingFile = SD.open("datalog.txt", FILE_WRITE);
  delay(1);
  if (recordingFile) {
    recordingFile.print(loopStartTime);
    recordingFile.print("|");
    recordingFile.println(str);
    /*recordingFile.print("b:");
    recordingFile.println(boxTemp);*/
    recordingFile.close();
  }
  digitalWrite(SD_CHIP_SELECT, HIGH);
}



void readFlags()
{
  long timeEval = millis();
  long time = timeEval + looplength;
  boolean packetReceived = false;
  while (!packetReceived && (time >= timeEval)) {
    timeEval = millis();
    char check/* = NULL*/;
    if (Serial1.available()) {
      check = Serial1.read();
    }
    if (check == 'h') {
      check = Serial1.read();
      if (check == ':') {
        //here is our health packet
        String incomingString = "";
        while (check != ';' && Serial1.available() && (time >= timeEval)  && !((loopStartTime - loopEndTime) > looplength)) {
          timeEval = millis();
          check = Serial1.read();
          if (check != ';') {
            incomingString += check;
          }
        }
        String checkSum = "";
        while (check != '|' && (time >= timeEval) && !((loopStartTime - loopEndTime) > looplength)) {
          timeEval = millis();
          if (Serial1.available()) {
            check = Serial1.read();
            if (check != '|') {
              checkSum += check;
            }
          }
        }
        int checkSumInt = checkSum.toInt();
        if (checkSumInt == incomingString.length()) {
          current_health_packet.stateString = incomingString;
          lastFlagReadTime = millis();
          packetReceived = true;
        }
        while (Serial1.available() && (time >= timeEval) && !((loopStartTime - loopEndTime) > looplength)) {
          Serial1.read();
          timeEval = millis();
        }
      }
    }
  }
}

