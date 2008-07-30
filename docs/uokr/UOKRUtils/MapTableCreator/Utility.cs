using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MapTableCreator
{
    abstract class Utility
    {
        public static ushort Flip16(ushort input)
        {
            byte p = (byte)(input & 0xFF);
            byte s = (byte)((input >> 8) & 0xFF);
            return (ushort)((ushort)s + ((ushort)p << 8));
        }

    }
}
