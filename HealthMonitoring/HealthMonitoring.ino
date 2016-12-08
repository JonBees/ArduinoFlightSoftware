/* 
 *Health Monitoring Sketch
 *
 * Inputs:
 * Pressure Transducers - analog input pins 0 - 6
 * Thermocouple - digital pins 24,26,27,28,29,50,52
 * Power Relay - digital pins 8,10
 *
 * Output:
 * Xbee Radio - Serial1
 * Micro SD Card - 52 to CLK
 *                 50 to MISO
 *                 51 to MOSI
 *                 53 to CS
 *
 * Tank-iso = 12
 * Servo Dump Valve = 11
 * Fail Closed Valve = 45
 * Fail Open Valve = 44
 */
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <WString.h>
#include <ctype.h>
#include <EEPROM.h>
#include <MAX31855.h>
#include <Servo.h>
#include <stdio.h>

/* Pinouts */
static const int AV5_M = 13;
static const int AV6_M = 11;
static const int FC_U = 45;
static const int FO_U = 44;
static const int FC2 = 42;
static const int FC3 = 43;
static const int POWERBOX_FAN_DIGITAL = 7;

static const int SD_CHIP_SELECT = 53; /* needs to be set to LOW when the SD card is being used and HIGH otherwise. */
static const int POWER_RELAY_DIGITAL = 8;

static const int PT4_M = 0;
static const int PT1_M = 1;
static const int PT3_M = 2;
static const int PT1_U = 3;
static const int PT2_U = 4;
static const int PT2_M = 5;
static const int PT5_M = 6;

/* other sensors */
static const int SENSOR9_ANALOG = 9;
static const int SENSOR8_ANALOG = 10;
static const int SENSOR11_ANALOG = 11;
static const int SENSOR10_ANALOG = 12;

static const int POWER_ERROR_DIGITAL = 40;

/* thermocouple */
static const int THERMOCOUPLE1_CHIP_SELECT = 24; /* needs to be set to LOW when the Thermocouple shield is being used and HIGH otherwise. */
MAX31855 TC(THERMOCOUPLE1_CHIP_SELECT);
static const int THERMOCOUPLE2_CHIP_SELECT = 25;

/* Max Pressure Values */
static const int SOFTKILL_MAX_PRESSURE = 900;
static const int SOFTKILL_PT1_U_MAX_PRESSURE = 10000;
static const int ABORT_MAX_PRESSURE = 1000;
static const int ABORT_PT1_U_MAX_PRESSURE = 10000;
static const int MAX_PRESSURE_OVERAGES = 5;

/* Max Temperature Values
 * Celsius Values
 * double SOFTKILL_MAX_TEMPERATURE[] = {
 * 65.5,1093,65.5,1093,1093,1093}; */
/* Fahrenheit Values for TCpw-1,2,3,4, TCp-1,2 */
static const double SOFTKILL_TCpw_MAX_TEMP = 1400.0;
static const double SOFTKILL_TCp_MAX_TEMP = 140.0;
static const double ABORT_TCpw_MAX_TEMP = 14000;
static const double ABORT_TCp_MAX_TEMP = 200;
static const int MAX_TEMPERATURE_OVERAGES = 5;

static const int CURRENT_SENSOR_ANALOG = 7;
static const int VOLTAGE_SENSOR_ANALOG = 8;
static const int ABSMIN_VOLTAGE = 758; /*  14 V */
static const int MIN_VOLTAGE = 786; /* 14.5 V */
static const int MAX_VOLTAGE = 924; /* 17 V */
static const int MIN_VOLTAGE_SOFTKILL = 25; /* 25 cycles -- 5s */
static const int ABSMIN_VOLTAGE_ABORT = 5;
static const int MAX_VOLTAGE_ABORT = 5; /* 5 cycles -- 1s */

