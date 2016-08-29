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
using Microsoft.Win32;
using System.IO;

namespace FlightProfileDesigner
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        double timeInc;
        string latestState;
        public MainWindow()
        {
            timeInc = 0.1;
            latestState = "1000,1000,1000,1000";

            InitializeComponent();
        }

        private void AddStep(object sender, RoutedEventArgs e)
        {
            int steps = profileSteps.Children.Count;
            latestState = steps > 0 ? ((ProfileStep)profileSteps.Children[steps - 1]).getEndValues() : "1000,1000,1000,1000";

            profileSteps.Children.Add(new ProfileStep(latestState));
        }

        private void RemoveStep(object sender, RoutedEventArgs e)
        {
            profileSteps.Children.RemoveAt(profileSteps.Children.Count - 1);
        }

        private void ClearSteps(object sender, RoutedEventArgs e)
        {
            profileSteps.Children.Clear();
        }

        private void CreateProfile(object sender, RoutedEventArgs e)
        {
            string finalString = "";
            foreach(ProfileStep ps in profileSteps.Children)
            {
                string stepString = "";
                int iterations = ps.getMilliseconds() / 100;
                List<int> startVals = new List<int>();
                List<int> endVals = new List<int>();
                List<double> increments = new List<double>();
                string[] endValString = ps.getEndValues().Split(',');

                foreach (string s in endValString) { endVals.Add(Convert.ToInt32(s)); }

                foreach (int i in startVals) stepString += i.ToString();
                if(!ps.isSustaining)
                {
                    string[] startValString = ps.getStartValues().Split(',');
                    foreach (string s in startValString) { startVals.Add(Convert.ToInt32(s)); }
                    for (int i = 0; i < 4; i++) increments.Add((endVals[i] - startVals[i]) / (double)iterations); 
                    for(int i = 0; i < iterations + 1; i++)
                    {
                        for(int j = 0; j < 4; j++)
                        {
                            stepString += ((int)(startVals[j] + (i *increments[j]))).ToString();
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < iterations; i++)
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            stepString += endVals[j].ToString();
                        }
                    }
                }

                finalString += stepString;
            }

            SaveFileDialog ofd = new SaveFileDialog();
            ofd.DefaultExt = ".txt";
            ofd.Filter = "Profile Text File (.txt)|*.txt";
            ofd.FileName = "pwm2.txt";

            bool saveData = ofd.ShowDialog().HasValue;
            if (saveData) File.WriteAllText(ofd.FileName, finalString);
        }

        private void CreatePreview(object sender, RoutedEventArgs e)
        {
            string finalString = "Time,PWM 1,PWM 2,PWM 3,PWM 4\n";

            double time = 0;
            foreach (ProfileStep ps in profileSteps.Children)
            {
                string stepString = "";
                int iterations = ps.getMilliseconds() / 100;
                List<int> startVals = new List<int>();
                List<int> endVals = new List<int>();
                List<double> increments = new List<double>();
                string[] endValString = ps.getEndValues().Split(',');

                foreach (string s in endValString) { endVals.Add(Convert.ToInt32(s)); }

                foreach (int i in startVals) stepString += i.ToString();
                if (!ps.isSustaining)
                {
                    string[] startValString = ps.getStartValues().Split(',');
                    foreach (string s in startValString) { startVals.Add(Convert.ToInt32(s)); }
                    for (int i = 0; i < 4; i++) increments.Add((endVals[i] - startVals[i]) / (double)iterations);
                    for (int i = 0; i < iterations + 1; i++)
                    {
                        stepString += time.ToString() + ",";
                        for (int j = 0; j < 4; j++)
                        {
                            stepString += ((int)(startVals[j] + (i * increments[j]))).ToString();
                            stepString += j == 3 ? "" : ",";
                        }
                        stepString += "\n";
                        time += timeInc;
                    }
                }
                else
                {
                    for (int i = 0; i < iterations; i++)
                    {
                        stepString += time.ToString() + ",";
                        for (int j = 0; j < 4; j++)
                        {
                            stepString += endVals[j].ToString();
                            stepString += j == 3 ? "" : ",";
                        }
                        stepString += "\n";
                        time += timeInc;
                    }
                }

                finalString += stepString;
            }

            SaveFileDialog ofd = new SaveFileDialog();
            ofd.DefaultExt = ".csv";
            ofd.Filter = "Profile Preview File (.csv)|*.csv";
            ofd.FileName = "profilePreview.csv";

            bool saveData = ofd.ShowDialog().HasValue;
            if (saveData) File.WriteAllText(ofd.FileName, finalString);
        }
    }
}
