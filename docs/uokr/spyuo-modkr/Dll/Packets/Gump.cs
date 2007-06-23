using System;
using System.Collections;

namespace SpyUO.Packets
{
	[PacketInfo( 0xB0 )]
	public class Gump : BaseGump
	{
		public Gump( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			uint serial = reader.ReadUInt32();
			uint gumpId = reader.ReadUInt32();
			uint x = reader.ReadUInt32();
			uint y = reader.ReadUInt32();

			ushort layoutLength = reader.ReadUInt16();
			string layout = reader.ReadASCIIString( layoutLength - 1 );
			reader.ReadByte();

			ArrayList textList = new ArrayList();
			ushort n = reader.ReadUInt16();
			for ( int i = 0; i < n; i++ )
			{
				int length = reader.ReadUInt16() * 2;

				string s = reader.ReadUnicodeString( length );
				textList.Add( s );
			}
			string[] text = (string[])textList.ToArray( typeof( string ) );

			Init( serial, gumpId, x, y, layout, text );
		}
	}
}