using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x6 )]
	public class DoubleClick : Packet
	{
		private uint m_Serial;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		public DoubleClick( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
		}
	}
}