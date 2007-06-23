using System;
using System.Text;

namespace SpyUO.Packets
{
	[PacketInfo( 0xD4 )]
	public class BookHeader : Packet
	{
		private string m_Title;
		private string m_Author;
		private uint m_Serial;
		private ushort m_PagesCount;
		private bool m_Writable;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1 )]
		public string Title{ get{ return m_Title; } }

		[PacketProp( 2 )]
		public string Author{ get{ return m_Author; } }

		[PacketProp( 3 )]
		public ushort PagesCount{ get{ return m_PagesCount; } }

		[PacketProp( 4 )]
		public bool Writable{ get{ return m_Writable; } }

		public BookHeader( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Serial = reader.ReadUInt32();

			reader.ReadByte();

			m_Writable = reader.ReadBoolean();
			m_PagesCount = reader.ReadUInt16();

			int titleLength = reader.ReadUInt16() - 1;
			byte[] titleBuffer = reader.ReadBytes( titleLength );
			m_Title = Encoding.UTF8.GetString( titleBuffer );
			reader.ReadByte();

			int authorLength = reader.ReadUInt16() - 1;
			byte[] authorBuffer = reader.ReadBytes( authorLength );
			m_Author = Encoding.UTF8.GetString( authorBuffer );
			reader.ReadByte();
		}
	}
}