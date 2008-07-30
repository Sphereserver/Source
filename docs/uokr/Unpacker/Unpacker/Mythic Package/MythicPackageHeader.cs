using System;
using System.IO;
using System.Collections.Generic;

namespace Unpacker.Mythic
{
	public class MythicPackageHeader
	{
		private int m_Version;
		private int m_Misc;
		private int m_HeaderSize;
		private int m_BlockSize;
		private int m_FileCount;

		public int Version
		{
			get{ return m_Version; }
			set{ m_Version = value; }
		}

		public int Misc
		{
			get{ return m_Misc; }
			set{ m_Misc = value; }
		}

		public int HeaderSize
		{
			get{ return m_HeaderSize; }
			set{ m_HeaderSize = value; }
		}

		public int BlockSize
		{
			get{ return m_BlockSize; }
			set{ m_BlockSize = value; }
		}

		public int FileCount
		{
			get{ return m_FileCount; }
			set{ m_FileCount = value; }
		}

		public static MythicPackageHeader Read( BinaryReader reader )
		{
			byte[] id = reader.ReadBytes( 4 );

			if ( id[ 0 ] != 'M' || id[ 1 ] != 'Y' || id[ 2 ] != 'P' || id[ 3 ] != 0 )
				throw new Exception( "This is not a Mythic Package file!" );

			MythicPackageHeader h = new MythicPackageHeader();

			h.Version = reader.ReadInt32();
			h.Misc = reader.ReadInt32(); // must be 0xFD23EC43
			h.HeaderSize = reader.ReadInt32();

			reader.ReadBytes( 4 ); // 0x0000000000

			h.BlockSize = reader.ReadInt32();
			h.FileCount = reader.ReadInt32();

			reader.ReadBytes( h.HeaderSize - 28 ); // 0x00...

			return h;
		}
	}
}
