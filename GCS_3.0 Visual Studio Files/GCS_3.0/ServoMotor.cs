using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GCS_3._0
{
    class ServoMotor
    {
        int pwm_val;
        public ServoMotor(string servo_name, int init_pwm)
        {
            name = servo_name;
            pwm_val = init_pwm;
        }

        public string name { get; private set; }
        public string pwm { get { return pwm_val.ToString(); } }

        public void set_value(int val) { pwm_val = val; }
    }
}
