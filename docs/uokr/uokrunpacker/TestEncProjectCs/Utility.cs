using System;
using System.Collections.Generic;
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
