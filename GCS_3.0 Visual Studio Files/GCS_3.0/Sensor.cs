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
        double sensedValue, maxSensedValue;
        double multi, off;
        public Sensor(int type, int subtype, string name, double multiplier, double offset)
        {
            Type = type;
            SubType = subtype;
            Name = name;
            sensedValue = 0;
            maxSensedValue = 0;

            //Setting calibration values.
            multi = multiplier;
            off = offset;

            nothing = Brushes.Transparent;
            warning = Brushes.Yellow;
            danger = Brushes.Red;
            safe = Brushes.Green;
        }

        //0 is pressure transducer, 1 is thermocouple, 2 is voltage sensor, 3 is current sensor
        public enum Instrumentation: int { Transducer, Thermocouple, Voltmeter, Ammeter}
        private int Type { get; set; }

        //For PTs: 0 is tank, 1 represents all other PTs
        public enum PressureTransducer { Tank, Other}
        //For TCs: 0 is engine, 1 is plumbing
        public enum ThermoSensor {  Engine, Plumbing}
        //For Voltage: 0 is the only type
        const double low_voltage_warning = 15.5, low_voltage_danger = 14.5, high_voltage_warning = 16.8, high_voltage_danger = 17.0;
        const double overpressure_warning = 800.0, overpressure_danger = 900;
        const double engine_overheat_warning = 1200.0, engine_overheat_danger = 1400.0;
        const double plumbing_overheat_warning = 120.0, plumbing_overheat_danger = 200.0;

        //For the time being, only current at the battery terminals is being considered, so 0 will cover all subtypes.
        const double over_current_warning = 30.0, over_current_danger = 40.0;
        private int SubType { get; set; }
        public string Name { get; private set; }
        public SolidColorBrush Status
        {
            get
            {
                switch (Type)
                {
                    case (int)Instrumentation.Transducer:
                        if (SubType == (int)PressureTransducer.Other && sensedValue > overpressure_warning && sensedValue < overpressure_danger) return warning;
                        else if (SubType == (int)PressureTransducer.Other && sensedValue > overpressure_danger) return danger;
                        else return nothing;
                    case (int)Instrumentation.Thermocouple:
                        if (SubType == (int)ThermoSensor.Engine && sensedValue > engine_overheat_warning && sensedValue < engine_overheat_danger) return warning;
                        else if (SubType == (int)ThermoSensor.Plumbing && sensedValue > plumbing_overheat_warning && sensedValue < plumbing_overheat_danger) return warning;
                        else if (SubType == (int)ThermoSensor.Engine && sensedValue > engine_overheat_danger) return danger;
                        else if (SubType == (int)ThermoSensor.Plumbing && sensedValue > plumbing_overheat_danger) return danger;
                        else return nothing;
                    case (int)Instrumentation.Voltmeter:
                        if (sensedValue <= low_voltage_danger) return danger;
                        else if (sensedValue > low_voltage_danger && sensedValue <= low_voltage_warning) return warning;
                        else if (sensedValue > low_voltage_warning && sensedValue < high_voltage_warning) return safe;
                        else if (sensedValue >= high_voltage_warning && sensedValue < high_voltage_danger) return warning;
                        else return danger; // the only remaining possibility is that the voltage is above the danger threshold
                        // else return nothing; /* this is unreachable code */
                    case (int)Instrumentation.Ammeter:
                        if (sensedValue < over_current_warning) return safe;
                        else if (sensedValue >= over_current_warning && sensedValue < over_current_danger) return warning;
                        else return danger;
                }
                return nothing;
            }
        }
        public string Value
        {
            get
            {
                switch(Type)
                {
                    case (int)Instrumentation.Transducer:
                        return sensedValue.ToString() + " psi";
                    case (int)Instrumentation.Thermocouple:
                        return sensedValue.ToString() + " °F";
                    case (int)Instrumentation.Voltmeter:
                        return sensedValue.ToString() + " V";
                    case (int)Instrumentation.Ammeter:
                        return sensedValue.ToString() + " A";
                }
                return "";
            }
        }
        public string MaxValue
        {
            get
            {
                switch (Type)
                {
                    case (int)Instrumentation.Transducer:
                        return maxSensedValue.ToString() + " psi";
                    case (int)Instrumentation.Thermocouple:
                        return maxSensedValue.ToString() + " °F";
                    case (int)Instrumentation.Voltmeter:
                        return maxSensedValue.ToString() + " V";
                    case (int)Instrumentation.Ammeter:
                        return maxSensedValue.ToString() + " A";
                }
                return "";
            }
        }

        //Put calibrations here with switch statement for each type of sensor.
        public void setValue(double val)
        {
            sensedValue = (multi * val) + off;

            if(sensedValue > maxSensedValue) maxSensedValue = sensedValue;
        }

        public double getValue_Double()
        {
            return sensedValue;
        }
    }
}
