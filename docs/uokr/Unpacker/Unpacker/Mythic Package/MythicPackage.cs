using System;
using System.IO;
using System.Collections.Generic;

namespace Unpacker.Mythic
{
	public class MythicPackage : IDisposable
	{
		private FileInfo m_Info;
		private MythicPackageHeader m_Header;
		private List<MythicPackageBlock> m_Blocks;
		private double m_Complete;
		
		public FileInfo Info
		{
			get{ return m_Info; }
			set{ m_Info = value; }
		}
		
		public MythicPackageHeader Header
		{
			get{ return m_Header; }
			set{ m_Header = value; }
		}
		
		public List<MythicPackageBlock> Blocks
		{
			get{ return m_Blocks; }
			set{ m_Blocks = value; }
		}
		
		public double Complete
		{
			get{ return m_Complete; }
			set{ m_Complete = value; }
		}

		public void Dispose()
		{
			m_Blocks.Clear();
		}

		public MythicPackageBlock Add( MythicPackageBlock block )
		{
			if ( m_Blocks == null )
				m_Blocks = new List<MythicPackageBlock>();

			m_Blocks.Add( block );
			return block;
		}

		public int[] Contains( string keyword )
		{
			return Contains( 0, 0, keyword );
		}

		public int[] Contains( int bstart, int istart, string keyword )
		{
			if ( m_Blocks == null || m_Blocks.Count - 1 < bstart )
				return null;

			int idx = m_Blocks[ bstart ].Contains( istart, keyword );

			if ( idx > -1 )
				return new int[] { bstart, idx };

			for ( int i = bstart; i < m_Blocks.Count; i++ )
			{
				idx = m_Blocks[ i ].Contains( keyword );

				if ( idx > - 1 )
					return new int[] { i, idx };
			}

			return null;
		}

		public void UpdateFileNames()
		{
			int count = 0;

			foreach ( MythicPackageBlock b in m_Blocks )
				count += b.UpdateFileNames();

			m_Complete = ( count * 100 ) / (double) m_Header.FileCount;
		}

		public void Decompress( string path, string blankName )
		{
			using ( FileStream stream = File.OpenRead( m_Info.FullName ) )
			{
				using ( BinaryReader reader = new BinaryReader( stream ) )
				{					
					for ( int i = 0; i < m_Blocks.Count; i++ )
						m_Blocks[ i ].Decompress( reader, path, blankName );
				}
			}
		}

		public static MythicPackage Read( string fileName )
		{
			using ( FileStream stream = new FileStream( fileName, FileMode.Open ) )
			{
				using ( BinaryReader reader = new BinaryReader( stream ) )
                {
                    MythicPackage myp = new MythicPackage();					
					int found = 0;
					int index = 0;

					myp.Info = new FileInfo( fileName );
					myp.Header = MythicPackageHeader.Read( reader );

					MythicPackageBlock block = myp.Add( MythicPackageBlock.Read( reader, ref found ) );
					block.Index = 0;

					while ( stream.Seek( block.NextBlock, SeekOrigin.Begin ) != 0 )
					{
						block = myp.Add( MythicPackageBlock.Read( reader, ref found ) );
						block.Index = index + 1;
						index += 1;
					}

					myp.Complete = ( found * 100 ) / (double) myp.Header.FileCount;

					return myp;
                }
			}
		}
	}
}