/* Xbee input flags */
struct Flags {
    bool safety : 1;  /* When true, disallow takeoff */
    bool AV5_M_open : 1;  /* Isolation valve */
    bool AV6_M_open : 1;  /* Fuel load valve */
    bool abort : 1;  /* If true, abort */
    bool flight_computer_on : 1;  /* When true, turn on power relay */
    bool flight_computer_reset : 1;  /* When true, reset flight computer */
    bool FO_U_dump : 1;  /* When true, opens the valve to dump ullage */ 
    bool take_off : 1;  /* When true, (and safety is false) begins takeoff */
    bool soft_kill : 1;  /* True starts soft kill procedure */
    bool FC_U_open : 1;  /* When true, opens valve to the ullage tank */
    bool profile_check : 1;
    bool fuel_load_open : 1; /* when true, sets av5 open, av6 closed, fo-u closed, fc-u open, av1-4 50% */
    bool fan_off : 1; /* when true, overrides fan control and forces fan off */
    bool fp_0 : 1; /* fp_0-9 are the flight profile selctors */
    bool fp_1 : 1;
    bool fp_2 : 1;
    bool fp_3 : 1;
    bool fp_4 : 1;
    bool fp_5 : 1;
    bool fp_6 : 1;
    bool fp_7 : 1;
    bool fp_8 : 1;
    bool fp_9 : 1;
  
