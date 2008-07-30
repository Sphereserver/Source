using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace MapMULToUOP
{

    public enum ZLibError : int
    {
        VersionError = -6,
        BufferError = -5,
        MemoryError = -4,
        DataError = -3,
        StreamError = -2,
        FileError = -1,

        Okay = 0,

        StreamEnd = 1,
        NeedDictionary = 2
    }

    public enum ZLibQuality : int
    {
        Default = -1,

        None = 0,

        Speed = 1,
        Size = 9
    }

    public class Compressor
    {
        [DllImport("zlib32")]
        private static extern string zlibVersion();

        [DllImport("zlib32")]
        private static extern ulong compressBound(ulong sourceLen);

        [DllImport("zlib32")]
        private static extern ZLibError compress(byte[] dest, ref int destLength, byte[] source, int sourceLength);

        [DllImport("zlib32")]
        private static extern uint adler32(uint adler, byte[] source, uint sourceLen);

        public static string Version
        {
            get
            {
                return zlibVersion();
            }
        }

        public static ZLibError Compress(byte[] dest, ref int destLength, byte[] source, int sourceLength)
        {
            return compress(dest, ref destLength, source, sourceLength);
        }

        private static uint Adler32(uint adler, byte[] source, uint sourceLen)
        {
            return adler32(adler, source, sourceLen);
        }

        public static uint AdlerCheckSum32(byte[] source)
        {
                uint m_adler = Adler32(0, null, 0);
                return Adler32(m_adler,source,(uint)source.Length);
        }

    }
}
