using System;
using System.Collections;
using System.Diagnostics;
using System.Windows.Forms;
using System.IO;
using SpyUO.Packets;

namespace SpyUO
{
	public delegate void PacketPumpPacketHandler( PacketPump packetPump );
	public delegate void PacketPumpTerminatedHandler();

	public class PacketPump : IDisposable
	{
		private struct RawTimePacket
		{
			public byte[] Data;
			public bool Send;
			public DateTime Time;

			public RawTimePacket( byte[] data, bool send, DateTime time )
			{
				Data = data;
				Send = send;
				Time = time;
			}
		}

		public static ClientsConfig ClientsConfig = new ClientsConfig();

		private Queue m_PacketsQueue;
		private PacketSpy m_PacketSpy;
		private Control m_Control;
		private PacketPumpPacketHandler m_OnPacket;
		private bool m_Pause;

		public event PacketPumpTerminatedHandler OnPacketPumpTerminated;

		public PacketSpy PacketSpy { get { return m_PacketSpy; } }

		private bool SafePause
		{
			get { lock ( this ) return m_Pause; }
			set { lock ( this ) m_Pause = value; }
		}

		public PacketPump( Control control, PacketPumpPacketHandler onPacket )
		{
			m_PacketsQueue = Queue.Synchronized( new Queue() );
			m_Control = control;
			m_OnPacket = onPacket;
		}

		public void Start( string path )
		{
			ClientInfo cInfo = ClientsConfig.GetClientInfo( path );
			if ( cInfo == null )
				throw new Exception( "Client not defined" );

			m_PacketSpy = new PacketSpy( cInfo.Send, cInfo.Recv, new PacketHandler( HandlePacket ) );
			m_PacketSpy.OnProcessTerminated += new ProcessTerminatedHandler( ProcessEnd );

			m_PacketSpy.Spy( path );
		}

		public void Start( Process process )
		{
			ClientInfo cInfo = ClientsConfig.GetClientInfo( process );
			if ( cInfo == null )
				throw new Exception( "Client not defined" );

			m_PacketSpy = new PacketSpy( cInfo.Send, cInfo.Recv, new PacketHandler( HandlePacket ) );
			m_PacketSpy.OnProcessTerminated += new ProcessTerminatedHandler( ProcessEnd );

			m_PacketSpy.Spy( process );
		}

		public void TogglePause()
		{
			SafePause = !SafePause;
		}

		public TimePacket Dequeue()
		{
			if ( m_PacketsQueue.Count == 0 )
				return null;
			else
			{
				RawTimePacket rPacket = (RawTimePacket)m_PacketsQueue.Dequeue();

				Packet packet = Packet.Create( rPacket.Data, rPacket.Send );
				DateTime time = rPacket.Time;
				return new TimePacket( packet, time );
			}
		}

		private void HandlePacket( byte[] data, bool send )
		{
			if ( !SafePause )
			{
				m_PacketsQueue.Enqueue( new RawTimePacket( data, send, DateTime.Now ) );

				try
				{
					m_Control.BeginInvoke( m_OnPacket, new object[] { this } );
				}
				catch { }
			}
		}

		private void ProcessEnd()
		{
			m_Control.BeginInvoke( new ProcessTerminatedHandler( SyncProcessEnd ) );
		}

		private void SyncProcessEnd()
		{
			if ( OnPacketPumpTerminated != null )
				OnPacketPumpTerminated();
		}

		public void Dispose()
		{
			m_PacketSpy.Dispose();
		}
	}
}