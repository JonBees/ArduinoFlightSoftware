# ArduinoFlightSoftware
﻿
####This is the repository for the Lunar Lions' Arduino-based Flight Software. Currently, this includes:

* [ComponentTestingPrograms](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/ComponentTestingPrograms)
  * SDCardSpeedTest tests the functionality and speed of the Arduino SD card adapter.

* [DocumentationandResources](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/DocumentationandResources)
  * [BlankArduinoSketch](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/DocumentationandResources/BlankArduinoSketch) lays out the basic framework for a new piece of Arduino code.
  * [LibrariesForArduinoIDE](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/DocumentationandResources/LibrariesForArduinoIDE) contains the library for the thermocouple.

* [FlightComputer](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/FlightComputer)
  * Software for the flight Arduino. This code is reading a text file named PWMCOORD.txt which has PWM coordinates written in.
These coordinates will open or close the motors to certain positions which enables us to control how much power is being outputted to the motors.
Once the code receives the character 'p' it's going to beging reading the coordinates from the text file and start setting the motors to the coordinates in the text file which enables lift off.

* [GroundControlwithRadio](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/GroundControlwithRadio)
  * The arduino Mega could not read Xbee data and send via USB at the same time, so this sketch runs on the arduino with the XBee. It takes a radio input from the XBee and sends it to an adjacent arduino as an intermediate step which will ultimately go to the ground control arduino without radio.

* [GroundControlwithoutRadio](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/GroundControlwithoutRadio)
  *  Reads the XBee data, processes it, and outputs it to LabView via serial connection. 

* [HealthMonitoring](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/HealthMonitoring)
  * Runs the health monitoring arduino, which is the arduino on the craft that has the XBee. It takes the data from the pressure sensors, thermocouples(pixhawk), voltage sensor and XBee (such as data from ground control and whether the connection is valid or not). It then sends this data over XBee to ground control while at the same time saving it to the SD card. This sketch also controls the dump valves and the safety.
