using System;
using System.Collections.Generic;
using System.Windows;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GCS_3._0
{
    //Sample health packet recieved from craft for reference:
    //3301|p:203,196,196,195,194,197,194;t:74.30,72.95,70.70,71.15,70.70,70.70;v:549;m:1000,1000,1000,1000;h:;s:;?102|b:-1.00

    //TO-DO: Must add calibrations and such.
    //TO-DO: Add current reading to health packet
    class HealthPacketParseMachine
    {
        public HealthPacketParseMachine() { }

        public string Packet { get; set; }

        public List<double> getPressures()
        {
            int startIndex = Packet.IndexOf('p') + 2;
            string pressString = Packet.Substring(startIndex);
            pressString = pressString.Split(';')[0];

            List<double> pressures = new List<double>();
            string[] presses = pressString.Split(',');
            foreach(string s in presses) { pressures.Add(Convert.ToDouble(s.Trim())); }

            return pressures;
        }

        public List<double> getTemperatures()
        {
            int startIndex = Packet.IndexOf('t') + 2;
            string tempString = Packet.Substring(startIndex);
            tempString = tempString.Split(';')[0];

            List<double> temperatures = new List<double>();
            string[] temps = tempString.Split(',');
            foreach (string s in temps) { temperatures.Add(Convert.ToDouble(s.Trim())); }

            return temperatures;
        }

        public double getVoltage()
        {
            int startIndex = Packet.IndexOf('v') + 2;
            string voltString = Packet.Substring(startIndex);
            voltString = voltString.Split(';')[0];

            return Convert.ToDouble(voltString);
        }

        // This is a temporary dummy function until the appropriate function is written
        public double getCurrent()
        {
            return 1.0;
        }

        public List<int> getServos()
        {
            int startIndex = Packet.IndexOf('m') + 2;
            string servString = Packet.Substring(startIndex);
            servString = servString.Split(';')[0];

            List<int> servos = new List<int>();
            string[] servs = servString.Split(',');
            foreach (string s in servs) { servos.Add(Convert.ToInt32(s.Trim())); }

            return servos;
        }

        public string getCraftState()
        {
            string state = "";
            try
            {
                state = Packet.Substring(Packet.IndexOf('h'));
                state = state.Remove(state.LastIndexOf('s'));
            }
            catch { }

            return state;
        }
    }
}
