using System;
using System.Text;

namespace SpyUO.Packets
{
	[PacketInfo( 0x3C )]
	public class ContainerContent : Packet
	{
		public class ContainedItem
		{
			private uint m_Serial;
			private ushort m_ItemId;
			private ushort m_Amount;
			private short m_X;
			private short m_Y;
			private uint m_ContSerial;
			private ushort m_Hue;

			public uint Serial { get { return m_Serial; } }
			public ushort ItemId { get { return m_ItemId; } }
			public ushort Amount { get { return m_Amount; } }
			public short X { get { return m_X; } }
			public short Y { get { return m_Y; } }
			public uint ContSerial { get { return m_ContSerial; } }
			public ushort Hue { get { return m_Hue; } }

			public string ItemIdName
			{
				get
				{
					try
					{
						return Ultima.TileData.ItemTable[m_ItemId].Name;
					}
					catch
					{
						return null;
					}
				} 
			}

			public ContainedItem( uint serial, ushort itemId, ushort amount,
				short x, short y, uint contSerial, ushort hue )
			{
				m_Serial = serial;
				m_ItemId = itemId;
				m_Amount = amount;
				m_X = x;
				m_Y = y;
				m_ContSerial = contSerial;
				m_Hue = hue;
			}

			public override string ToString()
			{
				return string.Format( "Serial: \"0x{0:X}\", ItemId: \"0x{1:X}\", ItemIdName: \"{2}\", Amount: \"{3}\", X: \"{4}\", Y: \"{5}\", ContSerial: \"0x{6:X}\", Hue: \"0x{7:X}\"",
					m_Serial, m_ItemId, ItemIdName, m_Amount, m_X, m_Y, m_ContSerial, m_Hue );
			}
		}

		private ContainedItem[] m_ContainedItems;

		[PacketProp( 0 )]
		public string ContainedItems
		{
			get
			{
				if ( m_ContainedItems.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					sb.AppendFormat( "{0}", m_ContainedItems[i] );

					if ( ++i < m_ContainedItems.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		public ContainerContent( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			ushort n = reader.ReadUInt16();
			m_ContainedItems = new ContainedItem[n];

			for ( int i = 0; i < n; i++ )
			{
				uint serial = reader.ReadUInt32();
				ushort itemId = reader.ReadUInt16();

				reader.ReadByte();

				ushort amount = reader.ReadUInt16();
				short x = reader.ReadInt16();
				short y = reader.ReadInt16();
				uint contSerial = reader.ReadUInt32();
				ushort hue = reader.ReadUInt16();

				m_ContainedItems[i] = new ContainedItem( serial, itemId, amount, x, y, contSerial, hue );
			}
		}
	}
}