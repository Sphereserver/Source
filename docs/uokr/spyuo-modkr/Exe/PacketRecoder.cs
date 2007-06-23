using System;
using System.Collections;
using System.IO;
using System.Threading;
using SpyUO.Packets;

namespace SpyUO
{
	public delegate void OnPacket( TimePacket packet );

	public class PacketRecorder : IDisposable
	{
		public class TypeArguments
		{
			private string m_Type;
			private string m_Arguments;

			public string Type { get { return m_Type; } set { m_Type = value; } }
			public string Arguments { get { return m_Arguments; } set { m_Arguments = value; } }
		}

		public static Hashtable ItemsTable = new Hashtable();

		public static void InitItemsTable( string file )
		{
			StreamReader reader = File.OpenText( file );

			string line;
			while ( (line = reader.ReadLine()) != null )
			{
				line = line.Trim();
				if ( line == "" || line.StartsWith( "#" ) )
					continue;

				string[] splt = line.Split( ' ', '\t' );

				string[] ids = splt[0].Split( '-' );
				int itemId = Int32.Parse( ids[0], System.Globalization.NumberStyles.HexNumber );
				int itemIdEnd = ids.Length > 1 ? Int32.Parse( ids[1], System.Globalization.NumberStyles.HexNumber ) : itemId;

				TypeArguments typeArgu = new TypeArguments();
				if ( splt.Length > 1 )
				{
					string[] types = splt[1].Split( '-' );
					typeArgu.Type = types[0];
					if ( types.Length > 1 ) typeArgu.Arguments = types[1];
				}

				for ( int id = itemId; id <= itemIdEnd; id++ )
				{
					ItemsTable[id] = typeArgu;
				}
			}

			reader.Close();
		}

		private ArrayList m_Packets;
		private Hashtable m_Items;
		private Hashtable m_Books;
		private Timer m_Timer;
		private ICounterDisplay m_CounterDisplay;

		public ArrayList Packets { get { return m_Packets; } }
		public Hashtable Items { get { return m_Items; } }
		public Hashtable Books { get { return m_Books; } }
		public Timer Timer { get { return m_Timer; } }
		public ICounterDisplay CounterDisplay { get { return m_CounterDisplay; } }

		public event OnPacket OnPacket;

		public PacketRecorder() : this( null )
		{
		}

		public PacketRecorder( ICounterDisplay cDisplay )
		{
			m_Packets = new ArrayList();
			m_Items = new Hashtable();
			m_Books = new Hashtable();

			m_CounterDisplay = cDisplay;
			if ( cDisplay != null )
				m_Timer = new Timer( new TimerCallback( TimerCallback ), null, TimeSpan.FromSeconds( 1.0 ), TimeSpan.FromSeconds( 1.0 ) );
		}

		public void Dispose()
		{
			if ( m_Timer != null )
			{
				ManualResetEvent disposed = new ManualResetEvent( false );
				m_Timer.Dispose( disposed );
				disposed.WaitOne();
			}
		}

		public void PacketPumpDequeue( PacketPump packetPump )
		{
			TimePacket packet = packetPump.Dequeue();
			if ( packet != null )
			{
				m_Packets.Add( packet );

				UpdateTables( packet );

				if ( OnPacket != null )
					OnPacket( packet );
			}
		}

		public void Clear()
		{
			m_Packets.Clear();
			m_Items.Clear();
			m_Books.Clear();
		}

		private class ItemsBlock
		{
			private ushort m_ItemId;
			private uint m_Hue;
			private string m_Name;
			private ArrayList m_List;

			public ushort ItemId { get { return m_ItemId; } }
			public uint Hue { get { return m_Hue; } }
			public string Name { get { return m_Name; } }
			public ArrayList List { get { return m_List; } }

			public ItemsBlock( ushort itemId, uint hue, string name )
			{
				m_ItemId = itemId;
				m_Hue = hue;
				m_Name = name;
				m_List = new ArrayList();
			}

			public bool IsCompatible( WorldItem packet )
			{
				return packet.ItemId == m_ItemId && packet.Hue == m_Hue;
			}

			public void Add( WorldItem packet )
			{
				m_List.Add( packet );
			}
		}

		private ItemsBlock FindBlock( ArrayList blocks, WorldItem packet )
		{
			foreach ( ItemsBlock block in blocks )
			{
				if ( block.IsCompatible( packet ) )
					return block;
			}
			return null;
		}

