using System;
using System.Collections;
using System.Text;

namespace SpyUO.Packets
{
	[PacketInfo( 0xD6 )]
	public class ObjectProperties : Packet
	{
		public struct Property
		{
			public uint Number;
			public string Arguments;

			public Property( uint number, string arguments )
			{
				Number = number;
				Arguments = arguments;
			}

			public override string ToString()
			{
				return string.Format( "Number: \"{0}\", NumberText: \"{1}\"{2}",
					Number, LocalizedList.List.Table[(int)Number], Arguments.Length != 0 ? " Arguments: \"" + Arguments + "\"" : "" );
			}
		}

		private uint m_Serial;
		private uint m_HashCode;
		private Property[] m_Properties;
		private ushort m_Type;

		[PacketProp( 0, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 1, "0x{0:X}" )]
		public uint HashCode { get { return m_HashCode; } }

		public Property[] Properties { get { return m_Properties; } }

		[PacketProp( 2 )]
		public string PropertiesString
		{
			get
			{
				if ( m_Properties.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					sb.AppendFormat( "{0}. \"{1}\"", i, m_Properties[i] );

					if ( ++i < m_Properties.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		[PacketProp( 3 )]
		public int Type{ get{ return m_Type; } }

		public ObjectProperties( PacketReader reader, bool send ) : base( reader, send )
		{
			reader.ReadUInt16();

			m_Type = reader.ReadUInt16();

			if ( m_Type == 1 )
			{
				m_Serial = reader.ReadUInt32();

				reader.ReadUInt16();

				m_HashCode = reader.ReadUInt32();

				ArrayList list = new ArrayList();
				while ( true )
				{
					uint number = reader.ReadUInt32();
					if ( number == 0 )
						break;

					ushort length = reader.ReadUInt16();
					string arguments = reader.ReadUnicodeStringLE( length );

					list.Add( new Property( number, arguments ) );
				}

				m_Properties = (Property[])list.ToArray( typeof( Property ) );
			}
			else
			{
				m_Properties = new Property[0];
			}
		}
	}
}