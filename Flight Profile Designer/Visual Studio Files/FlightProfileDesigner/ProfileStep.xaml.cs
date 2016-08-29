using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace FlightProfileDesigner
{
    /// <summary>
    /// Interaction logic for ProfileStep.xaml
    /// </summary>
    public partial class ProfileStep : UserControl
    {
        //True = Sustain, False = Ramp
        bool sustain;
        public ProfileStep(string startVals)
        {
            sustain = false;

            InitializeComponent();

            sustainCheckBox.IsChecked = false;
            startPWM.Text = startVals;
        }

        public string getStartValues()
        {
            string[] removingWhiteSpace = startPWM.Text.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
            string startVals = "";
            foreach (string s in removingWhiteSpace)
            {
                startVals += s;
            }
            return startVals;
        }

        public string getEndValues()
        {
            string[] removingWhiteSpace = endPWM.Text.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
            string endVals = "";
            foreach(string s in removingWhiteSpace)
            {
                endVals += s;
            }
            return endVals;
        }

        public int getMilliseconds()
        {
            return Convert.ToInt32(timePer.Text.Trim());
        }

        public bool isSustaining
        {
            get { return sustain; }
        }

        private void updateSustainBoolSCB(object sender, RoutedEventArgs e)
        {
            sustain = (bool)sustainCheckBox.IsChecked;

            startPWM.IsEnabled = !sustain;
            startPWM.Text = sustain ? "" : startPWM.Text;
        }
    }
}
