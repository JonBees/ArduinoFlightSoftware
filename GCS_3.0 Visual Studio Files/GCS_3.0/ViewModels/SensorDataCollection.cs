using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Research.DynamicDataDisplay.Common;

namespace GCS_3._0.ViewModels
{
    public class SensorDataCollection : RingArray<SensorDataPoint>
    {
        private const int TOTAL_POINTS = 10;    //Number of values before cycled

        public SensorDataCollection()
            : base(TOTAL_POINTS) 
        {
        }
    }

    public class SensorDataPoint
    {
        public DateTime Date { get; set; }

        public double SensorData { get; set; }

        public SensorDataPoint(double value, DateTime date)
        {
            this.Date = date;
            this.SensorData = value;
        }
    }
}
