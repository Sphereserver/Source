using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0x1C )]
	public class ASCIIMessage : Packet
	{
		private string m_Text;
		private byte m_SpeechType;
		private ushort m_Hue;
		private ushort m_Font;
		private string m_SourceName;
		private uint m_Serial;
		private ushort m_ModelId;

		[PacketProp( 0 )]
		public string Text { get { return m_Text; } }

		[PacketProp( 1, "0x{0:X}" )]
		public byte SpeechType { get { return m_SpeechType; } }

		[PacketProp( 2, "0x{0:X}" )]
		public ushort Hue { get { return m_Hue; } }

		[PacketProp( 3 )]
		public ushort Font { get { return m_Font; } }

		[PacketProp( 4 )]
		public string SourceName { get { return m_SourceName; } }

		[PacketProp( 5, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 6, "0x{0:X}" )]
		public ushort ModelId { get { return m_ModelId; } }

		public ASCIIMessage( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();
			m_ModelId = reader.ReadUInt16();
			m_SpeechType = reader.ReadByte();
			m_Hue = reader.ReadUInt16();
			m_Font = reader.ReadUInt16();
			m_SourceName = reader.ReadASCIIString( 30 );
			m_Text = reader.ReadASCIIString();
		}
	}
}