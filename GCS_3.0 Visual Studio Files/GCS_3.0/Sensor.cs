using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;

namespace GCS_3._0
{
    class Sensor 
    {
        SolidColorBrush nothing, warning, danger, safe;
        double raw_value, max_raw_value;
        double multi, off;
        public Sensor(int sensor_type, int sensor_subtype, string sensor_name, double sensor_scalar, double sensor_offset)
        {
            type = sensor_type;
            subtype = sensor_subtype;
            name = sensor_name;
            raw_value = 0;
            max_raw_value = 0;

            /* Setting calibration values. */
            multi = sensor_scalar;
            off = sensor_offset;

            nothing = Brushes.Transparent;
            warning = Brushes.Yellow;
            danger = Brushes.Red;
            safe = Brushes.Green;
        }

        public enum Instrumentation: int {
            TRANSDUCER,   
            THERMOCOUPLE, 
            VOLTMETER,    
            AMMETER
        }
        
        private int type { get; set; }

        public enum PressureTransducer: int {
            TANK,
            OTHER
        }
        public enum ThermoSensor: int {
            ENGINE,
            PLUMBING
        }

        const double LOW_VOLTAGE_WARNING = 15.5, LOW_VOLTAGE_DANGER = 14.5, HIGH_VOLTAGE_WARNING = 16.8, HIGH_VOLTAGE_DANGER = 17.0;
        const double OVERPRESSURE_WARNING = 800.0, OVERPRESSURE_DANGER = 900;
        const double ENGINE_OVERHEAT_WARNING = 1200.0, ENGINE_OVERHEAT_DANGER = 1400.0;
        const double PLUMBING_OVERRHEAT_WARNING = 120.0, PLUMBING_OVERHEAT_DANGER = 200.0;

        //For the time being, only current at the battery terminals is being considered, so 0 will cover all subtypes.
        const double OVER_CURRENT_WARNING = 30.0, OVER_CURRENT_DANGER = 40.0;
        private int subtype { get; set; }
        public string name { get; private set; }
        public SolidColorBrush status
        {
            get
            {
                switch (type)
                {
                    case (int)Instrumentation.TRANSDUCER:
                        if ((int)PressureTransducer.OTHER == subtype &&
                            raw_value > OVERPRESSURE_WARNING &&
                            raw_value < OVERPRESSURE_DANGER) {
                            
                            return warning;
                            
                        } else if ((int)PressureTransducer.OTHER == subtype &&
                                   raw_value > OVERPRESSURE_DANGER) {
                            
                            return danger;
                            
                        } else {
                            return nothing;                            
                        }
                    case (int)Instrumentation.THERMOCOUPLE:
                        if ((int)ThermoSensor.ENGINE == subtype &&
                            raw_value > ENGINE_OVERHEAT_WARNING &&
                            raw_value < ENGINE_OVERHEAT_DANGER) {
                            
                            return warning;
                            
                        } else if (subtype == (int)ThermoSensor.PLUMBING &&
                                   raw_value > PLUMBING_OVERRHEAT_WARNING &&
                                   raw_value < PLUMBING_OVERHEAT_DANGER) {
                            
                            return warning;
                            
                        } else if (subtype == (int)ThermoSensor.ENGINE &&
                                   raw_value > ENGINE_OVERHEAT_DANGER) {
                            
                            return danger;
                            
                        } else if (subtype == (int)ThermoSensor.PLUMBING &&
                                   raw_value > PLUMBING_OVERHEAT_DANGER) {
                            
                            return danger;
                            
                        } else {
                            return nothing;
                        }
                    case (int)Instrumentation.VOLTMETER:
                        if (raw_value <= LOW_VOLTAGE_DANGER) {

                            return danger;
                            
                        } else if (raw_value > LOW_VOLTAGE_DANGER &&
                                   raw_value <= LOW_VOLTAGE_WARNING) {

                            return warning;
                            
                        } else if (raw_value > LOW_VOLTAGE_WARNING &&
                                   raw_value < HIGH_VOLTAGE_WARNING) {

                            return safe;
                            
                        } else if (raw_value >= HIGH_VOLTAGE_WARNING &&
                                   raw_value < HIGH_VOLTAGE_DANGER) {

                            return warning;
                            
                        } else {
                            return danger; /* only remaining possibility is that voltage > danger threshold */
                        }
                    case (int)Instrumentation.AMMETER:
                        if (raw_value < OVER_CURRENT_WARNING) {

                            return safe;
                            
                        } else if (raw_value >= OVER_CURRENT_WARNING &&
                                   raw_value < OVER_CURRENT_DANGER) {

                            return warning;
                            
                        } else {
                            return danger;
                        }
                }
                return nothing;
            }
        }
        public string value
        {
            get
            {
                switch(type)
                {
                    case (int)Instrumentation.TRANSDUCER:
                        return raw_value.ToString() + " psi";
                    case (int)Instrumentation.THERMOCOUPLE:
                        return raw_value.ToString() + " °F";
                    case (int)Instrumentation.VOLTMETER:
                        return raw_value.ToString() + " V";
                    case (int)Instrumentation.AMMETER:
                        return raw_value.ToString() + " A";
                }
                return "";
            }
        }
        public string max_value
        {
            get
            {
                switch (type)
                {
                    case (int)Instrumentation.TRANSDUCER:
                        return max_raw_value.ToString() + " psi";
                    case (int)Instrumentation.THERMOCOUPLE:
                        return max_raw_value.ToString() + " °F";
                    case (int)Instrumentation.VOLTMETER:
                        return max_raw_value.ToString() + " V";
                    case (int)Instrumentation.AMMETER:
                        return max_raw_value.ToString() + " A";
                }
                return "";
            }
        }

        //Put calibrations here with switch statement for each type of sensor.
        public void set_value(double val)
        {
            raw_value = (multi * val) + off;

            if(raw_value > max_raw_value) max_raw_value = raw_value;
        }

        public double get_raw_value()
        {
            return raw_value;
        }

        public string get_raw_value_string()
        {
            return raw_value.ToString();
        }
    }
}
