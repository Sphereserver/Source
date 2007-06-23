using System;
using System.Collections;

namespace SpyUO.Packets
{
	[PacketInfo( 0xDD )]
	public class PackedGump : BaseGump
	{
		public PackedGump( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			uint serial = reader.ReadUInt32();
			uint gumpId = reader.ReadUInt32();
			uint x = reader.ReadUInt32();
			uint y = reader.ReadUInt32();

			byte[] dLayout = ReadPacked( reader );
			PacketReader layoutReader = new PacketReader( dLayout );
			string layout = layoutReader.ReadASCIIString( dLayout.Length );

			int stringCount = reader.ReadInt32();

			byte[] dText = ReadPacked( reader );
			PacketReader textReader = new PacketReader( dText );

			ArrayList textList = new ArrayList();
			for ( int i = 0; i < stringCount; i++ )
			{
				int length = textReader.ReadUInt16() * 2;

				string s = textReader.ReadUnicodeString( length );
				textList.Add( s );
			}
			string[] text = (string[])textList.ToArray( typeof( string ) );


			Init( serial, gumpId, x, y, layout, text );
		}

		byte[] ReadPacked( PacketReader reader )
		{
			int packLength = reader.ReadInt32() - 4;
			if ( packLength < 0 )
				return new byte[0];

			int length = reader.ReadInt32();

			byte[] pack = reader.ReadBytes( packLength );
			byte[] buffer = new byte[length];

			ZLib.uncompress( buffer, ref length, pack, packLength );

			return buffer;
		}
	}
}