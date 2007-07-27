using System;
using System.Collections.Generic;
using System.Text;

namespace UoKRUnpacker
{
    class Utility
    {
        public static string GetPathForSave(string sInput)
        {
            return System.Windows.Forms.Application.StartupPath + @"\" + "NEW-" + Utility.GetFileName(sInput);
        }

        public static string GetFileName(string sInput)
        {
            int iStartName = sInput.LastIndexOf('\\') + 1;

            if (iStartName != -1)
            {
                return sInput.Substring(iStartName, sInput.Length - iStartName);
            }
            else
            {
                return sInput;
            }
        }

        public enum HashStringStyle : int
        {
            BigHex = 0,
            SingleHex
        }

        public static string ByteArrayToString(byte[] thearray, HashStringStyle style)
        {
            StringBuilder sbBuffer = new StringBuilder();

            if (style == HashStringStyle.BigHex)
            {
                sbBuffer.Append("0x");
            }

            foreach (byte bSingle in thearray)
            {
                if (style == HashStringStyle.BigHex)
                    sbBuffer.AppendFormat("{0:X}", bSingle);
                else if (style == HashStringStyle.SingleHex)
                    sbBuffer.AppendFormat("0x{0:X} ", bSingle);
            }

            return sbBuffer.ToString().Trim();
        }
    }
}
