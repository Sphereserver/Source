using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace UoKRUnpacker
{
    public interface StupidInterface
    {
        void SetLoadIcon(bool bStatus);
        void DisableOtherIcon(bool bEnable);
        void SetTextArea(string sText);
        void SetNodes();
        void SetCursor(System.Windows.Forms.Cursor cCursore);
        void SetWaitStatus(bool bEnable);
    }

    public class ThreadArgs
    {
        private object[] m_Args;
        public delegate void MyThreadDelegate(object args);

        public ThreadArgs()
        {
            m_Args = null;
        }

        public ThreadArgs(object[] args)
        {
            m_Args = args;
        }

        public object[] Args
        {
            get { return m_Args; }
            set { m_Args = value; }
        }
    }

    public class DoingSomeJob
    {
        private static bool sm_Working = false;
        private static StupidInterface sm_Interface = null;

        public static bool Working
        {
            get { return sm_Working; }
            set 
            { 
                sm_Working = value;
                if (sm_Interface != null)
                {
                    sm_Interface.SetWaitStatus(sm_Working);
                }
            }
        }

        public static StupidInterface TheForm
        {
            set { sm_Interface = value; }
        }
    }

    class StaticData
    {
        public const string UNPACK_DIR = @"\Unpacked";
        public const string DUMPINFO_DIR = @"\Dumpinfo";
        public const string UNPACK_NAMEPATTERN = "{0}-{1:00}_{2:00}.{3}";
        public const string UNPACK_EXT_COMP = "dat";
        public const string UNPACK_EXT_UCOMP = "raw";
        public const string HELP_STRING = "This program can view, parse, dump and patch an uop file through a stupid interface." + "\n" +
                                          "This program is not optimized and/or written using a decent method." + "\n" +
                                          "This program is absolutely not for newbies." + "\n" +
                                          "\n" + "\n" +
                                          "It contains also a console mode to patch (multiple) file without the interface (--help for more info)," + "\n" +
                                          "it can be useful for batch process.";
    }

    class Utility
    {
        public static string GetPathForSave(string sInput)
        {
            return System.Windows.Forms.Application.StartupPath + @"\" + "NEW-" + Utility.GetFileName(sInput);
        }

        public static string GetFileName(string sInput)
        {
            return System.IO.Path.GetFileName(sInput);
        }

        public enum HashStringStyle : int
        {
            BigHex = 0,
            SingleHex,
            HexWithSeparator
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
                else if (style == HashStringStyle.HexWithSeparator)
                    sbBuffer.AppendFormat("{0:X} ", bSingle);
            }

            string toReturn = sbBuffer.ToString().Trim();

            if (style == HashStringStyle.HexWithSeparator)
            {
                toReturn = toReturn.Replace(' ', '-');
            }

            return toReturn;
        }

        public static byte[] StringToByteArray(string thearray, HashStringStyle style, int expectedLength)
        {
            byte[] toReturn = null;

            if (expectedLength > 0)
                toReturn = new byte[expectedLength];

            if (style == HashStringStyle.BigHex)
            {
                // sbBuffer.AppendFormat("{0:X}", bSingle);
            }
            else if (style == HashStringStyle.SingleHex)
            {
                // sbBuffer.AppendFormat("0x{0:X} ", bSingle);
            }
            else if (style == HashStringStyle.HexWithSeparator)
            {
                string[] separated = thearray.Split(new char[] { '-' });
                if (expectedLength > 0)
                {
                    if (expectedLength == separated.Length)
                    {
                        for(int i = 0; i < separated.Length; i++)
                        {
                            if (!Byte.TryParse(separated[i], NumberStyles.AllowHexSpecifier, null, out toReturn[i]))
                                return null; // error
                        }
                    }
                }
                else
                {
                    toReturn = new byte[separated.Length];
                    for (int i = 0; i < separated.Length; i++)
                    {
                        if (!Byte.TryParse(separated[i], NumberStyles.AllowHexSpecifier, null, out toReturn[i]))
                            return null; // error
                    }
                }
            }

            return toReturn;
        }

        public static string StringFileSize(uint uSize)
        {
            if (uSize > 1000000)
                return (uSize / 1000000).ToString() + " Mb";
            else if (uSize > 1000)
                return (uSize / 1000).ToString() + " Kb";
            else
                return uSize.ToString() + " bytes";
        }
    }
}