    Flags() {
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

struct Errors {
    bool temperature_softkill : 1;
    bool temperature_abort : 1;
    bool pressure_softkill : 1;
    bool pressure_abort : 1;
    bool voltage_softkill : 1;
    bool voltage_abort : 1;
    bool time : 1;
    bool fc_time : 1;
    bool power : 1;
};

/* Data structure for XBee health packet */
struct HealthPacket {
    int pressure_values[7];
    double temp_values[6];
    int voltage;
    int current;
    unsigned int motor_values[4];
    unsigned long elapsed;
    Flags state;
    Flags state_activated;
    Errors errorflags;
    String state_string;
};

void check_motors(HealthPacket& data);
String create_health_packet(HealthPacket& data);
void evaluate_error_flag(bool cond, char killtype, char flag, String message, char state_string_id);
void error_flags_evaluation(HealthPacket& data);
void add_flag_to_string(HealthPacket& data, char flag);
void state_function_evaluation(HealthPacket& data);
void reset_state();
void read_flags();
void check_voltage(HealthPacket& data);
void set_flight_profile(HealthPacket& data);
void state_evaluation(HealthPacket& data);
void reset_error_flags(HealthPacket& data);
void read_pressure_transducers(HealthPacket& data);
void read_voltage(HealthPacket& data);
void check_current(HealthPacket& data);
void read_thermocouples(HealthPacket& data);
void send_health_packet(String& str);
void write_to_sdcard(String& str);
void check_valve_status(HealthPacket& data);

int G_SOFTKILL_PRESSURE_OVERAGES[] = {
    0, 0, 0, 0, 0, 0, 0
};
int G_ABORT_PRESSURE_OVERAGES[] = {
    0, 0, 0, 0, 0, 0, 0
};
int G_SOFTKILL_TEMPERATURE_OVERAGES[] = {
    0, 0, 0, 0, 0, 0
};
int G_ABORT_TEMPERATURE_OVERAGES[] = {
    0, 0, 0, 0, 0, 0
};
int G_UNDERVOLTAGES_SOFTKILL = 0;
int G_OVERVOLTAGES_ABORT = 0;
int G_UNDERVOLTAGES_ABORT = 0;

Servo G_AV5_M_SERVO;
Servo G_AV6_M_SERVO;

File G_RECORDING_FILE;

HealthPacket G_CURRENT_HEALTH_PACKET;
String G_LAST_STATE_STRING = "";

int G_THERMO_COUNTER = 1; /* we start at thermocouple 2 (zero indexed) */

bool G_WRITE_SUCCESS = false;
bool G_RELAY_TRIGGERED = false;
bool G_FC_COMMAND_SENT = false;

long G_LAST_FLAG_READ_TIME = 0;
long G_LAST_FC_COMM = 0;
long G_CURRENT_TIME = 0;
long G_LOOP_START_TIME = 0;
long G_LOOP_END_TIME = 0;
int G_LOOP_LENGTH = 200;

int G_DUMP_TIMER = 0;
int G_LOOPS = 0;
int G_VOLTAGE_ABORT_LOOPS = 0;

void setup()
{
    Serial.begin(9600);  /* USB connection */
    Serial1.begin(9600); /* connection with XBee */
    Serial2.begin(9600); /* connection with FlightComputer */
    Serial3.begin(9600); /* currently disconnected */
    reset_state();       /* reset bool states */
    
    /* Motors/Valves init */
    G_AV5_M_SERVO.attach(AV5_M);
    G_AV6_M_SERVO.attach(AV6_M);
    pinMode(POWER_RELAY_DIGITAL, OUTPUT);

    pinMode(THERMOCOUPLE2_CHIP_SELECT, OUTPUT);
    digitalWrite(THERMOCOUPLE2_CHIP_SELECT, HIGH);
    pinMode(FC_U, OUTPUT);
    pinMode(FO_U, OUTPUT);

    G_AV5_M_SERVO.writeMicroseconds(1000);
    G_AV6_M_SERVO.writeMicroseconds(1000);
    digitalWrite(POWER_RELAY_DIGITAL, LOW);
    delay(100);

    /* temperature shield init. */
    TC.begin();
    TC.setMUX(G_THERMO_COUNTER); /* call the thermocouple in the start up since this is going to be evaulated first loop. */
    delayMicroseconds(50);
    digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);
    for (int i = 0; i < 6; i++) {
        G_CURRENT_HEALTH_PACKET.temp_values[i] = 0.00;
    }

    pinMode(POWER_ERROR_DIGITAL, INPUT);

    /* SD card initiation */
    pinMode(SD_CHIP_SELECT, OUTPUT);

    digitalWrite(SD_CHIP_SELECT, LOW);
    /* SD card feedback - apparently necessary for initialization */
    if (!SD.begin(SD_CHIP_SELECT)) {
        Serial.println("initialization failed!");
        digitalWrite(SD_CHIP_SELECT, HIGH);
        return;
    } else {
        Serial.println("initialized");
    }

    /* removes existing file and creates a new blank one */
    if (SD.exists("datalog.txt")) {
        SD.remove("datalog.txt");
        Serial.println("file deleted");
    }
    G_RECORDING_FILE = SD.open("datalog.txt");
    G_RECORDING_FILE.close();

    G_WRITE_SUCCESS = true;
    Serial.println("file created");
    digitalWrite(SD_CHIP_SELECT,HIGH);
}

void loop()
{
    G_LOOP_START_TIME = millis();
    G_LOOP_END_TIME = G_LOOP_START_TIME + G_LOOP_LENGTH;

    G_FC_COMMAND_SENT = false;
  
    /* reads characters from XBee(Serial1) and overwrites previous state_string */
    read_flags(); 
    G_CURRENT_TIME = millis();

    reset_error_flags(G_CURRENT_HEALTH_PACKET);

    /* if no read check time 1m 30 sec handshake process has failed. Turn on appropriate abort based on last health packet */
    if (G_LAST_FLAG_READ_TIME < (G_CURRENT_TIME - 90000)) {
        G_CURRENT_HEALTH_PACKET.errorflags.time = true;
    }
    
    if(!G_CURRENT_HEALTH_PACKET.state.flight_computer_on) {
        G_LAST_FC_COMM = millis();
    }
    
    if (G_CURRENT_HEALTH_PACKET.state.flight_computer_on && (G_CURRENT_TIME - G_LAST_FC_COMM) > 30000) {
        G_CURRENT_HEALTH_PACKET.errorflags.fc_time = true;
    }

    if(digitalRead(POWER_ERROR_DIGITAL) == HIGH) {
        G_CURRENT_HEALTH_PACKET.errorflags.power = true;
    }

    read_thermocouples(G_CURRENT_HEALTH_PACKET);
    read_pressure_transducers(G_CURRENT_HEALTH_PACKET);
    check_voltage(G_CURRENT_HEALTH_PACKET);
    check_current(G_CURRENT_HEALTH_PACKET);
    
    /* checks for soft/hardkills, adds an error flag to the statestring if any are enabled */
    error_flags_evaluation(G_CURRENT_HEALTH_PACKET);
    
    check_motors(G_CURRENT_HEALTH_PACKET);

    if (!G_CURRENT_HEALTH_PACKET.state_string.equals(G_LAST_STATE_STRING)) {
        state_evaluation(G_CURRENT_HEALTH_PACKET);
        set_flight_profile(G_CURRENT_HEALTH_PACKET);
        state_function_evaluation(G_CURRENT_HEALTH_PACKET);
    }

    if (G_CURRENT_HEALTH_PACKET.state.fuel_load_open && !G_CURRENT_HEALTH_PACKET.state.soft_kill) {
        G_DUMP_TIMER++;
        Serial2.write('b');
        G_FC_COMMAND_SENT = true;
    } else {
        G_DUMP_TIMER = 0;
    }
    
    if (G_DUMP_TIMER > 0) {
        if (G_DUMP_TIMER == 1) {
            G_CURRENT_HEALTH_PACKET.state.AV5_M_open = true;
            if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('t') == -1) {
                G_CURRENT_HEALTH_PACKET.state_string += String('t');
            }
        }
        if (G_DUMP_TIMER == 7) {
            G_CURRENT_HEALTH_PACKET.state.AV6_M_open = false;
            if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('v') != -1) {
                G_CURRENT_HEALTH_PACKET.state_string.remove(G_CURRENT_HEALTH_PACKET.state_string.indexOf('v'), 1);
            }
        }
        if (G_DUMP_TIMER == 13) {
            G_CURRENT_HEALTH_PACKET.state.FO_U_dump = false;
            if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('d') == -1) {
                G_CURRENT_HEALTH_PACKET.state_string += String('d');
            }
            G_CURRENT_HEALTH_PACKET.state.FC_U_open = true;
            if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('i') == -1) {
                G_CURRENT_HEALTH_PACKET.state_string += String('i');
            }
        }
    }

    /* if abort, change valve states */
    if (G_CURRENT_HEALTH_PACKET.state.abort) {
        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('a') == -1) {
            G_CURRENT_HEALTH_PACKET.state_string += String('a');
        }
    
        G_CURRENT_HEALTH_PACKET.state.AV5_M_open = true;
    
        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('t') != -1) {
            G_CURRENT_HEALTH_PACKET.state_string.remove(G_CURRENT_HEALTH_PACKET.state_string.indexOf('t'), 1);
        }
    
        G_CURRENT_HEALTH_PACKET.state.FO_U_dump = true;
    
        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('d') != -1) {
            G_CURRENT_HEALTH_PACKET.state_string.remove(G_CURRENT_HEALTH_PACKET.state_string.indexOf('d'), 1);
        }
    
        G_CURRENT_HEALTH_PACKET.state.FC_U_open = false;
    
        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('i') != -1) {
            G_CURRENT_HEALTH_PACKET.state_string.remove(G_CURRENT_HEALTH_PACKET.state_string.indexOf('i'), 1);
        }

        Serial2.write('a');
        G_FC_COMMAND_SENT = true;
    
    } else if(G_CURRENT_HEALTH_PACKET.state.soft_kill) {
        Serial.println(" Softkill ");

        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf('k') == -1) {
            G_CURRENT_HEALTH_PACKET.state_string += String('k');
        }

        Serial2.write('k');
        G_FC_COMMAND_SENT = true;
    }

    if (!G_FC_COMMAND_SENT) {
        Serial2.write('-');
    }

    check_valve_status(G_CURRENT_HEALTH_PACKET);

    String outgoing_packet = create_health_packet(G_CURRENT_HEALTH_PACKET);
    write_to_sdcard(outgoing_packet);
    send_health_packet(outgoing_packet);

    while (G_CURRENT_TIME < G_LOOP_END_TIME) {
        G_CURRENT_TIME = millis();
    }

    /* this resets the thermocouple shield to be called again. allowing us to run the program while the thermocouple shield computes the data. */
    digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);
    G_CURRENT_TIME = millis();
    Serial.println(G_CURRENT_TIME - G_LOOP_START_TIME);
    G_LOOPS++;
}

