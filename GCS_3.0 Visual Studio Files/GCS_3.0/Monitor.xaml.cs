using System.Windows;
using System.Windows.Threading;
using System;
using Microsoft.Research.DynamicDataDisplay.DataSources;
using Microsoft.Research.DynamicDataDisplay;
using Microsoft.Research.DynamicDataDisplay.Navigation;
using Microsoft.Research.DynamicDataDisplay.PointMarkers;
using System.Windows.Media;
using System.ComponentModel;
using GCS_3._0.ViewModels;
using System.Collections.Generic;

namespace GCS_3._0
{
    /// <summary>
    /// Logica di interazione per MainWindow.xaml
    /// </summary>
    public partial class Monitor : Window, INotifyPropertyChanged
    {
        private int _minVoltage;
        private int _maxVoltage;
        private int _minCurrent;
        private int _maxCurrent;
        public SensorDataCollection voltagePointCollection;
        public SensorDataCollection currentPointCollection;
        public SensorDataCollection av1PointCollection;
        public SensorDataCollection av2PointCollection;
        public SensorDataCollection av3PointCollection;
        public SensorDataCollection av4PointCollection;
        public SensorDataCollection pt1PointCollection;
        public SensorDataCollection pt2PointCollection;
        public SensorDataCollection pt3PointCollection;
        public SensorDataCollection pt4PointCollection;
        public SensorDataCollection pt5PointCollection;
        public SensorDataCollection pt6PointCollection;
        public SensorDataCollection pt7PointCollection;
        public SensorDataCollection tc1PointCollection;
        public SensorDataCollection tc2PointCollection;
        public SensorDataCollection tc3PointCollection;
        public SensorDataCollection tc4PointCollection;
        public SensorDataCollection tc5PointCollection;
        public SensorDataCollection tc6PointCollection;

        public Monitor()
        {
            InitializeComponent();
            this.DataContext = this;

            initializeCollections();
            initializeGraphs();
        }

        void initializeCollections()
        {
            voltagePointCollection = new SensorDataCollection();
            currentPointCollection = new SensorDataCollection();
            av1PointCollection = new SensorDataCollection();
            av2PointCollection = new SensorDataCollection();
            av3PointCollection = new SensorDataCollection();
            av4PointCollection = new SensorDataCollection();
            pt1PointCollection = new SensorDataCollection();
            pt2PointCollection = new SensorDataCollection();
            pt3PointCollection = new SensorDataCollection();
            pt4PointCollection = new SensorDataCollection();
            pt5PointCollection = new SensorDataCollection();
            pt6PointCollection = new SensorDataCollection();
            pt7PointCollection = new SensorDataCollection();
            tc1PointCollection = new SensorDataCollection();
            tc2PointCollection = new SensorDataCollection();
            tc3PointCollection = new SensorDataCollection();
            tc4PointCollection = new SensorDataCollection();
            tc5PointCollection = new SensorDataCollection();
            tc6PointCollection = new SensorDataCollection();
        }