		public void ExtractItems( StreamWriter writer )
		{
			ArrayList blocks = new ArrayList();

			foreach ( WorldItem item in m_Items.Values )
			{
				ItemsBlock block = FindBlock( blocks, item );

				if ( block == null )
				{
					block = new ItemsBlock( item.ItemId, item.Hue, item.ItemIdName );
					block.Add( item );
					blocks.Add( block );
				}
				else
				{
					block.Add( item );
				}
			}

			foreach ( ItemsBlock block in blocks )
			{
				TypeArguments typeArgu = ItemsTable[(int)block.ItemId] as TypeArguments;

				if ( typeArgu != null && typeArgu.Type == null )
					continue;

				if ( typeArgu == null )
				{
					typeArgu = new TypeArguments();
					typeArgu.Type = "Static";
				}

				string arguments = block.Hue != 0 ? string.Format( "Hue=0x{0:X}", block.Hue ) : "";
				if ( arguments.Length > 0 && typeArgu.Arguments != null )
					arguments += "; ";
				arguments += typeArgu.Arguments;

				writer.WriteLine( "# {0}", block.Name );

				writer.WriteLine( "{0} 0x{1:X4}{2}", typeArgu.Type, block.ItemId, arguments.Length > 0 ? string.Format( " ({0})", arguments ) : "" );

				foreach ( WorldItem packet in block.List )
					writer.WriteLine( "{0} {1} {2}", packet.Position.X, packet.Position.Y, packet.Position.Z );

				writer.WriteLine();
			}
		}

		public void TimerCallback( object state )
		{
			if ( m_CounterDisplay != null )
			{
				DateTime lastTenSeconds = DateTime.Now - TimeSpan.FromSeconds( 10.0 );

				int sentPackets = 0, sentPacketsSize = 0;
				int recvPackets = 0, recvPacketsSize = 0;

				for ( int i = m_Packets.Count - 1; i > 0; i-- )
				{
					TimePacket tPacket = (TimePacket)m_Packets[i];
					if ( tPacket.Time < lastTenSeconds )
						break;

					Packet packet = tPacket.Packet;
					if ( packet.Send )
					{
						sentPackets++;
						sentPacketsSize += packet.Data.Length;
					}
					else
					{
						recvPackets++;
						recvPacketsSize += packet.Data.Length;
					}
				}

				m_CounterDisplay.DisplayCounter( sentPackets/10, sentPacketsSize/10, recvPackets/10, recvPacketsSize/10 );
			}
		}

		public class TimePacketBuilder
		{
			private bool m_Send;
			private ArrayList m_Data;
			private DateTime m_Time;

			public bool Send { get { return m_Send; } set { m_Send = value; } }

			public byte[] Data
			{
				get
				{
					return (byte[])m_Data.ToArray( typeof( byte ) );
				}
			}

			public int DataLength { get { return m_Data.Count; } }

			public DateTime Time { get { return m_Time; } set { m_Time = value; } }

			public TimePacketBuilder()
			{
				m_Send = false;
				m_Data = new ArrayList();
				m_Time = DateTime.MinValue;
			}

			public void AddData( byte[] data )
			{
				m_Data.AddRange( data );
			}

			public void AddData( byte data )
			{
				m_Data.Add( data );
			}

			public TimePacket ToTimePacket()
			{
				Packet packet = Packet.Create( Data, m_Send );
				return new TimePacket( packet, m_Time );
			}
		}

