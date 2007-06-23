using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x54 )]
	public class PlaySound : Packet
	{
		private ushort m_SoundId;
		private byte m_SoundMode;
		private Point3D m_Position;
		private short m_Unknown;

		[PacketProp( 0, "0x{0:X}" )]
		public ushort SoundId { get { return m_SoundId; } }

		[PacketProp( 1 )]
		public byte SoundMode { get { return m_SoundMode; } }

		[PacketProp( 2 )]
		public Point3D Position { get { return m_Position; } }

		[PacketProp( 3, "0x{0:X}" )]
		public short Unknown { get { return m_Unknown; } }

		public PlaySound( PacketReader reader, bool send ) : base( reader, send )
		{
			m_SoundMode = reader.ReadByte();
			m_SoundId = reader.ReadUInt16();
			m_Unknown = reader.ReadInt16();
			m_Position = new Point3D( reader.ReadInt16(), reader.ReadInt16(), reader.ReadInt16() );
		}
	}
}