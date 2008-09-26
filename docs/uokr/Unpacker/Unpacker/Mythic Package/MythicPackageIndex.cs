using System;
using System.IO;

namespace Unpacker.Mythic
{
	public class MythicPackageIndex
	{
		private int m_Index;
		private MythicPackageData m_DataBlock;
		private long m_DataBlockOffset;
		private int m_DataLength;
		private int m_CompressedSize;
		private int m_DecompressedSize;
		private long m_FileHash;
		private string m_FileName;
		private int m_Unknown;
		private bool m_Compressed;

		public int Index
		{
			get{ return m_Index; }
			set{ m_Index = value; }
		}

		public MythicPackageData DataBlock
		{
			get{ return m_DataBlock; }
			set{ m_DataBlock = value; }
		}

		public long DataBlockOffset
		{
			get{ return m_DataBlockOffset; }
			set{ m_DataBlockOffset = value; }
		}

		public int DataLength
		{
			get{ return m_DataLength; }
			set{ m_DataLength = value; }
		}

		public int CompressedSize
		{
			get{ return m_CompressedSize; }
			set{ m_CompressedSize = value; }
		}

		public int DecompressedSize
		{
			get{ return m_DecompressedSize; }
			set{ m_DecompressedSize = value; }
		}

		public long FileHash
		{
			get{ return m_FileHash; }
			set{ m_FileHash = value; }
		}

		public string FileName
		{
			get{ return m_FileName; }
			set{ m_FileName = value; }
		}

		public int CRC
		{
			get{ return m_Unknown; }
			set{ m_Unknown = value; }
		}

		public bool Compressed
		{
			get{ return m_Compressed; }
			set{ m_Compressed = value; }
		}

		public override string ToString()
		{
			if ( m_FileName != null )
				return Path.GetFileName( m_FileName );

			return String.Format( "Index_{0}", m_Index );
		}

		public bool Contains( string keyword )
		{
			if ( m_FileName != null && m_FileName.Contains( keyword ) )
				return true;

			string hash = m_FileHash.ToString( "X16" );

			if ( hash.Contains( keyword ) )
				return true;

			if ( m_FileName == null && m_FileHash == HashDictionary.HashMeGently( keyword ) )
			{
				HashDictionary.Set( m_FileHash, keyword );
				m_FileName = keyword;
				return true;
			}

			if ( m_DataBlock != null )
				return m_DataBlock.Contains( keyword );

			return false;				
		}

		public int UpdateFileName()
		{
			if ( m_FileName != null )
				return 1;

			m_FileName = HashDictionary.Get( m_FileHash );
			
			if ( m_FileName != null )
				return 1;

			return 0;
		}

		public void Decompress( BinaryReader source, string path, string blankName )
		{
			if ( m_DataBlock != null )
			{
				string fileName = m_FileName;

				if ( fileName == null )
				{
					if ( blankName == null )
						return;

					fileName = String.Format( "{0}_{1}.dat", blankName, m_Index );
				}

				if ( !Directory.Exists( path + '\\' + fileName ) )
				{
					string dir = path + '\\' + Path.GetDirectoryName( fileName );

					if ( !String.IsNullOrEmpty( dir ) )
						Directory.CreateDirectory( dir );
				}

				using ( FileStream stream = File.Create( path + '\\' + fileName ) )
				{
					using ( BinaryWriter writer = new BinaryWriter( stream ) )
					{
						m_DataBlock.Decompress( source, m_DataBlockOffset, m_CompressedSize, m_DecompressedSize, m_Compressed, writer );
					}
				}
			}
		}

		public static MythicPackageIndex Read( BinaryReader reader, ref int found )
		{
			MythicPackageIndex index = new MythicPackageIndex();

			index.DataBlockOffset = reader.ReadInt64();
			index.DataLength = reader.ReadInt32();
			index.CompressedSize = reader.ReadInt32();
			index.DecompressedSize = reader.ReadInt32();
			index.FileHash = reader.ReadInt64();
			index.FileName = HashDictionary.Get( index.FileHash );			
			index.CRC = reader.ReadInt32();

			if ( index.FileName != null )
				found += 1;

			short flag = reader.ReadInt16();

			switch ( flag )
			{
				case 0x0: index.Compressed = false; break;
				case 0x1: index.Compressed = true; break;
				default: throw new Exception( String.Format( "Invalid compressed flag: '{0}'!", flag ) );
			}

			if ( index.DataBlockOffset != 0 )
			{
				long position = reader.BaseStream.Position;
				reader.BaseStream.Seek( index.DataBlockOffset, SeekOrigin.Begin );
				index.DataBlock = MythicPackageData.Read( reader );
				reader.BaseStream.Seek( position, SeekOrigin.Begin );
			}

			return index;
		}
	}
}
