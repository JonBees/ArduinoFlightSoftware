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
    /// Interaction logic for SecSafetyButton.xaml
    /// </summary>
    public partial class SecSafetyButton : UserControl
    {
        SolidColorBrush green, red;
        public SecSafetyButton()
        {
            red = new SolidColorBrush(Color.FromArgb(255, 255, 0, 0));
            green = new SolidColorBrush(Color.FromArgb(255, 0, 255, 0));

            InitializeComponent();

            button.Background = green;
        }

        private void Toggle(object sender, MouseButtonEventArgs e)
        {
            /* IsActive = IsActive ? false : true; */
	    IsActive = !IsActive;
            button.Background = IsActive ? green : red;

            MainWindow main = (MainWindow)Application.Current.MainWindow;
            main.toggle_takeoff_enabled(!IsActive);
        }

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

        public bool IsActive { get; private set; }
    }
}
