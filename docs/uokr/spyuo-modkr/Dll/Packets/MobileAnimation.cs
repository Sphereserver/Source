using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x6E )]
	public class MobileAnimation : Packet
	{
		private uint m_Serial;
		private ushort m_Action;
		private ushort m_FrameCount;
		private ushort m_RepeatCount;
		private bool m_Forward;
		private bool m_Repeat;
		private byte m_Delay;

		[PacketProp( 0, "0x{0}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public ushort Action { get { return m_Action; } }

		[PacketProp( 2 )]
		public ushort FrameCount { get { return m_FrameCount; } }

		[PacketProp( 3 )]
		public ushort RepeatCount { get { return m_RepeatCount; } }

		[PacketProp( 4 )]
		public bool Forward { get { return m_Forward; } }

		[PacketProp( 5 )]
		public bool Repeat { get { return m_Repeat; } }

		[PacketProp( 6 )]
		public byte Delay { get { return m_Delay; } }

		public MobileAnimation( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Serial = reader.ReadUInt32();
			m_Action = reader.ReadUInt16();
			m_FrameCount = reader.ReadUInt16();
			m_RepeatCount = reader.ReadUInt16();
			m_Forward = !reader.ReadBoolean();
			m_Repeat = reader.ReadBoolean();
			m_Delay = reader.ReadByte();
		}
	}
}