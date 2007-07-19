using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x25 )]
	public class ContainerContentUpdate : Packet
	{
		private uint m_Serial;
		private ushort m_ItemId;
		private ushort m_Amount;
		private short m_X;
		private short m_Y;
        private byte m_Grid;
		private uint m_ContSerial;
		private ushort m_Hue;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1, "0x{0:X}" )]
		public ushort ItemID { get { return m_ItemId; } }

		[PacketProp( 2 )]
		public ushort Amount { get { return m_Amount; } }

		[PacketProp( 3 )]
		public short X { get { return m_X; } }

		[PacketProp( 4 )]
		public short Y { get { return m_Y; } }

        [PacketProp( 5 )]
        public short GridPos { get { return m_Grid; } }

		[PacketProp( 6, "0x{0:X}" )]
		public uint ContSerial { get { return m_ContSerial; } }

		[PacketProp( 7, "0x{0:X}" )]
		public ushort Hue { get { return m_Hue; } }

		public ContainerContentUpdate( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
			m_ItemId = reader.ReadUInt16();

			reader.ReadByte();

			m_Amount = reader.ReadUInt16();
			m_X = reader.ReadInt16();
			m_Y = reader.ReadInt16();
            m_Grid = reader.ReadByte();
			m_ContSerial = reader.ReadUInt32();
			m_Hue = reader.ReadUInt16();
		}
	}
}