void check_valve_status(HealthPacket& data)
{
    if (data.state.AV5_M_open) {
        /* Isolation valve */
        G_AV5_M_SERVO.writeMicroseconds(2000);
    } else {
        G_AV5_M_SERVO.writeMicroseconds(1000);
    }
    
    if (data.state.AV6_M_open) {
        /* Fuel load valve */
        G_AV6_M_SERVO.writeMicroseconds(2000);
    } else {
        G_AV6_M_SERVO.writeMicroseconds(1000);
    }
    
    if (data.state.FC_U_open) { /* FC_U is closed when it has no power */
        digitalWrite(FC_U, HIGH);
    } else {
        digitalWrite(FC_U, LOW);
    }
    
    if (data.state.FO_U_dump) { /* FO_U is open when it has no power */
        digitalWrite(FO_U, LOW);
    } else {
        digitalWrite(FO_U, HIGH);
    }
}

void check_motors(HealthPacket& data)
{
    const int LOOPMAX = 14; 
    const int FAILED_VAL = 1000;
    const char ASCII_ZERO = 48;

    char c = 0;
    bool failed = false;
    int loop_counter_check_motor = 0;
    G_LAST_FC_COMM = millis();
    
    while (Serial2.available() && loop_counter_check_motor < LOOPMAX && c != 'm') {
        loop_counter_check_motor++;
        c = Serial2.read();
    }
    
    if (c == 'm' && Serial2.available() && (c = Serial2.read()) == ':') {
        for (int i = 0; i < 4; i++) {
            int j = 0;
            int numbers[] = {
                0, 0, 0, 0
            };
            do {
                if (Serial2.available()) {
                    c = Serial2.read();
                    numbers[j] = c - ASCII_ZERO;
                }
                if (c == 'm') {
                    failed = true;
                    for (int k = 0; k < 4; k++) {
                        data.motor_values[k] = FAILED_VAL;
                    }
                }
                j++;
            } while (!failed && j < 4);
            data.motor_values[i] = numbers[0] * 1000 + numbers[1] * 100 + numbers[2] * 10 + numbers[3]; /* add the tousands/hundreds/tens/ones places */
        }
    }
    
    if (failed) {
        for (int k = 0; k < 4; k++) {
            data.motor_values[k] = FAILED_VAL;
        }
    }
}