        void initializeGraphs()
        {
            var tc1ds = new EnumerableDataSource<SensorDataPoint>(tc1PointCollection);
            tc1ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc1ds.SetYMapping(y => y.SensorData);
            var tc2ds = new EnumerableDataSource<SensorDataPoint>(tc2PointCollection);
            tc2ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc2ds.SetYMapping(y => y.SensorData);
            var tc3ds = new EnumerableDataSource<SensorDataPoint>(tc3PointCollection);
            tc3ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc3ds.SetYMapping(y => y.SensorData);
            var tc4ds = new EnumerableDataSource<SensorDataPoint>(tc4PointCollection);
            tc4ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc4ds.SetYMapping(y => y.SensorData);
            var tc5ds = new EnumerableDataSource<SensorDataPoint>(tc5PointCollection);
            tc5ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc5ds.SetYMapping(y => y.SensorData);
            var tc6ds = new EnumerableDataSource<SensorDataPoint>(tc6PointCollection);
            tc6ds.SetXMapping(x => dateAxis1.ConvertToDouble(x.Date));
            tc6ds.SetYMapping(y => y.SensorData);
            temperaturePlotter.AddLineGraph(tc5ds, Colors.Red, 2, "TCp1-M");
            temperaturePlotter.AddLineGraph(tc6ds, Colors.Orange, 2, "TCp2-M");
            temperaturePlotter.AddLineGraph(tc1ds, Colors.Yellow, 2, "TCw1-E");
            temperaturePlotter.AddLineGraph(tc2ds, Colors.Green, 2, "TCw2-E");
            temperaturePlotter.AddLineGraph(tc3ds, Colors.Blue, 2, "TCw3-E");
            temperaturePlotter.AddLineGraph(tc4ds, Colors.Violet, 2, "TCw4-E");

            var pt1ds = new EnumerableDataSource<SensorDataPoint>(pt1PointCollection);
            pt1ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt1ds.SetYMapping(y => y.SensorData);
            var pt2ds = new EnumerableDataSource<SensorDataPoint>(pt2PointCollection);
            pt2ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt2ds.SetYMapping(y => y.SensorData);
            var pt3ds = new EnumerableDataSource<SensorDataPoint>(pt3PointCollection);
            pt3ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt3ds.SetYMapping(y => y.SensorData);
            var pt4ds = new EnumerableDataSource<SensorDataPoint>(pt4PointCollection);
            pt4ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt4ds.SetYMapping(y => y.SensorData);
            var pt5ds = new EnumerableDataSource<SensorDataPoint>(pt5PointCollection);
            pt5ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt5ds.SetYMapping(y => y.SensorData);
            var pt6ds = new EnumerableDataSource<SensorDataPoint>(pt6PointCollection);
            pt6ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt6ds.SetYMapping(y => y.SensorData);
            var pt7ds = new EnumerableDataSource<SensorDataPoint>(pt7PointCollection);
            pt7ds.SetXMapping(x => dateAxis2.ConvertToDouble(x.Date));
            pt7ds.SetYMapping(y => y.SensorData);
            pressurePlotter.AddLineGraph(pt4ds, Colors.Red, 2, "PT1-U");
            pressurePlotter.AddLineGraph(pt5ds, Colors.Orange, 2, "PT2-U");
            pressurePlotter.AddLineGraph(pt2ds, Colors.Yellow, 2, "PT1-M");
            pressurePlotter.AddLineGraph(pt6ds, Colors.Green, 2, "PT2-M");
            pressurePlotter.AddLineGraph(pt3ds, Colors.Blue, 2, "PT3-M");
            pressurePlotter.AddLineGraph(pt1ds, Colors.Violet, 2, "PT4-M");
            pressurePlotter.AddLineGraph(pt7ds, Colors.Black, 2, "PT5-M");

            var vds = new EnumerableDataSource<SensorDataPoint>(voltagePointCollection);   //Voltage Data Source
            vds.SetXMapping(x => dateAxis3.ConvertToDouble(x.Date));
            vds.SetYMapping(y => y.SensorData);
            voltagePlotter.AddLineGraph(vds, Colors.Green, 2, "Voltage"); //Plot Voltage
            
            var cds = new EnumerableDataSource<SensorDataPoint>(currentPointCollection);   //Current Data Source
            cds.SetXMapping(x => dateAxis4.ConvertToDouble(x.Date));
            cds.SetYMapping(y => y.SensorData);
            currentPlotter.AddLineGraph(cds, Colors.Red, 2, "Current");   //Plot Current
            
            var av1ds = new EnumerableDataSource<SensorDataPoint>(av1PointCollection);   
            av1ds.SetXMapping(x => dateAxis5.ConvertToDouble(x.Date));
            av1ds.SetYMapping(y => y.SensorData);
            var av2ds = new EnumerableDataSource<SensorDataPoint>(av2PointCollection);   
            av2ds.SetXMapping(x => dateAxis5.ConvertToDouble(x.Date));
            av2ds.SetYMapping(y => y.SensorData);
            var av3ds = new EnumerableDataSource<SensorDataPoint>(av2PointCollection);   
            av3ds.SetXMapping(x => dateAxis5.ConvertToDouble(x.Date));
            av3ds.SetYMapping(y => y.SensorData);
            var av4ds = new EnumerableDataSource<SensorDataPoint>(av2PointCollection);   
            av4ds.SetXMapping(x => dateAxis5.ConvertToDouble(x.Date));
            av4ds.SetYMapping(y => y.SensorData);
            motorPlotter.AddLineGraph(av1ds, Colors.Red, 2, "AV1-M");
            motorPlotter.AddLineGraph(av2ds, Colors.Blue, 2, "AV2-M");
            motorPlotter.AddLineGraph(av3ds, Colors.Green, 2, "AV3-M");
            motorPlotter.AddLineGraph(av4ds, Colors.Yellow, 2, "AV4-M");
        }


