using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xA3 )]
	public class MobileStamina : Packet
	{
		private uint m_Serial;
		private ushort m_Stamina;
		private ushort m_MaxStamina;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public ushort Stamina { get { return m_Stamina; } }

		[PacketProp( 2 )]
		public ushort MaxStamina { get { return m_MaxStamina; } }

		public MobileStamina( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
			m_Stamina = reader.ReadUInt16();
			m_MaxStamina = reader.ReadUInt16();
		}
	}
}