void evaluate_error_flag(bool cond, char killtype, char flag, String message, char state_string_id)
{
    if (cond) {
        add_flag_to_string(G_CURRENT_HEALTH_PACKET, flag);
        switch (killtype) {
        case 's':
            G_CURRENT_HEALTH_PACKET.state.soft_kill = true;
            break;
        case 'a':
            G_CURRENT_HEALTH_PACKET.state.abort = true;
            break;
        }
                
        Serial.println(message);
        if (G_CURRENT_HEALTH_PACKET.state_string.indexOf(state_string_id) == -1) {
            G_CURRENT_HEALTH_PACKET.state_string += String(state_string_id);
        }
    }
}

void error_flags_evaluation(HealthPacket& data)
{
    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.temperature_softkill, 's', 'u', " Temperature Softkill ", 'k');
    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.temperature_abort, 'a', 'u', "Temperature Abort ", 'a');

    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.pressure_softkill, 's', 'n', "Pressure Softkill ", 'k');
    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.pressure_abort, 'a', 'n', " Pressure Abort ", 'a');

    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.voltage_abort, 'a', 'g', " Voltage Abort ", 'a');
    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.voltage_softkill, 's', 'g', " Voltage Softkill ", 'k');

    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.time, 'a', 'x', " Time Abort ", 'a');
    evaluate_error_flag(G_CURRENT_HEALTH_PACKET.errorflags.fc_time, 'a', 'q', " Arduino Communication Abort ", 'a');
    
    if(G_CURRENT_HEALTH_PACKET.errorflags.power){
        add_flag_to_string(G_CURRENT_HEALTH_PACKET, 'j');
        Serial.println(" Power Error ");
    }
    if(!G_WRITE_SUCCESS){
        add_flag_to_string(G_CURRENT_HEALTH_PACKET, 'w');
        Serial.println(" Failed to write to SD ");
    }
}

void add_flag_to_string(HealthPacket& data, char flagchar)
{
    if(data.state_string.indexOf(flagchar) == -1){
        data.state_string += String(flagchar);
    }
}

