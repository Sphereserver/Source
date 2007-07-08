using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace UoKRUnpacker
{
    static class Program
    {
        /// <summary>
        /// Punto di ingresso principale dell'applicazione.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }

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
        private static extern ZLibError compress2(byte[] dest, ref int destLength, byte[] source, int sourceLength, ZLibQuality quality);

        [DllImport("zlib32")]
        private static extern ZLibError uncompress(byte[] dest, ref int destLen, byte[] source, int sourceLen);

        [DllImport("zlib32")]
        private static extern ulong adler32(ulong adler, byte[] source, uint sourceLen);

        [DllImport("zlib32")]
        private static extern ulong crc32(ulong crc, byte[] source, uint sourceLen);

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

        public static ZLibError Compress(byte[] dest, ref int destLength, byte[] source, int sourceLength, ZLibQuality quality)
        {
            return compress2(dest, ref destLength, source, sourceLength, quality);
        }

        public static ZLibError Decompress(byte[] dest, ref int destLength, byte[] source, int sourceLength)
        {
            return uncompress(dest, ref destLength, source, sourceLength);
        }

        public static ulong Adler32(ulong adler, byte[] source, uint sourceLen)
        {
            return adler32(adler, source, sourceLen);
        }

        public static ulong Crc32(ulong crc, byte[] source, uint sourceLen)
        {
            return crc32(crc, source, sourceLen);
        }

        public static ulong CompressBound(ulong sourceLen)
        {
            return compressBound(sourceLen);
        }
    }
}