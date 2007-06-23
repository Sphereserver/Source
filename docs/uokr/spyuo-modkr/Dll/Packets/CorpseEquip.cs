using System;
using System.Text;
using System.Collections;

namespace SpyUO.Packets
{
	[PacketInfo( 0x89 )]
	public class CorpseEquip : Packet
	{
		public class CorpseItem
		{
			private byte m_Layer;
			private uint m_Serial;

			public byte Layer { get { return m_Layer; } }
			public uint Serial { get { return m_Serial; } }

			public CorpseItem( byte layer, uint serial )
			{
				m_Layer = layer;
				m_Serial = serial;
			}

			public override string ToString()
			{
				return string.Format( "Layer: \"{0}\", Serial: \"0x{1:X}\"", m_Layer, m_Serial );
			}
		}

		private uint m_Serial;
		private CorpseItem[] m_CorpseItems;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public string CorpseItems
		{
			get
			{
				if ( m_CorpseItems.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					sb.AppendFormat( "{0}", m_CorpseItems[i] );

					if ( ++i < m_CorpseItems.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		public CorpseEquip( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();

			ArrayList list = new ArrayList();
			while ( true )
			{
				byte layer = reader.ReadByte();
				if ( layer == 0 )
					break;

				uint serial = reader.ReadUInt32();

				list.Add( new CorpseItem( (byte)(layer - 1), serial ) );
			}

			m_CorpseItems = (CorpseItem[])list.ToArray( typeof( CorpseItem ) );
		}
	}
}