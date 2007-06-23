using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x24 )]
	public class ContainerDisplay : Packet
	{
		private uint m_ItemId;
		private ushort m_Gump;

		[PacketProp( 0, "0x{0:X}" )]
		public uint ItemId { get { return m_ItemId; } }

		[PacketProp( 1, "0x{0:X}" )]
		public ushort Gump { get { return m_Gump; } }

		public ContainerDisplay( PacketReader reader, bool send ) : base( reader, send )
		{
			m_ItemId = reader.ReadUInt32();
			m_Gump = reader.ReadUInt16();
		}
	}
}