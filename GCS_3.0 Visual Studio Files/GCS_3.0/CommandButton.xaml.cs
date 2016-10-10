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

namespace GCS_3._0 {
    /// <summary>
    /// Interaction logic for CommandButton.xaml
    /// </summary>
    public partial class CommandButton : UserControl {
        SolidColorBrush dark_red, dark_green, green, red;
        string button_name;
        bool must_confirm;
        
        public CommandButton()
        {
            red = new SolidColorBrush(Color.FromArgb(255, 255, 0, 0));
            green = new SolidColorBrush(Color.FromArgb(255, 0, 255, 0));
            dark_red = new SolidColorBrush(Color.FromArgb(255, 180, 0, 0));
            dark_green = new SolidColorBrush(Color.FromArgb(255, 0, 180, 0));

            InitializeComponent();

            must_confirm = false;
        }

        /* This method MUST be called during initialization of the GCS interface, 
         * otherwise the command buttons will not function. */
        public void continue_setup(string new_button_name, bool def_on, bool confirmation, char comm_ch)
        {
            is_active = false;

            default_on = def_on;
            is_green = default_on;
            button.Background = is_green ? dark_green : dark_red;
            must_confirm = confirmation;

            command_character = comm_ch.ToString();

            button_name = new_button_name;
        }

        public string command_character { get; private set; }
        public bool is_active { get; private set; }

        private void mouse_over_effect(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (button.IsMouseDirectlyOver)
            {
                Border border = new Border
                {
                    BorderThickness = new Thickness(5),
                    BorderBrush = new SolidColorBrush(Colors.SteelBlue)
                };
                Grid.SetRow(border, 0);
                Grid.SetColumn(border, 0);
                button.Children.Add(border);
            }
            else button.Children.Clear();
        }

        public bool waiting { get; set; }
        public bool default_on { get; private set; }
        private bool is_green { get; set; }

        private void Command(object sender, MouseButtonEventArgs e)
        {
            MessageBoxResult user_confirmation;

            /* If the confirmation message is enabled for the button, present it. Otherwise, 
             * continue to the toggling of the button. */
            if (must_confirm) {
                string onOrOff = is_green ? "off" : "on";
                user_confirmation = MessageBox.Show("Are you sure you want to toggle " + button_name + " " +
                                                    onOrOff + "?", "Confirmation", MessageBoxButton.YesNo,
                                                    MessageBoxImage.Question);
            } else {
                user_confirmation = MessageBoxResult.Yes;
            }

            if (MessageBoxResult.Yes == user_confirmation) {
                is_active = !is_active;

                MainWindow main = (MainWindow)Application.Current.MainWindow;
                main.change_command_string(this, command_character, is_active);

                button.Background = is_green ? dark_red : dark_green;
                is_green = !is_green;
            }
        }

        public void toggle_enabled(bool enab)
        {
            button.IsEnabled = enab;
            button.Opacity = enab ? 1 : 0.3;

            if(!enab) {
                is_active = false;

                MainWindow main = (MainWindow)Application.Current.MainWindow;
                main.change_command_string(this, command_character, false);

                button.Background = default_on ? dark_green : dark_red;
                is_green = default_on;
            }
        }

        public void set_confirmed_state(bool active)
        {
            if (!waiting) {
                is_active = active;

                /*
                 * if (is_active) {
                 *     button.Background = default_on ? red : green;
                 *     if (default_on) is_green = false;
                 *     else is_green = true;
                 * } else {
                 *     button.Background = default_on ? green : red;
                 *     if (default_on) is_green = true;
                 *     else is_green = false;
                 * }
                 */
                /* | is_active | default_on | Background | is_green |
                 * |-----------|------------|------------|----------|
                 * | true      | true       | red        | false    |
                 * | true      | false      | green      | true     |
                 * | false     | true       | green      | true     |
                 * | false     | false      | red        | false    |
                 *
                 * IE, if both conditions are true, set background to red, and 
                 * if one is true and one is false, set background to green.
                 * This is an xnor operation, aka if is_active == default_on
                 * is_green is simply the result of xor(is_active, default_on)
                 */
                button.Background = (is_active == default_on) ? red : green;
                is_green = is_active ^ default_on;
            }
        }
    }
}
