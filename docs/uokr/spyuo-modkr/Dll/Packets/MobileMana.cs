using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xA2 )]
	public class MobileMana : Packet
	{
		private uint m_Serial;
		private ushort m_Mana;
		private ushort m_MaxMana;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public ushort Mana { get { return m_Mana; } }

		[PacketProp( 2 )]
		public ushort MaxMana { get { return m_MaxMana; } }

		public MobileMana( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
			m_MaxMana = reader.ReadUInt16();
			m_Mana = reader.ReadUInt16();
		}
	}
}