		public void Load( StreamReader reader )
		{
			Clear();

			string line;
			TimePacketBuilder packetBuilder = null;
			while ( (line = reader.ReadLine()) != null )
			{
				try
				{
					line = line.Trim().ToLower();

					if ( line == "" )
						continue;

					if ( line.StartsWith( "packet -" ) )
					{
						string[] bytes = line.Substring( 8 ).Trim().Split( ' ' );
						foreach ( string s in bytes )
						{
							byte b = Byte.Parse( s, System.Globalization.NumberStyles.HexNumber );
							packetBuilder.AddData( b );
						}

						continue;
					}

					if ( line.StartsWith( "time -" ) )
					{
						string time = line.Substring( 6 ).Trim();
						packetBuilder.Time = DateTime.Parse( time );

						continue;
					}

					bool def;
					try
					{
						Int32.Parse( line.Substring( 0, 4 ), System.Globalization.NumberStyles.HexNumber );
						def = true;
					}
					catch
					{
						def = false;
					}

					if ( def && line[4] == ':' )
					{
						if ( line.StartsWith( "0000" ) && packetBuilder.DataLength != 0 )
						{
							Add( packetBuilder );
							packetBuilder = new TimePacketBuilder();
						}

						string[] bytes = line.Substring( 5, 3*0x10 ).Trim().Split( ' ' );
						foreach ( string s in bytes )
						{
							byte b = Byte.Parse( s, System.Globalization.NumberStyles.HexNumber );
							packetBuilder.AddData( b );
						}

						continue;
					}

					if ( line.IndexOf( "send" ) >= 0 || line.IndexOf( "client -> server" ) >= 0 || line.IndexOf( "client->server" ) >= 0 )
					{
						Add( packetBuilder );
						packetBuilder = new TimePacketBuilder();
						packetBuilder.Send = true;

						string[] tmp = line.Replace( "-", "" ).Split();
						foreach ( string s in tmp )
						{
							try
							{
								packetBuilder.Time = DateTime.Parse( s );
								break;
							}
							catch { }
						}

						continue;
					}

					if ( line.IndexOf( "recv" ) >= 0 || line.IndexOf( "receive" ) >= 0 || line.IndexOf( "server -> client" ) >= 0 || line.IndexOf( "server->client" ) >= 0 )
					{
						Add( packetBuilder );
						packetBuilder = new TimePacketBuilder();
						packetBuilder.Send = false;

						string[] tmp = line.Replace( "-", "" ).Split();
						foreach ( string s in tmp )
						{
							try
							{
								packetBuilder.Time = DateTime.Parse( s );
								break;
							}
							catch { }
						}

						continue;
					}

					if ( line.IndexOf( "client" ) >= 0 )
					{
						Add( packetBuilder );
						packetBuilder = new TimePacketBuilder();
						packetBuilder.Send = true;

						string[] tmp = line.Replace( "-", "" ).Split();
						foreach ( string s in tmp )
						{
							try
							{
								packetBuilder.Time = DateTime.Parse( s );
								break;
							}
							catch { }
						}

						continue;
					}

					if ( line.IndexOf( "server" ) >= 0 )
					{
						Add( packetBuilder );
						packetBuilder = new TimePacketBuilder();
						packetBuilder.Send = false;

						string[] tmp = line.Replace( "-", "" ).Split();
						foreach ( string s in tmp )
						{
							try
							{
								packetBuilder.Time = DateTime.Parse( s );
								break;
							}
							catch { }
						}

						continue;
					}
				}
				catch { }
			}
			Add( packetBuilder );
		}

		public void LoadBin( BinaryReader reader )
		{
			int version = reader.ReadInt32();

			Clear();

			int count = reader.ReadInt32();

			for ( int i = 0; i < count; i++ )
			{
				bool send = reader.ReadBoolean();
				DateTime time = new DateTime( reader.ReadInt64() );

				int length = reader.ReadInt32();
				byte[] data = reader.ReadBytes( length );

				Packet packet = Packet.Create( data, send );
				TimePacket tPacket = new TimePacket( packet, time );

				m_Packets.Add( tPacket );
				UpdateTables( tPacket );
			}
		}

		private void UpdateTables( TimePacket packet )
		{
			if ( packet.Packet is WorldItem )
			{
				WorldItem item = (WorldItem)packet.Packet;
				m_Items[item.Serial] = packet.Packet;

				if ( m_Books[item.Serial] != null )
				{
					((BookInfo)m_Books[item.Serial]).Item = item;
				}
			}
			else if ( packet.Packet is BookHeader )
			{
				BookHeader header = (BookHeader)packet.Packet;

				if ( m_Books[header.Serial] != null )
				{
					BookInfo book = (BookInfo)m_Books[header.Serial];

					book.Title = header.Title;
					book.Author = header.Author;
					book.Writable = header.Writable;

					if ( book.Lines.Length != header.PagesCount )
						book.Lines = new string[header.PagesCount][];
				}
				else
				{
					BookInfo book = new BookInfo( header.Serial, header.Title, header.Author, header.Writable, header.PagesCount );

					if ( m_Items[book.Serial] != null )
						book.Item = (WorldItem)m_Items[book.Serial];

					m_Books[book.Serial] = book;
				}
			}
			else if ( packet.Packet is BookPageDetails )
			{
				BookPageDetails details = (BookPageDetails)packet.Packet;

				if ( m_Books[details.Serial] != null )
				{
					BookInfo book = (BookInfo)m_Books[details.Serial];

					for ( int i = 0; i < details.Pages.Length; i++ )
					{
						if ( !details.Pages[i].Request && details.Pages[i].Index - 1 < book.Lines.Length )
						{
							book.Lines[details.Pages[i].Index - 1] = details.Pages[i].Lines;
						}
					}
				}
			}
		}

		public void Add( TimePacketBuilder packetBuilder )
		{
			try
			{
				if ( packetBuilder != null && packetBuilder.DataLength > 0 )
				{
					TimePacket packet = packetBuilder.ToTimePacket();

					m_Packets.Add( packet );

					UpdateTables( packet );
				}
			}
			catch { }
		}

		public void WriteBooks()
		{
			using ( StreamWriter writer = File.CreateText( "Prova.cs" ) )
			{
				foreach ( BookInfo book in m_Books.Values )
				{
					book.WriteRunUOClass( writer );
				}
			}
		}
	}
}