using System;
using System.Collections;
using System.Text;

namespace SpyUO.Packets
{
	[PacketInfo( 0x78 )]
	public class MobileIncoming : Packet
	{
		public struct EquipInfo
		{
			public uint Serial;
			public ushort ItemId;
			public byte Layer;
			public ushort Hue;

			public string ItemIdName
			{
				get
				{
					try
					{
						return Ultima.TileData.ItemTable[ItemId].Name;
					}
					catch
					{
						return null;
					}
				} 
			}

			public EquipInfo( uint serial, ushort itemId, byte layer, ushort hue )
			{
				Serial = serial;
				ItemId = itemId;
				Layer = layer;
				Hue = hue;
			}

			public override string ToString()
			{
				return string.Format( "Serial: \"0x{0:X}\", ItemId: \"0x{1:X}\", ItemIdName: \"{2}\", Layer: \"{3}\", Hue: \"0x{4:X}\"", Serial, ItemId, ItemIdName, Layer, Hue );
			}
		}

		private ushort m_ModelId;
		private ushort m_Hue;
		private Point3D m_Position;
		private byte m_Notoriety;
		private uint m_Serial;
		private byte m_Direction;
		private byte m_Flag;
		private EquipInfo[] m_Equipment;

		[PacketProp( 0, "0x{0:X}" )]
		public ushort ModelId { get { return m_ModelId; } }

		[PacketProp( 1, "0x{0:X}" )]
		public uint Hue { get { return m_Hue; } }

		[PacketProp( 2 )]
		public Point3D Position { get { return m_Position; } }

		[PacketProp( 3 )]
		public byte Notoriety { get { return m_Notoriety; } }

		[PacketProp( 4, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 5, "0x{0:X}" )]
		public byte Direction { get { return m_Direction; } }

		[PacketProp( 6, "0x{0:X}" )]
		public byte Flag { get { return m_Flag; } }

		public EquipInfo[] Equipment { get { return m_Equipment; } }

		[PacketProp( 7 )]
		public string EquipmentString
		{
			get
			{
				if ( m_Equipment.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					EquipInfo ei = m_Equipment[i];
					sb.Append( ei.ToString() );

					if ( ++i < m_Equipment.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		public MobileIncoming( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();
			m_ModelId = reader.ReadUInt16();
			m_Position = new Point3D( reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadSByte() );
			m_Direction = reader.ReadByte();
			m_Hue = reader.ReadUInt16();
			m_Flag = reader.ReadByte();
			m_Notoriety = reader.ReadByte();

			ArrayList list = new ArrayList();
			uint serial;
			while ( (serial = reader.ReadUInt32()) != 0 )
			{
				ushort itemId = reader.ReadUInt16();
				byte layer = reader.ReadByte();

				ushort hue;
				if ( (itemId & 0x8000) != 0 )
				{
					itemId &= 0x7FFF;
					hue = reader.ReadUInt16();
				}
				else
					hue = 0;

				list.Add( new EquipInfo( serial, itemId, layer, hue ) );
			}

			m_Equipment = (EquipInfo[])list.ToArray( typeof( EquipInfo ) );
		}
	}
}