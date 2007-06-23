using System;
using System.Collections;
using System.ComponentModel;
using System.IO;
using System.Diagnostics;

namespace SpyUO
{
	public class ClientsConfig
	{
		private Hashtable m_Table;

		public Hashtable Table { get { return m_Table; } }

		public ClientsConfig()
		{
			m_Table = new Hashtable();
		}

		public ClientsConfig( string cfgPath )
		{
			m_Table = new Hashtable();

			AddToTable( cfgPath );
		}

		public void AddToTable( string cfgPath )
		{
			StreamReader reader = File.OpenText( cfgPath );

			string line;
			while ( (line = reader.ReadLine()) != null )
			{
				if ( line.StartsWith( "#" ) || line.Trim() == "" )
					continue;

				ClientInfo clientInfo = ClientInfo.Parse( line );
				m_Table[clientInfo.TimeStamp] = clientInfo;
			}

			reader.Close();
		}

		public ClientInfo GetClientInfo( string filePath )
		{
			int timeStamp = ExtractTimeStamp( filePath );

			return m_Table[timeStamp] as ClientInfo;
		}

		public ClientInfo GetClientInfo( Process process )
		{
			int timeStamp = ExtractTimeStamp( process );

			return m_Table[timeStamp] as ClientInfo;
		}

		public static int ExtractTimeStamp( string filePath )
		{
			Stream stream = File.OpenRead( filePath );
			int timeStamp = ExtractTimeStamp( stream );
			stream.Close();

			return timeStamp;
		}

		public static int ExtractTimeStamp( Stream stream )
		{
			using ( BinaryReader reader = new BinaryReader( stream ) )
			{
				stream.Seek( 0x3C, SeekOrigin.Begin );
				int peOffset = reader.ReadInt32();

				stream.Seek( peOffset + 8, SeekOrigin.Begin );
				return reader.ReadInt32();
			}
		}

		public static int ExtractTimeStamp( Process process )
		{
			IntPtr hProc = NativeMethods.OpenProcess( NativeMethods.DesiredAccessProcess.PROCESS_ALL_ACCESS, false, (uint)process.Id );

			try
			{
				byte[] buffer = new byte[4];
				uint read;
				NativeMethods.ReadProcessMemory( hProc, new IntPtr( 0x40003C ), buffer, 4, out read );
				if ( read != 4 )
					throw new Win32Exception();

				int peOffset = BitConverter.ToInt32( buffer, 0 );

				NativeMethods.ReadProcessMemory( hProc, new IntPtr( 0x400000 | (peOffset + 8) ), buffer, 4, out read );
				if ( read != 4 )
					throw new Win32Exception();

				return BitConverter.ToInt32( buffer, 0 );
			}
			finally
			{
				NativeMethods.CloseHandle( hProc );
			}
		}
	}
}