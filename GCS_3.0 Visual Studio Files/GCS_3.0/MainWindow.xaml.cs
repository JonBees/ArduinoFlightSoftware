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
using System.IO.Ports;
using System.Threading;
using System.Windows.Threading;
using System.ComponentModel;
using System.Diagnostics;
using Microsoft.Win32;
using System.IO;

namespace GCS_3._0
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 

    public partial class MainWindow : Window
    {
        List<CommandButton> commandButtons;
        List<char> commandCharacters;
        List<Sensor> PTs, TCs, voltage;
        List<ServoMotor> thrustServos;
        SerialPort GCArduino;
        HealthPacketParseMachine packetParse;
        StreamWriter dataWriter;
        string commandFlags, confirmedFlags;
        string dataFileName;
        string[] flags;
        int selectedProfile;
        bool running, saveData, craftAbort;

        readonly BackgroundWorker communicator;

        public MainWindow()
        {
            packetParse = new HealthPacketParseMachine();

            communicator = new BackgroundWorker();
            communicator.DoWork += TransmitAndRecieve;
            communicator.RunWorkerCompleted += ExchangeCommunicationInfo;

            commandFlags = "h:;";
            flags = new string[2];

            running = false;
            craftAbort = false;

            InitializeComponent();

            commandButtons = new List<CommandButton>() { PrimSafeButton, FullPowerButton, TakeoffButton, SimButton,
                ResetButton, SoftkillButton, HardkillButton, DumpFuelButton, FanShutButton, FOUButton, FCUButton, AV5Button, AV6Button};
            commandCharacters = new List<char>() { 's', 'z', 'o', 'p', 'r', 'k', 'a', 'b', 'f', 'd', 'i', 't', 'v' };

            setupSensors();
            setupServos();
            setupCommandButtons();
        }

        private void TransmitAndRecieve(object sender, DoWorkEventArgs e)
        {
            string[] flags = (string[])e.Argument;

            try
            {
                GCArduino.Write(flags[0]);

                packetParse.Packet = GCArduino.ReadLine().Trim();
                flags[1] = packetParse.getCraftState();
            }
            catch (System.IO.IOException) { }

            Thread.Sleep(10);

            e.Result = flags[1];
        }

        private void ExchangeCommunicationInfo(object sender, RunWorkerCompletedEventArgs e)
        {
            commandedStateBox.Text = commandFlags;
            try { recievedStateBox.Text = e.Result.ToString(); }
            catch (NullReferenceException) { }

            //Setting colors of buttons based on confirmed state of craft.
            confirmedFlags = e.Result.ToString();
            if (confirmedFlags != "")
            {
                char[] conChs = { ' ' };
                char[] commFlgs = commandFlags.Substring(2, commandFlags.Length - 3).ToCharArray();
                try { conChs = confirmedFlags.Substring(2, confirmedFlags.Length - 3).ToCharArray(); }
                catch { }
                foreach (char cc in commandCharacters)
                {
                    if (Array.IndexOf(conChs, cc) == Array.IndexOf(commFlgs, cc)) commandButtons[commandCharacters.IndexOf(cc)].Waiting = false;
                    if (Array.IndexOf(conChs, 'e') != -1 && craftAbort == false)
                    {
                        MessageBox.Show("The craft has entered a softkill/hardkill state. FO-U, FC-U, AV5-M, and AV6-M can still be actuated if needed. Avoid sending commands which do not actuate the valves.",
                        "Craft Status", MessageBoxButton.OK, MessageBoxImage.Stop);
                        craftAbort = true;
                    }
                    else if (Array.IndexOf(conChs, 'e') == -1) craftAbort = false;

                    if (Array.IndexOf(conChs, cc) != -1) commandButtons[commandCharacters.IndexOf(cc)].setConfirmedState(true);
                    else commandButtons[commandCharacters.IndexOf(cc)].setConfirmedState(false);
                }
            }
            //

            updateSensorData();
            updateServoData();

            if (saveData) recordData();

            //Transmit and recieve updated data.
            string[] flags = new string[2] { commandFlags, confirmedFlags };
            if (running) communicator.RunWorkerAsync(flags);
            else
            {
                GCArduino.Close();
                communicator.Dispose();
            }
        }

        public bool Running
        {
            get { return running; }
            private set { }
        }

        bool InitiateConnection()
        {
            try
            {
                GCArduino = new SerialPort();
                GCArduino.BaudRate = 9600;
                GCArduino.PortName = comPortName.Text.Trim();
                GCArduino.Open();
                return true;
            }
            catch
            {
                MessageBox.Show("No device detected at " + GCArduino.PortName.ToString() + ". Ensure proper connection and correct port name.");
                return false;
            }
        }

        private void DecrementProfileNumber(object sender, RoutedEventArgs e)
        {
            int num = Convert.ToInt32(selProf.Text) - 1;
            selProf.Text = num < 0 ? "0" : num.ToString();
        }

        private void IncrementProfileNumber(object sender, RoutedEventArgs e)
        {
            selProf.Text = (Convert.ToInt32(selProf.Text) + 1).ToString();
        }

        private void QuitInterface(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }

        private void CommenceTesting(object sender, RoutedEventArgs e)
        {
            if (InitiateConnection())
            {
                bool cancel = !prepareDataFile();
                if (cancel)
                {
                    GCArduino.Close();
                    return;
                }

                QuitButton.IsEnabled = false;
                StartButton.IsEnabled = false;
                running = true;

                //Commence exchange of information.
                string[] flags = new string[2] { commandFlags, confirmedFlags };
                communicator.RunWorkerAsync(flags);
            }
        }

        private void EndTesting(object sender, RoutedEventArgs e)
        {
            running = false;
            Thread.Sleep(10);

            QuitButton.IsEnabled = true;
            StartButton.IsEnabled = true;
            running = false;
        }

        void setupSensors()
        {
            PTs = new List<Sensor>();
            TCs = new List<Sensor>();
            voltage = new List<Sensor>();

            PTs.Add(new Sensor(0, 0, "PT1-U"));
            PTs.Add(new Sensor(0, 1, "PT2-U"));
            PTs.Add(new Sensor(0, 1, "PT1-M"));
            PTs.Add(new Sensor(0, 1, "PT2-M"));
            PTs.Add(new Sensor(0, 1, "PT3-M"));
            PTs.Add(new Sensor(0, 1, "PT4-M"));
            PTs.Add(new Sensor(0, 1, "PT5-M"));

            TCs.Add(new Sensor(1, 1, "TCp1-M"));
            TCs.Add(new Sensor(1, 1, "TCp2-M"));
            TCs.Add(new Sensor(1, 0, "TCw1-E"));
            TCs.Add(new Sensor(1, 0, "TCw2-E"));
            TCs.Add(new Sensor(1, 0, "TCw3-E"));
            TCs.Add(new Sensor(1, 0, "TCw4-E"));

            voltage.Add(new Sensor(2, 0, "Voltage"));

            PTsensorTable.ItemsSource = PTs;
            TCsensorTable.ItemsSource = TCs;
            VoltageSensorTable.ItemsSource = voltage;
        }

        void setupServos()
        {
            thrustServos = new List<ServoMotor>()
            {
                new ServoMotor("AV1-M", 1000),
                new ServoMotor("AV2-M", 1000),
                new ServoMotor("AV3-M", 1000),
                new ServoMotor("AV4-M", 1000)
            };

            ServoValsTable.ItemsSource = thrustServos;
        }

        void setupCommandButtons()
        {
            commandButtons[0].continueSetup("Primary Safety", true, false, commandCharacters[0]);

            commandButtons[1].continueSetup("Full Power", false, false, commandCharacters[1]);
            commandButtons[1].ToggleEnabled(false);

            commandButtons[2].continueSetup("Takeoff", false, true, commandCharacters[2]);
            commandButtons[2].ToggleEnabled(false);

            commandButtons[3].continueSetup("Simulate", false, false, commandCharacters[3]);
            commandButtons[4].continueSetup("Reset FC", false, false, commandCharacters[4]);
            commandButtons[5].continueSetup("Softkill", false, true, commandCharacters[5]);
            commandButtons[6].continueSetup("Hardkill", false, true, commandCharacters[6]);
            commandButtons[7].continueSetup("Dump Fuel", false, true, commandCharacters[7]);
            commandButtons[8].continueSetup("Fan Shutdown", false, true, commandCharacters[8]);
            commandButtons[9].continueSetup("FO-U", true, true, commandCharacters[9]);
            commandButtons[10].continueSetup("FC-U", false, true, commandCharacters[10]);
            commandButtons[11].continueSetup("AV5-M", false, true, commandCharacters[11]);
            commandButtons[12].continueSetup("AV6-M", false, true, commandCharacters[12]);
        }

        //Meant to be called by the command buttons only.
        public void changeCommandString(CommandButton cb, string s, bool addChar)
        {
            int index = addChar ? 2 : commandFlags.IndexOf(s);

            /*If the character is to be removed from the command string and the string to be removed is in fact in the command string,
            then remove the character.*/
            if(index != -1) commandFlags = addChar ? commandFlags.Insert(index, s) :
                commandFlags.Substring(0, index) + commandFlags.Substring(index + 1, commandFlags.Length - index - 1);

            /*This code snippet causes the profile number to come before the 'o' character. To have it come after, this snippet can be moved
            to the beginning of this function.*/
            if (s.Equals("o") || s.Equals("p"))
            {
                if (addChar)
                {
                    selectedProfile = Convert.ToInt32(selProf.Text.Trim());
                    changeCommandString(cb, selProf.Text.Trim(), addChar);
                }
                else changeCommandString(cb, selectedProfile.ToString(), addChar);
            }

            cb.Waiting = true;

            //If Primary Safety is on, disable the Takeoff button.
            if (cb == PrimSafeButton) FullPowerButton.ToggleEnabled(cb.IsActive);
        }

        //To be used only by the Secondary Safety button to enable/disable the Takeoff button as needed.
        public void ToggleTakeoffEnabled(bool b)
        {
            TakeoffButton.ToggleEnabled(b);
        }

        //Reminder: values being displayed are currently raw values.
        void updateSensorData()
        {
            List<double> pressures = packetParse.getPressures();
            PTs[5].setValue(pressures[0]);
            PTs[2].setValue(pressures[1]);
            PTs[4].setValue(pressures[2]);
            PTs[0].setValue(pressures[3]);
            PTs[1].setValue(pressures[4]);
            PTs[3].setValue(pressures[5]);
            PTs[6].setValue(pressures[6]);

            List<double> temperatures = packetParse.getTemperatures();
            for (int i = 0; i < TCs.Count; i++) TCs[i].setValue(i < 2 ? temperatures[i + 4] : temperatures[i - 2]);
            //Setting statuses of thermocouple indicators.
            foreach(Sensor s in TCs)
            {
                TextBox tb = (TextBox)TCInds.Children[TCs.IndexOf(s)];
                int reading = Convert.ToInt32(s.getValue_Double());
                switch (reading)
                {
                    case -1:
                        tb.Background = Brushes.Crimson;
                        break;
                    case -2:
                        tb.Background = Brushes.Gold;
                        break;
                    case -3:
                        tb.Background = Brushes.SpringGreen;
                        break;
                }
            }

            voltage[0].setValue(packetParse.getVoltage());

            //Refresh datagrid.
            PTsensorTable.ItemsSource = null;
            TCsensorTable.ItemsSource = null;
            VoltageSensorTable.ItemsSource = null;

            PTsensorTable.ItemsSource = PTs;
            TCsensorTable.ItemsSource = TCs;
            VoltageSensorTable.ItemsSource = voltage;
        }

        void updateServoData()
        {
            List<int> servVals = packetParse.getServos();
            for(int i = 0; i < servVals.Count; i++) { thrustServos[i].setValue(servVals[i]);}

            ServoValsTable.ItemsSource = null;
            ServoValsTable.ItemsSource = thrustServos;
        }

        //Take user input and prepare data file.
        bool prepareDataFile()
        {
            SaveFileDialog datalog = new SaveFileDialog();
            datalog.FileName = "TestData1";
            datalog.DefaultExt = ".csv";
            datalog.Filter = "CSV File (.csv)|*.csv";
            saveData = datalog.ShowDialog().HasValue;
            if (saveData)
            {
                dataFileName = datalog.FileName;
                try { File.WriteAllText(dataFileName, ""); }
                catch(System.IO.IOException)
                {
                    MessageBox.Show("The test file is being used by another program. Close the file and try again. Otherwise, enter a new filename.",
                        "Data File Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return false;
                }

                dataWriter = File.AppendText(dataFileName);
                dataWriter.Write("Time,");
                foreach (Sensor pt in PTs) { dataWriter.Write(pt.Name + ","); }
                foreach (Sensor tc in TCs) { dataWriter.Write(tc.Name + ","); }
                foreach (Sensor vol in voltage) { dataWriter.Write(vol.Name + ","); }
                foreach (ServoMotor sm in thrustServos) { dataWriter.Write(sm.Name + ","); }
                dataWriter.Write("Recieved Command Flag");
                dataWriter.Write("\n");
            }
            return true;
        }

        void recordData()
        {
            dataWriter.Write(DateTime.Now.TimeOfDay.ToString() + ",");
            foreach (Sensor pt in PTs) { dataWriter.Write(pt.getValue_Double().ToString() + ","); }
            foreach (Sensor tc in TCs) { dataWriter.Write(tc.getValue_Double().ToString() + ","); }
            foreach (Sensor vol in voltage) { dataWriter.Write(vol.getValue_Double().ToString() + ","); }
            foreach (ServoMotor sm in thrustServos) { dataWriter.Write(sm.PWM + ","); }
            dataWriter.Write(confirmedFlags + "\n");
        }
    }
}
