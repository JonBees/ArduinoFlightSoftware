using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GCS_3._0
{
    class ServoMotor
    {
        int pwmVal;
        public ServoMotor(string name, int initPWM)
        {
            Name = name;
            pwmVal = initPWM;
        }

        public string Name { get; private set; }
        public string PWM { get { return pwmVal.ToString(); } }

        public void setValue(int val) { pwmVal = val; }
    }
}
