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


/*
// QWORD 1 Hashcode (HashMeGently(filename))

public const uint SEED = 0xDEADBEEF;

public static ulong HashMeGently( string s )
{
	uint eax, ecx, edx, ebx, esi, edi;

	eax = ecx = edx = ebx = esi = edi = 0;
	ebx = edi = esi = (uint) s.Length + SEED;

	int i = 0;

	for ( i = 0; i + 12 < s.Length; i += 12 )
	{
		edi = (uint) ( ( s[ i + 7 ] << 24 ) | ( s[ i + 6 ] << 16 ) | ( s[ i + 5 ] << 8 ) | s[ i + 4 ] ) + edi;
		esi = (uint) ( ( s[ i + 11 ] << 24 ) | ( s[ i + 10 ] << 16 ) | ( s[ i + 9 ] << 8 ) | s[ i + 8 ] ) + esi;
		edx = (uint) ( ( s[ i + 3 ] << 24 ) | ( s[ i + 2 ] << 16 ) | ( s[ i + 1 ] << 8 ) | s[ i ] ) - esi;

		edx = ( edx + ebx ) ^ ( esi >> 28 ) ^ ( esi << 4 );
		esi += edi;
		edi = ( edi - edx ) ^ ( edx >> 26 ) ^ ( edx << 6 );
		edx += esi;
		esi = ( esi - edi ) ^ ( edi >> 24 ) ^ ( edi << 8 );
		edi += edx;
		ebx = ( edx - esi ) ^ ( esi >> 16 ) ^ ( esi << 16 );
		esi += edi;
		edi = ( edi - ebx ) ^ ( ebx >> 13 ) ^ ( ebx << 19 );
		ebx += esi;
		esi = ( esi - edi ) ^ ( edi >> 28 ) ^ ( edi << 4 );
		edi += ebx;
	}

	if ( s.Length - i > 0 )
	{
		switch ( s.Length - i )
		{
			case 12:
				esi += (uint) s[ i + 11 ] << 24;
				goto case 11;
			case 11:
				esi += (uint) s[ i + 10 ] << 16;
				goto case 10;
			case 10:
				esi += (uint) s[ i + 9 ] << 8;
				goto case 9;
			case 9:
				esi += (uint) s[ i + 8 ];
				goto case 8;
			case 8:
				edi += (uint) s[ i + 7 ] << 24;
				goto case 7;
			case 7:
				edi += (uint) s[ i + 6 ] << 16;
				goto case 6;
			case 6:
				edi += (uint) s[ i + 5 ] << 8;
				goto case 5;
			case 5:
				edi += (uint) s[ i + 4 ];
				goto case 4;
			case 4:
				ebx += (uint) s[ i + 3 ] << 24;
				goto case 3;
			case 3:
				ebx += (uint) s[ i + 2 ] << 16;
				goto case 2;
			case 2:
				ebx += (uint) s[ i + 1 ] << 8;
				goto case 1;
			case 1:		
				ebx += (uint) s[ i ];
				break;			
		}

		esi = ( esi ^ edi ) - ( ( edi >> 18 ) ^ ( edi << 14 ) );
		ecx = ( esi ^ ebx ) - ( ( esi >> 21 ) ^ ( esi << 11 ) );
		edi = ( edi ^ ecx ) - ( ( ecx >> 7 ) ^ ( ecx << 25 ) );
		esi = ( esi ^ edi ) - ( ( edi >> 16 ) ^ ( edi << 16 ) );
		edx = ( esi ^ ecx ) - ( ( esi >> 28 ) ^ ( esi << 4 ) );
		edi = ( edi ^ edx ) - ( ( edx >> 18 ) ^ ( edx << 14 ) );
		eax = ( esi ^ edi ) - ( ( edi >> 8 ) ^ ( edi << 24 ) );

		return ( (ulong) edi << 32 ) | eax;
	}

	return ( (ulong) esi << 32 ) | eax;
}
*/