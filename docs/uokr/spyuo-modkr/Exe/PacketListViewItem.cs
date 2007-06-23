using System;
using System.Windows.Forms;
using System.Text;
using System.Drawing;
using SpyUO.Packets;

namespace SpyUO
{
	public class PacketListViewItem : ListViewItem
	{
		private TimePacket m_TimePacket;
		private DateTime m_BaseTime;
		private DateTime m_LastTime;

		public TimePacket TimePacket { get { return m_TimePacket; } }
		public DateTime BaseTime { get { return m_BaseTime; } }
		public DateTime LastTime { get { return m_LastTime; } }

		public PacketListViewItem( TimePacket packet, DateTime baseTime, DateTime lastTime ) : base( GetPacketType( packet ) )
		{
			m_TimePacket = packet;

			SubItems.Add( GetMessage() );
			SubItems.Add( GetTime() );
			SubItems.Add( GetRelTime( baseTime ) );
			SubItems.Add( GetDifTime( lastTime ) );
			SubItems.Add( GetPacket() );
			SubItems.Add( GetASCII() );
			SubItems.Add( GetLength() );
		}

		public static string GetPacketType( TimePacket timePacket )
		{
			Packet packet = timePacket.Packet;

			return string.Format( "{0} ({1:X2}) - {2}", packet.GetType().Name, packet.Cmd, packet.Send ? "Send" : "Receive" );
		}

		public string GetPacketType()
		{
			return GetPacketType( m_TimePacket );
		}

		public string GetMessage()
		{
			PacketProp[] props = m_TimePacket.Packet.GetPacketProperties();
			if ( props == null )
				return "Unknown message";
			else if ( props.Length == 0 )
				return "Empty message";

			StringBuilder sb = new StringBuilder();
			int i = 0;
			while ( true )
			{
				PacketProp prop = props[i];
				sb.Append( prop.PropInfo.Name + ": \"" + prop.GetStringValue() + "\"" );

				if ( ++i < props.Length )
					sb.Append( " " );
				else
					break;
			}
			return sb.ToString();
		}

		public string GetTime()
		{
			return m_TimePacket.Time.ToString( @"H\:mm\:ss.ff" );
		}

		public string GetRelTime( DateTime baseTime )
		{
			m_BaseTime = baseTime;
			return GetRelTime();
		}

		public string GetRelTime()
		{
			TimeSpan relTime = m_TimePacket.Time - m_BaseTime;
			return relTime.TotalSeconds.ToString( @"0.00\s" );
		}

		public string GetDifTime( DateTime baseTime )
		{
			m_LastTime = baseTime;
			return GetDifTime();
		}

		public string GetDifTime()
		{
			TimeSpan difTime;
			if ( m_LastTime == DateTime.MinValue )
				difTime = TimeSpan.Zero;
			else
				difTime = m_TimePacket.Time - m_LastTime;

			return difTime.TotalSeconds.ToString( @"0.00\s" );
		}

		public string GetPacket()
		{
			byte[] data = m_TimePacket.Packet.Data;
			if ( data.Length == 0 )
				return "Empty packet";

			StringBuilder sb = new StringBuilder();
			int i = 0;
			while ( true )
			{
				sb.Append( data[i].ToString( "X2") );

				if ( ++i < data.Length )
					sb.Append( " " );
				else
					break;
			}
			return sb.ToString();
		}

		public string GetASCII()
		{
			byte[] data = m_TimePacket.Packet.Data;
			if ( data.Length == 0 )
				return "Empty packet";

			StringBuilder sb = new StringBuilder();
			foreach ( byte b in data )
			{
				if ( b >= 0x20 && b < 0x80 )
					sb.Append( (char)b );
				else
					sb.Append( '.' );
			}
			return sb.ToString();
		}

		public string GetLength()
		{
			return m_TimePacket.Packet.Data.Length.ToString();
		}

		public void UpdateRelTime( DateTime newBaseTime )
		{
			SubItems[3].Text = GetRelTime( newBaseTime );
		}

		public void UpdateDifTime( DateTime newBaseTime )
		{
			SubItems[4].Text = GetDifTime( newBaseTime );
		}
	}
}