void state_function_evaluation(HealthPacket& data)
{
    if (data.state.abort) {
        Serial2.write('a');
        G_FC_COMMAND_SENT = true;
    }
    if ((data.state.soft_kill) && !data.state_activated.abort) {
        /* True starts soft kill procedure */
        Serial2.write('k');
        G_FC_COMMAND_SENT = true;
    }
    if (data.state.profile_check && !data.state_activated.abort) {
        Serial2.write('p');
        G_FC_COMMAND_SENT = true;
    }
    /* if (data.state.fuel_load_open && G_DUMP_TIMER == 10) { */
    if (data.state.fuel_load_open) {
        Serial.write('b');
        Serial2.write('b');
        G_FC_COMMAND_SENT = true;
    }
    /* ensures take_off is not enabled during an abort */
    if (data.state.take_off && !data.state_activated.safety && !data.state_activated.abort && !data.state_activated.soft_kill) {
        /* Take off on true */
        Serial.write('o');
        Serial2.write('o');
        G_FC_COMMAND_SENT = true;
        data.state_activated.take_off = data.state.take_off;
    }

    /* Flight computer state adjustments */
    if (data.state.flight_computer_reset && !data.state_activated.abort && !data.state_activated.safety) {
        /* On change, will reset computer */
        Serial.write('r');
        Serial2.write('r');
        G_FC_COMMAND_SENT = true;
        G_DUMP_TIMER = 0;
    }
    /* send 1ms pulse to relay when packet sets FC to on but relay isn't triggerd */
    if (data.state.flight_computer_on && !data.state_activated.safety && !G_RELAY_TRIGGERED) { 
        flip_relay();
        G_RELAY_TRIGGERED = true;
    } else if (!data.state.flight_computer_on && G_RELAY_TRIGGERED){ /* turn off flight computer if the packet says to but it's still on */
        flip_relay();
        G_RELAY_TRIGGERED = false;
    } else { /* make sure the relay doesn't receive a flip command */
        digitalWrite(POWER_RELAY_DIGITAL, LOW);
    }
    /* store currently used state */
    data.state_activated = data.state;
}

