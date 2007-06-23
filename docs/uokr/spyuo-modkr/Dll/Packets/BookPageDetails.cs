using System;
using System.Text;
using System.Collections;

namespace SpyUO.Packets
{
	[PacketInfo( 0x66 )]
	public class BookPageDetails : Packet
	{
		public class BookPageInfo
		{
			private int m_Index;
			private bool m_Request;
			private string[] m_Lines;

			public int Index{ get{ return m_Index; } }
			public bool Request{ get{ return m_Request; } }
			public string[] Lines{ get{ return m_Lines; } }

			public bool IsEmpty{ get{ return m_Lines.Length == 0; } }

			public BookPageInfo( int index, bool request, string[] lines )
			{
				m_Index = index;
				m_Request = request;
				m_Lines = lines;
			}
		}

		private uint m_Serial;
		private BookPageInfo[] m_Pages;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial{ get{ return m_Serial; } }

		public BookPageInfo[] Pages{ get{ return m_Pages; } }

		[PacketProp( 1 )]
		public string Text
		{
			get
			{
				StringBuilder builder = new StringBuilder();

				for ( int i = 0; i < m_Pages.Length; i++ )
				{
					builder.AppendFormat( "[Page {0}{1}] ", m_Pages[i].Index, m_Pages[i].Request ? " req" : "" );

					if ( m_Pages[i].IsEmpty )
						continue;

					for ( int j = 0; ; )
					{
						builder.Append( m_Pages[i].Lines[j] + " " );

						if ( ++j < m_Pages[i].Lines.Length )
							builder.Append( "| " );
						else
							break;
					}
				}

				return builder.ToString();
			}
		}

		public BookPageDetails( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();

			int pagesCount = reader.ReadUInt16();
			m_Pages = new BookPageInfo[pagesCount];

			for ( int i = 0; i < m_Pages.Length; i++ )
			{
				int index = reader.ReadUInt16();

				int linesCount = reader.ReadInt16();
				if ( linesCount < 0 )
				{
					m_Pages[i] = new BookPageInfo( index, true, new string[0] );
					continue;
				}

				string[] lines = new string[linesCount];

				for ( int j = 0; j < linesCount; j++ )
				{
					ArrayList buffer = new ArrayList();

					while ( true )
					{
						byte b = reader.ReadByte();

						if ( b != 0 )
						{
							buffer.Add( b );
						}
						else
						{
							byte[] bytes = (byte[])buffer.ToArray( typeof( byte ) );
							lines[j] = Encoding.UTF8.GetString( bytes );

							break;
						}
					}
				}

				m_Pages[i] = new BookPageInfo( index, false, lines );
			}
		}
	}
}