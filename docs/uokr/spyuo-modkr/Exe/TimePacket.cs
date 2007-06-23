using System;
using SpyUO.Packets;

namespace SpyUO
{
	public class TimePacket
	{
		private Packet m_Packet;
		private DateTime m_Time;

		public Packet Packet { get { return m_Packet; } }
		public DateTime Time { get { return m_Time; } }

		public TimePacket( Packet packet, DateTime time )
		{
			m_Packet = packet;
			m_Time = time;
		}
	}
}