void state_evaluation(HealthPacket& data)
{
    String temp_string = data.state_string;
    int string_length = temp_string.length();
    reset_state();
    for (int i = 0; i < string_length; i++) {
        char ramp = temp_string[i];
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
    G_LAST_STATE_STRING = data.state_string;
}

void reset_state()
{
    Flags temp_flags;
    G_CURRENT_HEALTH_PACKET.state = temp_flags;
}

void reset_error_flags(HealthPacket& data)
{
    data.errorflags.temperature_softkill = false;
    data.errorflags.temperature_abort = false;
    data.errorflags.pressure_softkill = false;
    data.errorflags.pressure_abort = false;
    data.errorflags.voltage_softkill = false;
    data.errorflags.voltage_abort = false;
    data.errorflags.time = false;
    data.errorflags.power = false;
}

void read_pressure_transducers(HealthPacket& data) {
    data.pressure_values[0] = analogRead(PT4_M);
    data.pressure_values[1] = analogRead(PT1_M);
    data.pressure_values[2] = analogRead(PT3_M);
    data.pressure_values[3] = analogRead(PT1_U);
    data.pressure_values[4] = analogRead(PT2_U);
    data.pressure_values[5] = analogRead(PT2_M);
    data.pressure_values[6] = analogRead(PT5_M);

    for (int i = 0; i < 7; i++) {
        int abort_max_pressure = ABORT_MAX_PRESSURE;
        int softkill_max_pressure = SOFTKILL_MAX_PRESSURE;
        if (3 == i) {
            abort_max_pressure = ABORT_PT1_U_MAX_PRESSURE;
            softkill_max_pressure = SOFTKILL_PT1_U_MAX_PRESSURE;
        }
        
        if (data.pressure_values[i] > (abort_max_pressure * 1.27 - 250)) {
            G_ABORT_PRESSURE_OVERAGES[i]++;
        } else if (data.pressure_values[i] > (softkill_max_pressure * 1.27 - 250)) {
            G_SOFTKILL_PRESSURE_OVERAGES[i]++;
        } else {
            G_SOFTKILL_PRESSURE_OVERAGES[i] = 0;
            G_ABORT_PRESSURE_OVERAGES[i] = 0;
        }

        if (G_ABORT_PRESSURE_OVERAGES[i] >= MAX_PRESSURE_OVERAGES) {
            data.errorflags.pressure_abort = true;
        } else if (G_SOFTKILL_PRESSURE_OVERAGES[i] >= MAX_PRESSURE_OVERAGES) {
            data.errorflags.pressure_softkill = true;
        }
    }
}

void check_voltage(HealthPacket& data) {
    data.voltage = analogRead(VOLTAGE_SENSOR_ANALOG);
    if (data.voltage < MIN_VOLTAGE) {
        G_UNDERVOLTAGES_SOFTKILL++;
    } else {
        G_UNDERVOLTAGES_SOFTKILL = 0;
    }
    
    if (data.voltage > MAX_VOLTAGE) {
        G_OVERVOLTAGES_ABORT++;
    } else {
        G_OVERVOLTAGES_ABORT = 0;
        G_VOLTAGE_ABORT_LOOPS = 0;
    }
    
    if (data.voltage < ABSMIN_VOLTAGE) {
        G_UNDERVOLTAGES_ABORT++;
    } else {
        G_UNDERVOLTAGES_ABORT = 0;
        G_VOLTAGE_ABORT_LOOPS = 0;
    }

    if (G_UNDERVOLTAGES_SOFTKILL >= MIN_VOLTAGE_SOFTKILL) {
        data.errorflags.voltage_softkill = true;
    }
    
    if (G_OVERVOLTAGES_ABORT >= MAX_VOLTAGE_ABORT) {
        data.errorflags.voltage_abort = true;
        G_VOLTAGE_ABORT_LOOPS++;
    }
    
    if (G_UNDERVOLTAGES_ABORT >= ABSMIN_VOLTAGE_ABORT) {
        data.errorflags.voltage_abort = true;
        G_VOLTAGE_ABORT_LOOPS++;
    }

    if (G_VOLTAGE_ABORT_LOOPS > 15 && G_RELAY_TRIGGERED) {
        flip_relay();
        G_RELAY_TRIGGERED = false;
    }
}

void check_current(HealthPacket& data)
{
    data.current = analogRead(CURRENT_SENSOR_ANALOG);
}

double read_thermocouple(int index, byte& error)
{
    double result, dummy;
    /* ExternalfTempDoubleVariable, InternalTempDoubleVariable, SCALE, ErrorByteVariable) --- 
     * SCALE: 0 for Celsius/Centigrade, 1 for Kelvin, 2 for Fahrenheit, and 3 for Rankine. */
    TC.getTemp(result, dummy, 2, error); 
    if (error & 0x01) {
        result = -1.0;
    } else if (error & 0x02) {
        result = -2.0;
    } else if (error & 0x04) {
        result = -3.0;
    }
    /* TODO: Verify that thees are the only possibilities and that else clause is not required. */
    return result;
}

void read_thermocouples(HealthPacket& data)
{
    /* disables connection to SD card while we're reading TC values */
    digitalWrite(SD_CHIP_SELECT, HIGH);
    /* enables connection from TC board */
    digitalWrite(THERMOCOUPLE1_CHIP_SELECT, LOW);

    byte dummy;
    int thermo_counter_temp = G_THERMO_COUNTER - 1;
    if (G_THERMO_COUNTER == 7) {
        G_THERMO_COUNTER = 1;
    } else {
        int abort_max_temp = ABORT_TCpw_MAX_TEMP;
        int softkill_max_temp = SOFTKILL_TCpw_MAX_TEMP;
        if (thermo_counter_temp > 3) {
            abort_max_temp = ABORT_TCp_MAX_TEMP;
            softkill_max_temp = SOFTKILL_TCp_MAX_TEMP;
        }
        
        data.temp_values[thermo_counter_temp] = read_thermocouple(G_THERMO_COUNTER, dummy);
        if (data.temp_values[thermo_counter_temp] > abort_max_temp) {
            G_ABORT_TEMPERATURE_OVERAGES[thermo_counter_temp]++;
        }
        else if (data.temp_values[thermo_counter_temp] > softkill_max_temp) {
            G_SOFTKILL_TEMPERATURE_OVERAGES[thermo_counter_temp]++;
        }
        else {
            G_SOFTKILL_TEMPERATURE_OVERAGES[thermo_counter_temp] = 0;
            G_ABORT_TEMPERATURE_OVERAGES[thermo_counter_temp] = 0;
        }
        for (int i = 0; i < 6; i++) {
            if (G_ABORT_TEMPERATURE_OVERAGES[i] >= MAX_TEMPERATURE_OVERAGES) {
                data.errorflags.temperature_abort = true;
            }
            else if (G_SOFTKILL_TEMPERATURE_OVERAGES[i] >= MAX_TEMPERATURE_OVERAGES) {
                data.errorflags.temperature_softkill = true;
                if (i >= 4) {
                    data.state.FO_U_dump = true;
                    if (data.state_string.indexOf('d') != -1) {
                        data.state_string.remove(data.state_string.indexOf('d'), 1);
                    }
                    data.state.FC_U_open = false;
                    if (data.state_string.indexOf('i') != -1) {
                        data.state_string.remove(data.state_string.indexOf('i'), 1);
                    }
                }
            }
        }

        G_THERMO_COUNTER++;
    }
    
    TC.setMUX(G_THERMO_COUNTER);
    digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);//disables connection from TC board
}

