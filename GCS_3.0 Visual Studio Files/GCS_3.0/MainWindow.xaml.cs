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

    public partial class MainWindow : Window
    {
        List<CommandButton> command_buttons;
        List<char> command_characters;
        List<Sensor> pressure_transducers, thermocouples, voltage, ammeter;
        List<ServoMotor> thrust_servos;
        SerialPort gc_arduino;
        HealthPacketParseMachine packet_parse;
        StreamWriter data_writer;
        string command_flags, confirmed_flags;
        string data_filename;
        string[] flags;
        int selected_profile;
        bool running, save_data_flag, craft_abort_flag;

        readonly BackgroundWorker communicator;

        public MainWindow()
        {
            packet_parse = new HealthPacketParseMachine();

            communicator = new BackgroundWorker();
            communicator.DoWork += transmit_and_receive;
            communicator.RunWorkerCompleted += exchange_communication_info;

            command_flags = "h:;";
            flags = new string[2];

            running = false;
            craft_abort_flag = false;

            InitializeComponent();

            command_buttons = new List<CommandButton>() {
                PrimSafeButton, FullPowerButton, TakeoffButton, SimButton, ResetButton, SoftkillButton,
                HardkillButton, DumpFuelButton, FanShutButton, FOUButton, FCUButton, AV5Button, AV6Button};
            command_characters = new List<char>() { 's', 'z', 'o', 'p', 'r', 'k', 'a', 'b', 'f', 'd', 'i', 't', 'v' };

            setup_sensors();
            setup_servos();
            setup_command_buttons();
        }

        private void transmit_and_receive(object sender, DoWorkEventArgs e)
        {
            string[] flags = (string[])e.Argument;

            try
            {
                gc_arduino.Write(flags[0]);

                packet_parse.packet = gc_arduino.ReadLine().Trim();
                flags[1] = packet_parse.get_craft_state();
            }
            catch (System.IO.IOException) { }

            Thread.Sleep(10);

            e.Result = flags[1];
        }

        private void exchange_communication_info(object sender, RunWorkerCompletedEventArgs e)
        {
            commandedStateBox.Text = command_flags;
            try {
                recievedStateBox.Text = e.Result.ToString();
            }
            catch (NullReferenceException) { }

            /* Setting colors of buttons based on confirmed state of craft. */
            confirmed_flags = e.Result.ToString();

            if (confirmed_flags != "")
            {
                char[] control_chars = { ' ' };
                char[] comm_flags = command_flags.Substring(2, command_flags.Length - 3).ToCharArray();
                try {
                    control_chars = confirmed_flags.Substring(2, confirmed_flags.Length - 3).ToCharArray();
                }
                catch { }
                foreach (char cc in command_characters)
                {
                    if (Array.IndexOf(control_chars, cc) == Array.IndexOf(comm_flags, cc)) {
                        command_buttons[command_characters.IndexOf(cc)].waiting = false;
                    }
                    if (-1 != Array.IndexOf(control_chars, 'e') && !craft_abort_flag)
                    {
                        MessageBox.Show("The craft has entered a softkill/hardkill state. FO-U, FC-U, AV5-M, and AV6-M can still be actuated if needed. Avoid sending commands which do not actuate the valves.",
                                        "Craft Status", MessageBoxButton.OK, MessageBoxImage.Stop);
                        craft_abort_flag = true;
                    } else if (-1 == Array.IndexOf(control_chars, 'e')) {
                        craft_abort_flag = false;
                    }

                    command_buttons[command_characters.IndexOf(cc)].set_confirmed_state(-1 != Array.IndexOf(control_chars, cc));
                    /* if (-1 != Array.IndexOf(control_chars, cc)) {
                     * command_buttons[command_characters.IndexOf(cc)].set_confirmed_state(true);
                     * } else {
                     * command_buttons[command_characters.IndexOf(cc)].set_confirmed_state(false);
                     * }
                     */
                }
            }

            update_sensor_data();
            update_servo_data();

            if (save_data_flag) {
                record_data();
            }

            /* Transmit and recieve updated data. */
            string[] flags = new string[2] { command_flags, confirmed_flags };
            if (running) communicator.RunWorkerAsync(flags);
            else
            {
                gc_arduino.Close();
                communicator.Dispose();
            }
        }

        public bool is_running {
            get {
                return running;
            }
            private set { }
        }

        bool initiate_connection()
        {
            try
            {
                gc_arduino = new SerialPort();
                gc_arduino.BaudRate = 9600;
                gc_arduino.PortName = comPortName.Text.Trim();
                gc_arduino.Open();
                return true;
            }
            catch
            {
                MessageBox.Show("No device detected at " + gc_arduino.PortName.ToString() +
                                ". Ensure proper connection and correct port name.");
                return false;
            }
        }

        private void decrement_profile_number(object sender, RoutedEventArgs e)
        {
            int num = Convert.ToInt32(selProf.Text) - 1;
            selProf.Text = num < 0 ? "0" : num.ToString();
        }

        private void increment_profile_number(object sender, RoutedEventArgs e)
        {
            selProf.Text = (Convert.ToInt32(selProf.Text) + 1).ToString();
        }

        private void quit_interface(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }

        private void commence_testing(object sender, RoutedEventArgs e)
        {
            if (initiate_connection())
            {
                bool cancel = !prepare_data_file();
                if (cancel)
                {
                    gc_arduino.Close();
                    return;
                }

                QuitButton.IsEnabled = false;
                StartButton.IsEnabled = false;
                running = true;

                //Commence exchange of information.
                string[] flags = new string[2] { command_flags, confirmed_flags };
                communicator.RunWorkerAsync(flags);
            }
        }

        private void end_testing(object sender, RoutedEventArgs e)
        {
            running = false;
            Thread.Sleep(10);

            QuitButton.IsEnabled = true;
            StartButton.IsEnabled = true;
            running = false;
        }

        void setup_sensors()
        {
            pressure_transducers = new List<Sensor>();
            thermocouples = new List<Sensor>();
            voltage = new List<Sensor>();
            ammeter = new List<Sensor>();

            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.TANK, "PT1-U", 6.36, -1253));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT2-U", 1.274, -250.9));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT1-M", 1.264, -250.2));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT2-M", 1.267, -249.8));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT3-M", 1.277, -250.3));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT4-M", 1.264, -251.3));
            pressure_transducers.Add(new Sensor((int)Sensor.Instrumentation.TRANSDUCER,
                                                (int)Sensor.PressureTransducer.OTHER, "PT5-M", 1.277, -251));

            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.PLUMBING, "TCp1-M", 1.0, 0.0));
            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.PLUMBING, "TCp2-M", 1.0, 0.0));
            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.ENGINE, "TCw1-E", 1.0, 0.0));
            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.ENGINE, "TCw2-E", 1.0, 0.0));
            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.ENGINE, "TCw3-E", 1.0, 0.0));
            thermocouples.Add(new Sensor((int)Sensor.Instrumentation.THERMOCOUPLE,
                                         (int)Sensor.ThermoSensor.ENGINE, "TCw4-E", 1.0, 0.0));

            voltage.Add(new Sensor((int)Sensor.Instrumentation.VOLTMETER, 0, "Voltage", 0.0069, 11.144));
            ammeter.Add(new Sensor((int)Sensor.Instrumentation.AMMETER, 0, "Current", 1.0, 0.0));

            PTsensorTable.ItemsSource = pressure_transducers;
            TCsensorTable.ItemsSource = thermocouples;
            VoltageSensorTable.ItemsSource = voltage;
            AmmeterTable.ItemsSource = ammeter;
        }

        void setup_servos()
        {
            thrust_servos = new List<ServoMotor>() {
                new ServoMotor("AV1-M", 1000),
                new ServoMotor("AV2-M", 1000),
                new ServoMotor("AV3-M", 1000),
                new ServoMotor("AV4-M", 1000)
            };

            ServoValsTable.ItemsSource = thrust_servos;
        }

        void setup_command_buttons()
        {
            command_buttons[0].continue_setup("Primary Safety", true, false, command_characters[0]);

            command_buttons[1].continue_setup("Full Power", false, false, command_characters[1]);
            command_buttons[1].toggle_enabled(false);

            command_buttons[2].continue_setup("Takeoff", false, true, command_characters[2]);
            command_buttons[2].toggle_enabled(false);

            command_buttons[3].continue_setup("Simulate", false, false, command_characters[3]);
            command_buttons[4].continue_setup("Reset FC", false, false, command_characters[4]);
            
            command_buttons[5].continue_setup("Softkill", false, true, command_characters[5]);
            command_buttons[6].continue_setup("Hardkill", false, true, command_characters[6]);
            command_buttons[7].continue_setup("Dump Fuel", false, true, command_characters[7]);
            command_buttons[8].continue_setup("Fan Shutdown", false, true, command_characters[8]);
            
            command_buttons[9].continue_setup("FO-U", true, true, command_characters[9]);
            command_buttons[10].continue_setup("FC-U", false, true, command_characters[10]);
            command_buttons[11].continue_setup("AV5-M", false, true, command_characters[11]);
            command_buttons[12].continue_setup("AV6-M", false, true, command_characters[12]);
        }

        //Meant to be called by the command buttons only.
        public void change_command_string(CommandButton cb, string s, bool add_char)
        {
            int index = add_char ? 2 : command_flags.IndexOf(s);

            /* If the character is to be removed from the command string and the string to be removed is in fact 
             * in the command string, then remove the character. */
            if(-1 != index) {
                if (add_char) {
                    command_flags = command_flags.Insert(index, s);
                } else {
                    command_flags = command_flags.Substring(0, index) + command_flags.Substring(index + 1, command_flags.Length - index - 1);
                }
            }

            /* This code snippet causes the profile number to come before the 'o' character. To have it come after, 
             * this snippet can be moved to the beginning of this function. */
            if (s.Equals("o") || s.Equals("p")) {
                if (add_char) {
                    selected_profile = Convert.ToInt32(selProf.Text.Trim());
                    change_command_string(cb, selProf.Text.Trim(), add_char);
                } else {
                    change_command_string(cb, selected_profile.ToString(), add_char);
                }
            }

            cb.waiting = true;

            /*If Primary Safety is on, disable the Takeoff button. */
            if (cb == PrimSafeButton) FullPowerButton.toggle_enabled(cb.is_active);
        }

        /* To be used only by the Secondary Safety button to enable/disable the Takeoff button as needed. */
        public void toggle_takeoff_enabled(bool b)
        {
            TakeoffButton.toggle_enabled(b);
        }

        /* Reminder: values being displayed are currently raw values. */
        void update_sensor_data()
        {
            List<double> pressures = packet_parse.get_pressures();
            pressure_transducers[0].set_value(pressures[3]);
            pressure_transducers[1].set_value(pressures[4]);
            pressure_transducers[2].set_value(pressures[1]);
            pressure_transducers[3].set_value(pressures[5]);
            pressure_transducers[4].set_value(pressures[2]);
            pressure_transducers[5].set_value(pressures[0]);
            pressure_transducers[6].set_value(pressures[6]);

            UI_PT1_U.Text = pressures[3].ToString();
            UI_PT2_U.Text = pressures[4].ToString();
            UI_PT1_M.Text = pressures[1].ToString();
            UI_PT2_M.Text = pressures[5].ToString();
            UI_PT3_M.Text = pressures[2].ToString();
            UI_PT4_M.Text = pressures[0].ToString();    
            UI_PT5_M.Text = pressures[6].ToString();

            List<double> temperatures = packet_parse.get_temperatures();
            
            /* Don't be clever like this! Maintain readability and don't do fancy tricks to save a few lines. */
            /* for (int i = 0; i < thermocouples.Count; i++) thermocouples[i].set_value(i < 2 ? temperatures[i + 4] : temperatures[i - 2]); */
            thermocouples[0].set_value(temperatures[4]);
            thermocouples[1].set_value(temperatures[5]);
            thermocouples[2].set_value(temperatures[0]);
            thermocouples[3].set_value(temperatures[1]);
            thermocouples[4].set_value(temperatures[2]);
            thermocouples[5].set_value(temperatures[3]);

            UI_TCP1_M.Text = temperatures[4].ToString();
            UI_TCP2_M.Text = temperatures[5].ToString();
            UI_TCW1_E.Text = temperatures[0].ToString();
            UI_TCW2_E.Text = temperatures[1].ToString();
            UI_TCW3_E.Text = temperatures[2].ToString();
            UI_TCW4_E.Text = temperatures[3].ToString();
            
            //Setting statuses of thermocouple indicators.
            foreach(Sensor s in thermocouples)
            {
                TextBox tb = (TextBox)TCInds.Children[thermocouples.IndexOf(s)];
                int reading = Convert.ToInt32(s.get_raw_value());
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

            voltage[0].set_value(packet_parse.get_voltage());
            ammeter[0].set_value(packet_parse.get_current());
            
            /* Refresh datagrid and textblocks */
            
            PTsensorTable.ItemsSource = null;
            TCsensorTable.ItemsSource = null;
            VoltageSensorTable.ItemsSource = null;
            AmmeterTable.ItemsSource = null;

            PTsensorTable.ItemsSource = pressure_transducers;
            TCsensorTable.ItemsSource = thermocouples;
            VoltageSensorTable.ItemsSource = voltage;
            AmmeterTable.ItemsSource = ammeter;
             
        }

        void update_servo_data()
        {
            List<int> serv_vals = packet_parse.get_servos();
            for(int i = 0; i < serv_vals.Count; i++) {
                thrust_servos[i].set_value(serv_vals[i]);
            }

            ServoValsTable.ItemsSource = null;
            ServoValsTable.ItemsSource = thrust_servos;
        }

        //Take user input and prepare data file.
        bool prepare_data_file()
        {
            SaveFileDialog datalog = new SaveFileDialog();
            datalog.FileName = "TestData1";
            datalog.DefaultExt = ".csv";
            datalog.Filter = "CSV File (.csv)|*.csv";
            save_data_flag = datalog.ShowDialog().HasValue;
            if (save_data_flag)
            {
                data_filename = datalog.FileName;
                try {
                    File.WriteAllText(data_filename, "");
                }
                catch(System.IO.IOException)
                {
                    MessageBox.Show("The test file is being used by another program. Close the file and try again. Otherwise, enter a new filename.",
                                    "Data File Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return false;
                }

                data_writer = File.AppendText(data_filename);
                data_writer.Write("Time,");
                foreach (Sensor pt in pressure_transducers) { data_writer.Write(pt.name + ","); }
                foreach (Sensor tc in thermocouples) { data_writer.Write(tc.name + ","); }
                foreach (Sensor vol in voltage) { data_writer.Write(vol.name + ","); }
                foreach (ServoMotor sm in thrust_servos) { data_writer.Write(sm.name + ","); }
                data_writer.Write("Recieved Command Flag");
                data_writer.Write("\n");
            }
            return true;
        }

        void record_data()
        {
            data_writer.Write(DateTime.Now.TimeOfDay.ToString() + ",");
            foreach (Sensor pt in pressure_transducers) { data_writer.Write(pt.get_raw_value().ToString() + ","); }
            foreach (Sensor tc in thermocouples) { data_writer.Write(tc.get_raw_value().ToString() + ","); }
            foreach (Sensor vol in voltage) { data_writer.Write(vol.get_raw_value().ToString() + ","); }
            foreach (Sensor am in ammeter) { data_writer.Write(am.get_raw_value().ToString() + ","); }
            foreach (ServoMotor sm in thrust_servos) { data_writer.Write(sm.pwm + ","); }
            data_writer.Write(confirmed_flags + "\n");
        }
    }
}
