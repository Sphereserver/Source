using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x6D )]
	public class PlayMusic : Packet
	{
		private ushort m_MusicId;

		[PacketProp( 0 )]
		public ushort MusicId { get { return m_MusicId; } }

		public PlayMusic( PacketReader reader, bool send ) : base( reader, send )
		{
			m_MusicId = reader.ReadUInt16();
		}
	}
}