void flip_relay()
{
    digitalWrite(POWER_RELAY_DIGITAL, HIGH);
    delay(1);
    digitalWrite(POWER_RELAY_DIGITAL, LOW);
}

void set_flight_profile(HealthPacket& data)
{
    if (data.state.fp_0) {
        Serial2.write('0');
        Serial.println("0");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_1) {
        Serial2.write('1');
        Serial.println("1");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_2) {
        Serial2.write('2');
        Serial.println("2");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_3) {
        Serial2.write('3');
        Serial.println("3");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_4) {
        Serial2.write('4');
        Serial.println("4");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_5) {
        Serial2.write('5');
        Serial.println("5");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_6) {
        Serial2.write('6');
        Serial.println("6");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_7) {
        Serial2.write('7');
        Serial.println("7");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_8) {
        Serial2.write('8');
        Serial.println("8");
        G_FC_COMMAND_SENT = true;
    } else if (data.state.fp_9) {
        Serial2.write('9');
        Serial.println("9");
        G_FC_COMMAND_SENT = true;
    }
}

String create_health_packet(HealthPacket& data)
{
    String outgoingPacket = "p:";
    
    for (int i = 0; i < 6; i++) {
        outgoingPacket += String(data.pressure_values[i]);
        outgoingPacket += String(",");
    }
    
    outgoingPacket += String(data.pressure_values[6]);
    outgoingPacket += String(";t:");

    for (int i = 0; i < 5; i++) {
        outgoingPacket += String((int)data.temp_values[i]);
        outgoingPacket += String(",");
    }
    outgoingPacket += String((int)data.temp_values[5]);
    
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
    outgoingPacket += String(data.state_string);
    
    outgoingPacket += String(";?");
    int packetLength = outgoingPacket.length() - 1;
    outgoingPacket += String(packetLength);
    outgoingPacket += String("|");
    
    return outgoingPacket;
}

void send_health_packet(String& str)
{
    Serial1.println(str);
    Serial.println(str);
}

void write_to_sdcard(String& str)
{
    digitalWrite(THERMOCOUPLE1_CHIP_SELECT, HIGH);
    digitalWrite(SD_CHIP_SELECT, LOW);
    File G_RECORDING_FILE = SD.open("datalog.txt", FILE_WRITE);
    delay(1);
    if (G_RECORDING_FILE) { /* what the fuck? */
        G_RECORDING_FILE.print(G_LOOP_START_TIME);
        G_RECORDING_FILE.print("|");
        G_RECORDING_FILE.println(str);
        G_RECORDING_FILE.close();
    }
    digitalWrite(SD_CHIP_SELECT, HIGH);
}

void read_flags()
{
    long time_eval = millis();
    long time = time_eval + G_LOOP_LENGTH;
    bool packet_received = false;
    while (!packet_received && (time >= time_eval)) {
        time_eval = millis();
        char check;

        if (Serial1.available()) {
            check = Serial1.read();
        }

        if (check != 'h' || (Serial.read()) != ':') {
            continue;
        }

        String incoming_health_packet = "";
        while (check != ';' && Serial1.available() && (time >= time_eval)  && !((G_LOOP_START_TIME - G_LOOP_END_TIME) > G_LOOP_LENGTH)) {
            time_eval = millis();
            check = Serial1.read();
            if (check != ';') {
                incoming_health_packet += check;
            }
        }
        
        String checksum = "";
        while (check != '|' &&
               (time >= time_eval) &&
               !((G_LOOP_START_TIME - G_LOOP_END_TIME) > G_LOOP_LENGTH)) {
                
            time_eval = millis();
            
            if (Serial1.available()) {
                check = Serial1.read();
                if (check != '|') {
                    checksum += check;
                }
            }
        }
                
        int checksum_int = checksum.toInt();
        if (checksum_int == incoming_health_packet.length()) {
            G_CURRENT_HEALTH_PACKET.state_string = incoming_health_packet;
            G_LAST_FLAG_READ_TIME = millis();
            packet_received = true;
        }
        while (Serial1.available() && (time >= time_eval) && !((G_LOOP_START_TIME - G_LOOP_END_TIME) > G_LOOP_LENGTH)) {
            Serial1.read();
            time_eval = millis();
        }
    }
}
