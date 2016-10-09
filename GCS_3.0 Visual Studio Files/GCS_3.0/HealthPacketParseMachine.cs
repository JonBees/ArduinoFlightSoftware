using System;
using System.Collections.Generic;
using System.Windows;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

/* Sample health packet recieved from craft for reference:
 * 3301|p:203,196,196,195,194,197,194;t:74.30,72.95,70.70,71.15,70.70,70.70;v:549,849;m:1000,1000,1000,1000;h:;s:;?102|b:-1.00
 */

namespace GCS_3._0
{

    /* TO-DO: Must add calibrations and such.
     * TO-DO: Add current reading to health packet */
    class HealthPacketParseMachine
    {
        public HealthPacketParseMachine() { }

        public string packet { get; set; }

        public List<double> get_pressures()
        {
            int start_idx = packet.IndexOf('p') + 2;
            string raw_pres = packet.Substring(start_idx);
            raw_pres = raw_pres.Split(';')[0];

            List<double> pressures = new List<double>();
            string[] presses = raw_pres.Split(',');
            foreach(string s in presses) { pressures.Add(Convert.ToDouble(s.Trim())); }

            return pressures;
        }

        public List<double> get_temperatures()
        {
            int start_idx = packet.IndexOf('t') + 2;
            string tempString = packet.Substring(start_idx);
            tempString = tempString.Split(';')[0];

            List<double> temperatures = new List<double>();
            string[] temps = tempString.Split(',');
            foreach (string s in temps) { temperatures.Add(Convert.ToDouble(s.Trim())); }

            return temperatures;
        }

        public double get_voltage()
        {
            int start_idx = packet.IndexOf('v') + 2;
            string voltString = packet.Substring(start_idx);
            voltString = voltString.Split(',')[0];

            return Convert.ToDouble(voltString);
        }

        // This is a temporary dummy function until the appropriate function is written
        public double get_current()
        {
            int start_idx = packet.IndexOf('v') + 2;
            string raw_current = packet.Substring(start_idx);
            raw_current = raw_current.Split(';')[0];

            return Convert.ToDouble(raw_current);
        }

        public List<int> get_servos()
        {
            int start_idx = packet.IndexOf('m') + 2;
            string servString = packet.Substring(start_idx);
            servString = servString.Split(';')[0];

            List<int> servos = new List<int>();
            string[] servs = servString.Split(',');
            foreach (string s in servs) { servos.Add(Convert.ToInt32(s.Trim())); }

            return servos;
        }

        public string get_craft_state()
        {
            string state = "";
            try
            {
                state = packet.Substring(packet.IndexOf('h'));
                state = state.Remove(state.LastIndexOf('s'));
            }
            catch { }

            return state;
        }
    }
}
