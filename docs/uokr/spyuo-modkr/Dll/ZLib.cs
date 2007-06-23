using System;
using System.Runtime.InteropServices;

namespace SpyUO
{
	public enum ZLibError : int
	{
		Z_OK = 0,
		Z_STREAM_END = 1,
		Z_NEED_DICT = 2,
		Z_ERRNO = (-1),
		Z_STREAM_ERROR = (-2),
		Z_DATA_ERROR = (-3),
		Z_MEM_ERROR = (-4),
		Z_BUF_ERROR = (-5),
		Z_VERSION_ERROR = (-6),
	}

	public enum ZLibCompressionLevel : int
	{
		Z_NO_COMPRESSION = 0,
		Z_BEST_SPEED = 1,
		Z_BEST_COMPRESSION = 9,
		Z_DEFAULT_COMPRESSION = (-1)
	}

	public class ZLib
	{
		[DllImport( "zlib" )]
		public static extern string zlibVersion();

		[DllImport( "zlib" )]
		public static extern ZLibError compress( byte[] dest, ref int destLength, byte[] source, int sourceLength );

		[DllImport( "zlib" )]
		public static extern ZLibError compress2( byte[] dest, ref int destLength, byte[] source, int sourceLength, ZLibCompressionLevel level );

		[DllImport( "zlib" )]
		public static extern ZLibError uncompress( byte[] dest, ref int destLen, byte[] source, int sourceLen );
	}
}
