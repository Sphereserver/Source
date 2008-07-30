using System;
using System.IO;

namespace Unpacker.Mythic
{
	public class MythicPackageData
	{
		private int m_Flag;
		private int m_DataOffset;
		private long m_Unknown;		

		public int Flag
		{
			get{ return m_Flag; }
			set{ m_Flag = value; }
		}

		public int DataOffset
		{
			get{ return m_DataOffset; }
			set{ m_DataOffset = value; }
		}

		public long Unknown
		{
			get{ return m_Unknown; }
			set{ m_Unknown = value; }
		}

		public bool Contains( string keyword )
		{			
			string hash = m_Unknown.ToString( "X16" );

			return hash.Contains( keyword );		
		}

		public void Decompress( BinaryReader source, long offset, int clength, int ulength, bool compressed, BinaryWriter destination )
		{
			source.BaseStream.Seek( offset + m_DataOffset + 4, SeekOrigin.Begin );

			byte[] destData = new byte[ ulength ];
			byte[] sourceData = new byte[ clength ]; 
			int destLength = ulength;

			if ( source.Read( sourceData, 0, clength ) != clength )
				throw new Exception( "Error reading data from stream!" );

			if ( compressed )
			{
				ZLibError error = Zlib.Decompress( destData, ref destLength, sourceData, clength );

				if ( error != ZLibError.Okay )
					throw new Exception( String.Format( "Error decompressing: {0}", error ) );
			
				destination.Write( destData, 0, ulength );
			}
			else
				destination.Write( sourceData, 0, clength );
		}

		public static MythicPackageData Read( BinaryReader reader )
		{
			MythicPackageData data = new MythicPackageData();

			data.Flag = reader.ReadInt16();
			data.DataOffset = reader.ReadInt16();
			data.Unknown = reader.ReadInt64();

			return data;
		}
	}
}
