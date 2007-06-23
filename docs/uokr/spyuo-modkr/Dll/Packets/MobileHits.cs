using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xA1 )]
	public class MobileHits : Packet
	{
		private uint m_Serial;
		private ushort m_Hits;
		private ushort m_MaxHits;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public ushort Hits { get { return m_Hits; } }

		[PacketProp( 2 )]
		public ushort MaxHits { get { return m_MaxHits; } }

		public MobileHits( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
			m_MaxHits = reader.ReadUInt16();
			m_Hits = reader.ReadUInt16();
		}
	}
}