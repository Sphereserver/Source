using System;
using System.IO;
using System.Collections.Generic;

namespace Unpacker.Mythic
{
	public class MythicPackageBlock
	{
		private int m_Index;
		private int m_FileCount;
		private long m_NextBlock;
		private double m_Complete;
		private List<MythicPackageIndex> m_Files;

		public int Index
		{
			get{ return m_Index; }
			set{ m_Index = value; }
		}
		
		public int FileCount
		{
			get{ return m_FileCount; }
			set{ m_FileCount = value; }
		}
		
		public long NextBlock
		{
			get{ return m_NextBlock; }
			set{ m_NextBlock = value; }
		}
		
		public double Complete
		{
			get{ return m_Complete; }
			set{ m_Complete = value; }
		}
		
		public List<MythicPackageIndex> Files
		{
			get{ return m_Files; }
			set{ m_Files = value; }
		}

		public MythicPackageIndex Add( MythicPackageIndex index )
		{
			if ( m_Files == null )
				m_Files = new List<MythicPackageIndex>();

			m_Files.Add( index );
			return index;
		}

		public int Contains( string keyword )
		{
			return Contains( 0, keyword );
		}

		public int Contains( int start, string keyword )
		{
			if ( m_Files == null || m_Files.Count - 1 < start )
				return -1;

			for ( int i = start; i < m_Files.Count; i++ )
			{
				if ( m_Files[ i ].Contains( keyword ) )
					return i;
			}

			return -1;
		}

		public int UpdateFileNames()
		{
			int count = 0;
			int num = 0;
			
			foreach ( MythicPackageIndex idx in m_Files )
			{
				count += idx.UpdateFileName();
				num += 1;
			}

			m_Complete = ( count * 100 ) / (double) num;

			return count;
		}

		public void Decompress( BinaryReader source, string path, string blankName )
		{
			for ( int i = 0; i < m_Files.Count; i++ )
				m_Files[ i ].Decompress( source, path, String.Format( "{0}_{1}", blankName, m_Index ) );
		}

		public static MythicPackageBlock Read( BinaryReader reader, ref int found )
		{
			MythicPackageBlock block = new MythicPackageBlock();
			
			block.FileCount = reader.ReadInt32();
			block.NextBlock = reader.ReadInt64();

			int count = 0;
			int i;

			for ( i = 0; i < block.FileCount; i ++ )
			{
				MythicPackageIndex index = MythicPackageIndex.Read( reader, ref count );

				if ( index.DataBlockOffset == 0x0 )
					continue;

				index.Index = i;
				block.Add( index );
			}

			
			block.Complete = ( count * 100 ) / (double) i;
			found += count;

			return block;
		}
	}
}
