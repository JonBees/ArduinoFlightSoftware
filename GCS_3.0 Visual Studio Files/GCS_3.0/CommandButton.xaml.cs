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

namespace GCS_3._0
{
    /// <summary>
    /// Interaction logic for CommandButton.xaml
    /// </summary>
    public partial class CommandButton : UserControl
    {
        SolidColorBrush darkRed, darkGreen, green, red;
        string buttonName;
        bool mustConfirm;
        public CommandButton()
        {
            darkGreen = new SolidColorBrush(Color.FromArgb(255, 0, 180, 0));
            darkRed = new SolidColorBrush(Color.FromArgb(255, 180, 0, 0));
            red = new SolidColorBrush(Color.FromArgb(255, 255, 0, 0));
            green = new SolidColorBrush(Color.FromArgb(255, 0, 255, 0));

            InitializeComponent();

            mustConfirm = false;
        }

        //This method MUST be called during initialization of the GCS interface, otherwise the command buttons will not function.
        public void continueSetup(string butName, bool defOn, bool confirmation, char comCh)
        {
            IsActive = false;

            DefaultOn = defOn;
            IsGreen = DefaultOn ? true : false;
            button.Background = IsGreen ? darkGreen : darkRed;
            mustConfirm = confirmation;

            CommandCharacter = comCh.ToString();

            buttonName = butName;
        }

        public string CommandCharacter { get; private set; }
        public bool IsActive { get; private set; }

        private void MouseOverEffect(object sender, DependencyPropertyChangedEventArgs e)
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

        public bool Waiting { get; set; }
        public bool DefaultOn { get; private set; }
        private bool IsGreen { get; set; }

        private void Command(object sender, MouseButtonEventArgs e)
        {
            MessageBoxResult userConfirm;

            //If the confirmation message is enabled for the button, present it. Otherwise, continue to the toggling of the button.
            if (mustConfirm)
            {
                string onOrOff = IsGreen ? "off" : "on";
                userConfirm = MessageBox.Show("Are you sure you want to toggle " + buttonName + " " + onOrOff + "?", "Confirmation",
                MessageBoxButton.YesNo, MessageBoxImage.Question);
            }
            else userConfirm = MessageBoxResult.Yes;

            if (userConfirm == MessageBoxResult.Yes)
            {
                IsActive = IsActive ? false : true;

                MainWindow main = (MainWindow)Application.Current.MainWindow;
                main.changeCommandString(this, CommandCharacter, IsActive);

                button.Background = IsGreen ? darkRed : darkGreen;
                IsGreen = IsGreen ? false : true;
            }
        }

        public void ToggleEnabled(bool enab)
        {
            button.IsEnabled = enab;
            button.Opacity = enab ? 1 : 0.3;

            if(!enab)
            {
                IsActive = false;

                MainWindow main = (MainWindow)Application.Current.MainWindow;
                main.changeCommandString(this, CommandCharacter, false);

                button.Background = DefaultOn ? darkGreen : darkRed;
                IsGreen = DefaultOn ? true : false;
            }
        }

        public void setConfirmedState(bool active)
        {
            if (!Waiting)
            {
                IsActive = active;

                if (IsActive)
                {
                    button.Background = DefaultOn ? red : green;
                    if (DefaultOn) IsGreen = false;
                    else IsGreen = true;
                }
                else
                {
                    button.Background = DefaultOn ? green : red;
                    if (DefaultOn) IsGreen = true;
                    else IsGreen = false;
                }
            }
        }
    }
}
