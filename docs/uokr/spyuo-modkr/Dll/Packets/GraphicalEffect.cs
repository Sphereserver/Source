using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x70 )]
	public class GraphicalEffect : Packet
	{
		private byte m_Type;
		private uint m_From;
		private uint m_To;
		private ushort m_ItemId;
		private Point3D m_FromPoint;
		private Point3D m_ToPoint;
		private byte m_Speed;
		private byte m_Duration;
		private bool m_FixedDirection;
		private bool m_Explode;

		[PacketProp( 0 )]
		public byte Type { get { return m_Type; } }

		[PacketProp( 1, "0x{0:X}" )]
		public uint From { get { return m_From; } }

		[PacketProp( 2, "0x{0:X}" )]
		public uint To { get { return m_To; } }

		[PacketProp( 3, "0x{0:X}" )]
		public ushort ItemId { get { return m_ItemId; } }

		[PacketProp( 4 )]
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

		[PacketProp( 5 )]
		public Point3D FromLocation { get { return m_FromPoint; } }

		[PacketProp( 6 )]
		public Point3D ToLocation { get { return m_ToPoint; } }

		[PacketProp( 7 )]
		public byte Speed { get { return m_Speed; } }

		[PacketProp( 8 )]
		public byte Duration { get { return m_Duration; } }

		[PacketProp( 9 )]
		public bool FixedDirection { get { return m_FixedDirection; } }

		[PacketProp( 10 )]
		public bool Explode { get { return m_Explode; } }

		public GraphicalEffect( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Type = reader.ReadByte();
			m_From = reader.ReadUInt32();
			m_To = reader.ReadUInt32();
			m_ItemId = reader.ReadUInt16();
			m_FromPoint = new Point3D( reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadSByte() );
			m_ToPoint = new Point3D( reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadSByte() );
			m_Speed = reader.ReadByte();
			m_Duration = reader.ReadByte();

			reader.ReadByte();
			reader.ReadByte();

			m_FixedDirection = reader.ReadBoolean();
			m_Explode = reader.ReadBoolean();
		}
	}
}