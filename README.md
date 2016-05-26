# ArduinoFlightSoftware
####This is the repository for the Lunar Lions' Arduino-based Flight Software. Currently, this includes:

[ArduinoLibraries](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/ArduinoLibraries)
  * [MAX31855](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/ArduinoLibraries/MAX31855) contains the library for the thermocouple board.

[ComponentTestingPrograms](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/ComponentTestingPrograms)
  * [SDCardSpeedTest](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/ComponentTestingPrograms/SDCardSpeedTest) tests the functionality and speed of the Arduino SD card adapter.

[FlightComputer](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/FlightComputer)
  * Runs on Puma's arduino without the XBee. Reads a text file named PWMCOORD.txt, which contains PWM values from 1000 to 2000.
These values will open or close the servos to certain positions, which enables us to control thrust.

[GroundControlwithRadio](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/GroundControlwithRadio)
  * Runs on the ground control arduino with the XBee. 
It takes a radio input from the XBee and sends it to the GroundControlWithoutRadio.
This is necessary beacuse the arduino can't read Xbee data and send via USB at the same time.

[GroundControlwithoutRadio](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/GroundControlwithoutRadio)
  *  Runs on the ground control arduino without the XBee.
Sends craft health monitoring information to the PC with Labview, and sends control flags to the craft.

[HealthMonitoring](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/HealthMonitoring)
  * Runs on Puma's arduino with the XBee. 
It aggregates the data from the pressure sensors, thermocouples, voltage sensor, etc. 
It then sends this data over XBee to ground control and saves it to the SD card. 
This sketch also controls and responds to the command flags from the ground control station.

[Labview](https://github.com/JonBees/ArduinoFlightSoftware/tree/master/Labview)
  * This is the ground control interface software. 
It sends command flags and recieves health monitoring information via the ground control arduinos, and displays it in a (somewhat) user-friendly way. 

SampleThrustValues.py
  * Generates a pwmcoord file for the FlightComputer to use to set thrust values.
