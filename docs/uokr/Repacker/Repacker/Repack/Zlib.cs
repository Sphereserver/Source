using System;
using System.Runtime.InteropServices;

namespace Zlib
{
    public enum ZLibQuality
    {
        None		= 0,
        Speed		= 1,
        Best		= 9,
        Default		= -1,
    }

	public enum ZLibError
	{
		Okay			= 0,
		StreamEnd		= 1,
		NeedDictionary	= 2,
		FileError		= -1,
		StreamError		= -2,
		DataError		= -3,
		MemoryError		= -4,
		BufferError		= -5,
		VersionError	= -6,
	}

	public class Zlib
	{
        [DllImport("zlib32")]
        private static extern ZLibError uncompress( byte[] dest, ref int destLen, byte[] source, int sourceLen );

		public static ZLibError Decompress( byte[] dest, ref int destLength, byte[] source, int sourceLength )
        {
            return uncompress( dest, ref destLength, source, sourceLength );
        }

        [DllImport("zlib32")]
        private static extern ZLibError compress(byte[] dest, ref int destLen, byte[] source, int sourceLen);

        public static ZLibError Compress(byte[] dest, ref int destLength, byte[] source, int sourceLength)
        {
            return compress(dest, ref destLength, source, sourceLength);
        }

        [DllImport("zlib32")]
        private static extern string zlibVersion();

        public static string Version{ get{ return zlibVersion(); } }
	}
}
