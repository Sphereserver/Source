using System;

namespace SpyUO.Packets
{
	[AttributeUsage( AttributeTargets.Class )]
	public class PacketInfoAttribute : Attribute
	{
		private byte m_Cmd;

		public byte Cmd { get { return m_Cmd; } }

		public PacketInfoAttribute( byte cmd )
		{
			m_Cmd = cmd;
		}
	}
}