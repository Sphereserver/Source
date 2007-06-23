using System;
using SpyUO.Packets;

namespace SpyUO
{
	public class PacketFilter
	{
		private bool[] m_Table;
		private bool[][] m_PropsHave;
		private string[][] m_PropsTable;

		public bool[] Table { get { return m_Table; } }
		public bool[][] PropsHave { get { return m_PropsHave; } }
		public string[][] PropsTable { get { return m_PropsTable; } }

		public PacketFilter()
		{
			m_Table = new bool[0x100];
			m_PropsHave = new bool[0x100][];
			m_PropsTable = new string[0x100][];

			for ( int i = 0; i < 0x100; i++ )
			{
				m_Table[i] = Packet.Table[i] != null;

				PacketProp[] props = Packet.PropsTable[i];
				if ( props != null )
				{
					m_PropsTable[i] = new string[props.Length];
					m_PropsHave[i] = new bool[props.Length];
					for ( int j = 0; j < props.Length; j++ )
						m_PropsHave[i][j] = true;
				}
			}
		}

		public bool Filter( TimePacket timePacket )
		{
			Packet packet = timePacket.Packet;

			byte cmd = packet.Cmd;
			if ( !m_Table[cmd] )
				return false;

			PacketProp[] props = packet.GetPacketProperties();
			if ( props != null )
			{
				string[] filterProps = m_PropsTable[cmd];
				bool[] mustHave = m_PropsHave[cmd];
				for ( int i = 0; i < props.Length; i++ )
				{
					if ( mustHave[i] )
					{
						if ( filterProps[i] != null && !FilterProp( filterProps[i], props[i].GetStringValue() ) )
							return false;
					}
					else
					{
						if ( filterProps[i] != null && FilterProp( filterProps[i], props[i].GetStringValue() ) )
							return false;
					}
				}
			}

			return true;
		}

		private bool FilterProp( string filterProp, string prop )
		{
			string[] splt = filterProp.Split( '|' );
			foreach ( string s in splt )
			{
				if ( s == prop )
					return true;
			}
			return false;
		}
	}
}