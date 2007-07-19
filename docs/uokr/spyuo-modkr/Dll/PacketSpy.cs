using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace SpyUO
{
	public delegate void PacketHandler( byte[] data, bool send );
	public delegate void ProcessTerminatedHandler();

	public class PacketSpy : IDisposable
	{
		private abstract class PacketSpyStarter
		{
			public static void Start( PacketSpy packetSpy, string path )
			{
				PacketSpyStarter starter = new PathStarter( packetSpy, path );
				Start( starter );
			}

			public static void Start( PacketSpy packetSpy, Process process )
			{
				PacketSpyStarter starter = new ProcessStarter( packetSpy, process );
				Start( starter );
			}

			private static void Start( PacketSpyStarter starter )
			{
				Thread thread = new Thread( new ThreadStart( starter.Start ) );
				thread.Start();
			}

			private PacketSpy m_PacketSpy;

			public PacketSpy PacketSpy { get { return m_PacketSpy; } }

			public PacketSpyStarter( PacketSpy packetSpy )
			{
				m_PacketSpy = packetSpy;
			}

			public abstract void Start();

			private class PathStarter : PacketSpyStarter
			{
				string m_Path;

				public PathStarter( PacketSpy packetSpy, string path ) : base( packetSpy )
				{
					m_Path = path;
				}

				public override void Start()
				{
					PacketSpy.Init( m_Path );
					PacketSpy.MainLoop();
				}
			}

			private class ProcessStarter : PacketSpyStarter
			{
				private Process m_Process;

				public ProcessStarter( PacketSpy packetSpy, Process process ) : base( packetSpy )
				{
					m_Process = process;
				}

				public override void Start()
				{
					PacketSpy.Init( m_Process );
					PacketSpy.MainLoop();
				}
			}
		}

		private Process m_Process;
		private IntPtr m_hProcess;
		private AddressAndRegisters m_Send;
		private byte m_OrSCode;
        private byte m_OrSCode2;
        private AddressAndRegisters m_Recv;
		private byte m_OrRCode;
        private byte m_OrRCode2;
        private PacketHandler m_PacketHandler;

		private NativeMethods.CONTEXT m_ContextBuffer;
		private NativeMethods.DEBUG_EVENT_EXCEPTION m_DEventBuffer;

		private bool m_ToStop;
		private ManualResetEvent m_Stopped;

		private bool SafeToStop
		{
			get	{ lock ( this ) return m_ToStop; }
			set	{ lock ( this )	m_ToStop = value; }
		}

		private bool ProcessTerminated { get { return m_Process.HasExited; } }

		public event ProcessTerminatedHandler OnProcessTerminated;

		public PacketSpy( AddressAndRegisters send, AddressAndRegisters recv, PacketHandler packetHandler )
		{
			m_Send = send;
			m_Recv = recv;
			m_PacketHandler = packetHandler;

			m_ContextBuffer = new NativeMethods.CONTEXT();
			m_ContextBuffer.ContextFlags = NativeMethods.ContextFlags.CONTEXT_CONTROL | NativeMethods.ContextFlags.CONTEXT_INTEGER;
			m_DEventBuffer = new NativeMethods.DEBUG_EVENT_EXCEPTION();

			m_ToStop = false;
			m_Stopped = new ManualResetEvent( true );
		}

		public void Spy( string path )
		{
			PacketSpyStarter.Start( this, path );
		}

		public void Spy( Process process )
		{
			PacketSpyStarter.Start( this, process );
		}

		private void Init( string path )
		{
			string pathDir = Path.GetDirectoryName( path );

			NativeMethods.STARTUPINFO startupInfo = new NativeMethods.STARTUPINFO();
			NativeMethods.PROCESS_INFORMATION processInfo;

			if ( !NativeMethods.CreateProcess( path, null, IntPtr.Zero, IntPtr.Zero, false,
				NativeMethods.CreationFlag.DEBUG_PROCESS, IntPtr.Zero, pathDir, ref startupInfo, out processInfo ) )
				throw new Win32Exception();

			NativeMethods.CloseHandle( processInfo.hThread );

			m_Process = Process.GetProcessById( (int)processInfo.dwProcessId );
			m_hProcess = processInfo.hProcess;

			InitBreakpoints();
		}

		private void Init( Process process )
		{
			uint id = (uint)process.Id;

			m_Process = process;

			m_hProcess = NativeMethods.OpenProcess( NativeMethods.DesiredAccessProcess.PROCESS_ALL_ACCESS, false, id );
			if ( m_hProcess == IntPtr.Zero )
				throw new Win32Exception();

			if ( !NativeMethods.DebugActiveProcess( id ) )
				throw new Win32Exception();

			InitBreakpoints();
		}

		private void InitBreakpoints()
		{
			m_OrSCode = AddBreakpoint( m_Send.Address );
            m_OrSCode2 = AddBreakpoint( m_Send.LengthAddress );
			m_OrRCode = AddBreakpoint( m_Recv.Address );
            m_OrRCode2 = AddBreakpoint( m_Recv.LengthAddress );
		}

		private static readonly byte[] BreakCode = { 0xCC };

		private byte AddBreakpoint( uint address )
		{
			byte[] orOpCode = ReadProcessMemory( address, 1 );

			WriteProcessMemory( address, BreakCode );

			return orOpCode[0];
		}

		private void RemoveBreakpoints()
		{
			WriteProcessMemory(m_Send.Address, new byte[] { m_OrSCode } );
            WriteProcessMemory(m_Send.LengthAddress, new byte[] { m_OrSCode2 });
            WriteProcessMemory(m_Recv.Address, new byte[] { m_OrRCode });
            WriteProcessMemory(m_Recv.LengthAddress, new byte[] { m_OrRCode2 });
        }

		private void MainLoop()
		{
			m_Stopped.Reset();

			try
			{
				while ( !SafeToStop && !ProcessTerminated )
				{
					if ( NativeMethods.WaitForDebugEvent( ref m_DEventBuffer, 1000 ) )
					{
						if ( m_DEventBuffer.dwDebugEventCode == NativeMethods.DebugEventCode.EXCEPTION_DEBUG_EVENT )
						{
							uint address = (uint)m_DEventBuffer.u.Exception.ExceptionRecord.ExceptionAddress.ToInt32();
							if ( address == m_Send.Address )
							{
								SpySendPacket( m_DEventBuffer.dwThreadId, false );
								continue;
							}
							else if ( address == m_Send.LengthAddress )
							{
								SpySendPacket( m_DEventBuffer.dwThreadId, true );
								continue;
							}
                            else if (address == m_Recv.Address)
                            {
                                SpyRecvPacket(m_DEventBuffer.dwThreadId, false);
                                continue;
                            }
                            else if ( address == m_Recv.LengthAddress )
							{
								SpyRecvPacket( m_DEventBuffer.dwThreadId, true );
								continue;
							}
						}

						ContinueDebugEvent( m_DEventBuffer.dwThreadId );
					}
				}
			}
			finally
			{
				EndSpy();

				if ( ProcessTerminated && OnProcessTerminated != null )
					OnProcessTerminated();
			}
		}

		private void SpySendPacket( uint threadId, bool length )
		{
			SpyPacket( threadId, true, length );
		}

        private void SpyRecvPacket(uint threadId, bool length)
		{
            SpyPacket(threadId, false, length);
		}

        private uint lastAddress_s,lastAddress_r;

        private void SpyPacket(uint threadId, bool send, bool length)
		{
			IntPtr hThread = NativeMethods.OpenThread( NativeMethods.DesiredAccessThread.THREAD_GET_CONTEXT | NativeMethods.DesiredAccessThread.THREAD_SET_CONTEXT, false, threadId );

			GetThreadContext( hThread, ref m_ContextBuffer );

			AddressAndRegisters ar = send ? m_Send : m_Recv;

            byte[] data=null;

            bool handle=true;

            if (send && GetContextRegister(m_ContextBuffer, ar.CheckRegister) != 2)
                handle = false;

            uint lastAddress;

            if (handle)
            {
                if (!length)
                {
                    lastAddress = GetContextRegister(m_ContextBuffer, ar.AddressRegister);
                    if (send)
                        lastAddress_s = lastAddress;
                    else
                        lastAddress_r = lastAddress;
                }
                else
                {
                    uint dataLength = GetContextRegister(m_ContextBuffer, ar.LengthRegister) & 0xFFFF;
                    if (send)
                        lastAddress = new BinaryReader(new MemoryStream(ReadProcessMemory(lastAddress_s + 4, 4))).ReadUInt32();
                    else
                        lastAddress = lastAddress_r;
                    data = ReadProcessMemory(lastAddress, dataLength);
                }
            }
			#region Breakpoint Recovery

			WriteProcessMemory( length ? ar.LengthAddress : ar.Address, new byte[] { send ? (length ? m_OrSCode2 : m_OrSCode) : (length ?  m_OrRCode2 : m_OrRCode )} );
			m_ContextBuffer.Eip--;
			m_ContextBuffer.EFlags |= 0x100; // Single step

			SetThreadContext( hThread, ref m_ContextBuffer );
			ContinueDebugEvent( threadId );

			if ( !NativeMethods.WaitForDebugEvent( ref m_DEventBuffer, uint.MaxValue ) )
				throw new Win32Exception();

            WriteProcessMemory(length ? ar.LengthAddress : ar.Address, BreakCode);

			GetThreadContext( hThread, ref m_ContextBuffer );
			m_ContextBuffer.EFlags &= ~0x100u; // End single step
			SetThreadContext( hThread, ref m_ContextBuffer );

			#endregion

			NativeMethods.CloseHandle( hThread );

			ContinueDebugEvent( threadId );

            if(length && handle)
			    m_PacketHandler( data, send );
		}

		private byte[] ReadProcessMemory( uint address, uint length )
		{
			byte[] buffer = new byte[length];
			IntPtr ptrAddr = new IntPtr( address );

			uint read;
			NativeMethods.ReadProcessMemory( m_hProcess, ptrAddr, buffer, length, out read );
			if ( read != length )
				throw new Win32Exception();

			return buffer;
		}

		private void WriteProcessMemory( uint address, byte[] data )
		{
			uint length = (uint)data.Length;
			IntPtr ptrAddr = new IntPtr( address );

			uint written;
			NativeMethods.WriteProcessMemory( m_hProcess, ptrAddr, data, length, out written );
			if ( written != length )
				throw new Win32Exception();

			NativeMethods.FlushInstructionCache( m_hProcess, ptrAddr, length );
		}

		private void ContinueDebugEvent( uint threadId )
		{
			if ( !NativeMethods.ContinueDebugEvent( (uint)m_Process.Id, threadId, NativeMethods.ContinueStatus.DBG_CONTINUE ) )
				throw new Win32Exception();
		}

		private void GetThreadContext( IntPtr hThread, ref NativeMethods.CONTEXT context )
		{
			if ( !NativeMethods.GetThreadContext( hThread, ref context ) )
				throw new Win32Exception();
		}

		private void SetThreadContext( IntPtr hThread, ref NativeMethods.CONTEXT context )
		{
			if ( !NativeMethods.SetThreadContext( hThread, ref context ) )
				throw new Win32Exception();
		}

		private uint GetContextRegister( NativeMethods.CONTEXT context, Register register )
		{
			switch ( register )
			{
				case Register.Eax: return context.Eax;
				case Register.Ebp: return context.Ebp;
				case Register.Ebx: return context.Ebx;
				case Register.Ecx: return context.Ecx;
				case Register.Edi: return context.Edi;
				case Register.Edx: return context.Edx;
				case Register.Esi: return context.Esi;
				case Register.Esp: return context.Esp;

				default: throw new ArgumentException();
			}
		}

		public void Dispose()
		{
			if ( !SafeToStop )
			{
				SafeToStop = true;

				m_Stopped.WaitOne();
				m_Stopped.Close();
			}
		}

		private void EndSpy()
		{
			try
			{
				RemoveBreakpoints();
				NativeMethods.DebugActiveProcessStop( (uint)m_Process.Id );
				NativeMethods.CloseHandle( m_hProcess );
			}
			catch { }

			m_Stopped.Set();
		}
	}
}