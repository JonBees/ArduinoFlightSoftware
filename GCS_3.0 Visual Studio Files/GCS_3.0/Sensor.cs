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
        public Sensor(int type, int subtype, string name)
        {
            Type = type;
            SubType = subtype;
            Name = name;
            sensedValue = 0;
            maxSensedValue = 0;

            nothing = Brushes.Transparent;
            warning = Brushes.Yellow;
            danger = Brushes.Red;
            safe = Brushes.Green;
        }

        //0 is pressure transducer, 1 is thermocouple, 2 is voltage sensor
        private int Type { get; set; }

        //For PTs: 0 is tank, 1 represents all other PTs
        //For TCs: 0 is engine, 1 is plumbing
        //For Voltage: 0 is the only type
        private int SubType { get; set; }
        public string Name { get; private set; }
        public SolidColorBrush Status
        {
            get
            {
                switch (Type)
                {
                    case 0:
                        if (SubType == 1 && sensedValue > 800 && sensedValue < 900) return warning;
                        else if (SubType == 1 && sensedValue > 900) return danger;
                        else return nothing;
                    case 1:
                        if (SubType == 0 && sensedValue > 1200 && sensedValue < 1400) return warning;
                        else if (SubType == 1 && sensedValue > 120 && sensedValue < 140) return warning;
                        else if (SubType == 0 && sensedValue > 1400) return danger;
                        else if (SubType == 1 && sensedValue > 200) return danger;
                        else return nothing;
                    case 2:
                        if (sensedValue <= 14.5) return danger;
                        else if (sensedValue > 14.5 && sensedValue <= 15.5) return warning;
                        else if (sensedValue > 15.5 && sensedValue < 16.8) return safe;
                        else if (sensedValue >= 16.8 && sensedValue < 17) return warning;
                        else if (sensedValue >= 17.0) return danger;
                        else return nothing;
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
                    case 0:
                        return sensedValue.ToString() + " psi";
                    case 1:
                        return sensedValue.ToString() + " °F";
                    case 2:
                        return sensedValue.ToString() + " V";
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
                    case 0:
                        return maxSensedValue.ToString() + " psi";
                    case 1:
                        return maxSensedValue.ToString() + " °F";
                    case 2:
                        return maxSensedValue.ToString() + " V";
                }
                return "";
            }
        }

        //Put calibrations here with swithc statement for each type of sensor.
        public void setValue(double val)
        {
            sensedValue = val;
            if(sensedValue > maxSensedValue) maxSensedValue = sensedValue;
        }

        public double getValue_Double()
        {
            return sensedValue;
        }
    }
}