        public void updateVoltage(double v)
        {
            voltagePointCollection.Add(new SensorDataPoint(v, DateTime.Now));
        }
        public void updateCurrent(double c)
        {
            currentPointCollection.Add(new SensorDataPoint(c, DateTime.Now));
        }
        public void updateMotors(List<int> motors)
        {
            av1PointCollection.Add(new SensorDataPoint(motors[0], DateTime.Now));
            av2PointCollection.Add(new SensorDataPoint(motors[1], DateTime.Now));
            av3PointCollection.Add(new SensorDataPoint(motors[2], DateTime.Now));
            av4PointCollection.Add(new SensorDataPoint(motors[3], DateTime.Now));
        }
        public void updatePressures(List<double> pressures)
        {
            pt1PointCollection.Add(new SensorDataPoint(pressures[0], DateTime.Now));
            pt2PointCollection.Add(new SensorDataPoint(pressures[1], DateTime.Now));
            pt3PointCollection.Add(new SensorDataPoint(pressures[2], DateTime.Now));
            pt4PointCollection.Add(new SensorDataPoint(pressures[3], DateTime.Now));
            pt5PointCollection.Add(new SensorDataPoint(pressures[4], DateTime.Now));
            pt6PointCollection.Add(new SensorDataPoint(pressures[5], DateTime.Now));
            pt7PointCollection.Add(new SensorDataPoint(pressures[6], DateTime.Now));
        }
        public void updateTemperatures(List<double> temperatures)
        {
            tc1PointCollection.Add(new SensorDataPoint(temperatures[0], DateTime.Now));
            tc2PointCollection.Add(new SensorDataPoint(temperatures[1], DateTime.Now));
            tc3PointCollection.Add(new SensorDataPoint(temperatures[2], DateTime.Now));
            tc4PointCollection.Add(new SensorDataPoint(temperatures[3], DateTime.Now));
            tc5PointCollection.Add(new SensorDataPoint(temperatures[4], DateTime.Now));
            tc6PointCollection.Add(new SensorDataPoint(temperatures[5], DateTime.Now));
        }

        #region INotifyPropertyChanged members

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                this.PropertyChanged(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }


        //MIN AND MAX GET AND SET
        public int MaxVoltage
        {
            get { return _maxVoltage; }
            set { _maxVoltage = value; this.OnPropertyChanged("MaxVoltage"); }
        }

        
        public int MinVoltage
        {
            get { return _minVoltage; }
            set { _minVoltage = value; this.OnPropertyChanged("MinVoltage"); }
        }

        public int MaxCurrent
        {
            get { return _maxCurrent; }
            set { _maxCurrent = value; this.OnPropertyChanged("MaxCurrent"); }
        }
        public int MinCurrent
        {
            get { return _minCurrent; }
            set { _minCurrent = value; this.OnPropertyChanged("MinCurrent"); }
        }

        private int _maxMotor;
        public int MaxMotor
        {
            get { return _maxMotor; }
            set { _maxMotor = value; this.OnPropertyChanged("MaxMotor"); }
        }

        private int _minMotor;
        public int MinMotor
        {
            get { return _minMotor; }
            set { _minMotor = value; this.OnPropertyChanged("MinMotor"); }
        }

        private int _maxTemperature;
        public int MaxTemperature
        {
            get { return _maxTemperature; }
            set { _maxTemperature = value; this.OnPropertyChanged("MaxTemperature"); }
        }

        private int _minTemperature;
        public int MinTemperature
        {
            get { return _minTemperature; }
            set { _minTemperature = value; this.OnPropertyChanged("MinTemperature"); }
        }

        private int _maxPressure;
        public int MaxPressure
        {
            get { return _maxPressure; }
            set { _maxPressure = value; this.OnPropertyChanged("MaxPressure"); }
        }

        private int _minPressure;
        public int MinPressure
        {
            get { return _minPressure; }
            set { _minPressure = value; this.OnPropertyChanged("MinPressure"); }
        }

        #endregion
    }
}