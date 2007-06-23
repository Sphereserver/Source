using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xCC )]
	public class LocalizedMessageAffix : Packet
	{
		private uint m_Number;
		private string m_Affix;
		private byte m_AffixType;
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
		public string Affix { get { return m_Affix; } }

		[PacketProp( 2, "0x{0:X}" )]
		public byte AffixType { get { return m_AffixType; } }

		[PacketProp( 3 )]
		public string Append { get { return m_Append; } }

		[PacketProp( 4, "0x{0:X}" )]
		public byte SpeechType { get { return m_SpeechType; } }

		[PacketProp( 5, "0x{0:X}" )]
		public ushort Hue { get { return m_Hue; } }

		[PacketProp( 6 )]
		public ushort Font { get { return m_Font; } }

		[PacketProp( 7 )]
		public string SourceName { get { return m_SourceName; } }

		[PacketProp( 8, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 9, "0x{0:X}" )]
		public ushort ModelId { get { return m_ModelId; } }

		[PacketProp( 10 )]
		public string NumberText { get { return LocalizedList.List.Table[(int)m_Number] as string; } }

		public LocalizedMessageAffix( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();
			m_ModelId = reader.ReadUInt16();
			m_SpeechType = reader.ReadByte();
			m_Hue = reader.ReadUInt16();
			m_Font = reader.ReadUInt16();
			m_Number = reader.ReadUInt32();
			m_AffixType = reader.ReadByte();
			m_SourceName = reader.ReadASCIIString( 30 );
			m_Affix = reader.ReadASCIIString();
			m_Append = reader.ReadUnicodeString();
		}
	}
}