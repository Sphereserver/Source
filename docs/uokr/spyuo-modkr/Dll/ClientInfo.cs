using System;
using System.Globalization;

namespace SpyUO
{
	public class ClientInfo
	{
		public static ClientInfo Parse( string s )
		{
			string name;
			int ts;
			AddressAndRegisters send;
			AddressAndRegisters recv;

			try
			{
				int tsEnd = s.IndexOf( ':' );
				ts = Int32.Parse( s.Substring( 0, tsEnd ), NumberStyles.HexNumber );

				int nameStart = s.IndexOf( '"', tsEnd ) + 1;
				int nameEnd = s.IndexOf( '"', nameStart + 1 );
				name = s.Substring( nameStart, nameEnd - nameStart );

				string[] splt = s.Substring( nameEnd + 2 ).Split( ' ' );

                
				uint[] sendRecv = new uint[9];
				for ( int i = 0, n = 0; n < 9; i++ )
				{
					if ( splt[i] != "" )
					{
						sendRecv[n] = UInt32.Parse( splt[i], NumberStyles.HexNumber );
						n++;
					}
				}

				Register sAddrReg = GetRegister( sendRecv[1] );
				Register sLengthReg = GetRegister( sendRecv[3] );
                Register sCheckReg = GetRegister( sendRecv[4] );

				Register rAddrReg = GetRegister( sendRecv[6] );
				Register rLengthReg = GetRegister( sendRecv[8] );

				send = new AddressAndRegisters( sendRecv[0], sAddrReg, sendRecv[2], sLengthReg, sCheckReg );
                recv = new AddressAndRegisters( sendRecv[5], rAddrReg, sendRecv[7], rLengthReg );
			}
			catch
			{
				throw new FormatException();
			}

			return new ClientInfo( name, ts, send, recv );
		}

		private static Register GetRegister( uint i )
		{
			switch ( i )
			{
				case 0x1: return Register.Eax;
				case 0x2: return Register.Ebx;
				case 0x3: return Register.Ecx;
				case 0x4: return Register.Edx;
				case 0x5: return Register.Esi;
				case 0x6: return Register.Edi;
				case 0x7: return Register.Ebp;
				case 0x8: return Register.Esp;

				default: throw new ArgumentException();
			}
		}

		private string m_Name;
		private int m_TimeStamp;
		private AddressAndRegisters m_Send;
		private AddressAndRegisters m_Recv;

		public string Name { get { return m_Name; } }
		public int TimeStamp { get { return m_TimeStamp; } }
		public AddressAndRegisters Send { get { return m_Send; } }
		public AddressAndRegisters Recv { get { return m_Recv; } }

		public ClientInfo( string name, int timeStamp, AddressAndRegisters send, AddressAndRegisters recv )
		{
			m_Name = name;
			m_TimeStamp = timeStamp;
			m_Send = send;
			m_Recv = recv;
		}
	}
}