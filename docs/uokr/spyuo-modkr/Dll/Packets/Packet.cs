using System;
using System.Reflection;
using System.Collections;

namespace SpyUO.Packets
{
	public class Packet
	{
		private static Type[] m_Table;
		private static PacketProp[][] m_PropsTable;

		public static Type[] Table { get { return m_Table; } }
		public static PacketProp[][] PropsTable { get { return m_PropsTable; } }

		static Packet()
		{
			m_Table = new Type[0x100];
			m_PropsTable = new PacketProp[0x100][];

			Type[] types = Assembly.GetExecutingAssembly().GetTypes();
			foreach ( Type type in types )
			{
				PacketInfoAttribute[] attrs = (PacketInfoAttribute[])type.GetCustomAttributes( typeof( PacketInfoAttribute ), false );
				if ( attrs.Length > 0 )
				{
					byte cmd = attrs[0].Cmd;
					m_Table[cmd] = type;

					PropertyInfo[] properties = type.GetProperties();

					ArrayList list = new ArrayList();
					foreach ( PropertyInfo propInfo in properties )
					{
						PacketPropAttribute[] propsAttrs = (PacketPropAttribute[])propInfo.GetCustomAttributes( typeof( PacketPropAttribute ), true );

						if ( propsAttrs.Length > 0 )
						{
							PacketProp pp = new PacketProp( propInfo, propsAttrs[0], null );

							list.Add( pp );
						}
					}
					list.Sort();

					m_PropsTable[cmd] = (PacketProp[])list.ToArray( typeof( PacketProp ) );
				}
			}
		}

		public static Packet Create( byte[] data, bool send )
		{
			PacketReader reader = new PacketReader( data );
			byte cmd = reader.ReadByte();

			Type type = m_Table[cmd];
			if ( type != null )
				return (Packet)Activator.CreateInstance( type, new object[] { reader, send } );
			else
				return new Packet( reader, send );
		}

		private byte[] m_Data;
		private bool m_Send;

		public byte[] Data { get { return m_Data; } }
		public bool Send { get { return m_Send; } }
		public byte Cmd { get { return m_Data[0]; } }

		public Packet( PacketReader reader, bool send )
		{
			m_Data = reader.Data;
			m_Send = send;
		}

		public PacketProp[] GetPacketProperties()
		{
			PacketProp[] tableProps = m_PropsTable[Cmd];
			if ( tableProps != null )
			{
				PacketProp[] props = new PacketProp[tableProps.Length];

				for ( int i = 0; i < props.Length; i++ )
				{
					PacketProp pp = tableProps[i];
					props[i] = new PacketProp( pp, pp.PropInfo.GetValue( this, null ) );
				}

				return props;
			}
			else
				return null;
		}
	}
}