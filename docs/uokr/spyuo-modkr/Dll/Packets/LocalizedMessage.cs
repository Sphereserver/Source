using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xC1 )]
	public class LocalizedMessage : Packet
	{
		private uint m_Number;
		private string m_Append;
		private byte m_SpeechType;
		private ushort m_Hue;
		private ushort m_Font;
		private string m_SourceName;
		private uint m_Serial;
		private ushort m_ModelId;

		[PacketProp( 0 )]
		public uint Number { get { return m_Number; } }

		[PacketProp( 1 )]
		public string Append { get { return m_Append; } }

		[PacketProp( 2, "0x{0:X}" )]
		public byte SpeechType { get { return m_SpeechType; } }

		[PacketProp( 3, "0x{0:X}" )]
		public ushort Hue { get { return m_Hue; } }

		[PacketProp( 4 )]
		public ushort Font { get { return m_Font; } }

		[PacketProp( 5 )]
		public string SourceName { get { return m_SourceName; } }

		[PacketProp( 6, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 7, "0x{0:X}" )]
		public ushort ModelId { get { return m_ModelId; } }

		[PacketProp( 8 )]
		public string NumberText { get { return LocalizedList.List.Table[(int)m_Number] as string; } }

		public LocalizedMessage( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();
			m_ModelId = reader.ReadUInt16();
			m_SpeechType = reader.ReadByte();
			m_Hue = reader.ReadUInt16();
			m_Font = reader.ReadUInt16();
			m_Number = reader.ReadUInt32();
			m_SourceName = reader.ReadASCIIString( 30 );
			m_Append = reader.ReadUnicodeStringLE();
		}
	}
}