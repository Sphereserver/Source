using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;

using Unpacker.Mythic;

namespace Unpacker.Spying
{
	public class Spy : IDisposable
	{
		private Process m_Process;
		private IntPtr m_hProcess;

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

		public Spy()
		{
			m_ContextBuffer = new NativeMethods.CONTEXT();
			m_ContextBuffer.ContextFlags = NativeMethods.ContextFlags.CONTEXT_CONTROL | NativeMethods.ContextFlags.CONTEXT_INTEGER;
			m_DEventBuffer = new NativeMethods.DEBUG_EVENT_EXCEPTION();

			m_ToStop = false;
			m_Stopped = new ManualResetEvent( true );
		}

		private static byte[] Signature = new byte[]
		{
			0xC9, 0xC3, 0x8B, 0x45, 0x10, 0x89, 0x30, 0xEB
		};

		private byte[] m_Buffer = new byte[ ScanRange ];
		private const uint StartAddress = 0x665000;
		private const uint EndAddress = 0x675000;
		private const uint ScanRange = EndAddress - StartAddress;
		private uint m_Address;

		private uint FindBreakpoint()
		{
			m_Buffer = ReadProcessMemory( StartAddress, ScanRange );

			for ( int i = 0; i < m_Buffer.Length - Signature.Length; i ++ )
			{
				int j;

				for ( j = 0; j < Signature.Length; j ++ )
				{
					if ( m_Buffer[ i + j ] != Signature[ j ] )
						break;
				}

				if ( j == 8 )
					return (uint) ( StartAddress + i );
			}

			return 0;
		}

		public void Init( string path )
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
			m_Address = FindBreakpoint();

			if ( m_Address == 0 )
				throw new Exception( "Cannot find hash function!" );

			InitBreakpoints();
		}

		public void Init( Process process )
		{
			uint id = (uint)process.Id;

			m_Process = process;
			m_hProcess = NativeMethods.OpenProcess( NativeMethods.DesiredAccessProcess.PROCESS_ALL_ACCESS, false, id );

			if ( m_hProcess == IntPtr.Zero )
				throw new Win32Exception();

			if ( !NativeMethods.DebugActiveProcess( id ) )
				throw new Win32Exception();

			m_Address = FindBreakpoint();

			if ( m_Address == 0 )
				throw new Exception( "Cannot find hash function!" );

			InitBreakpoints();
		}

		private void InitBreakpoints()
		{
            m_OrCode = AddBreakpoint( m_Address );
		}
		
        private byte[] m_OrCode;
		private static readonly byte[] BreakCode = { 0xCC };
		private static ASCIIEncoding Encoding = new ASCIIEncoding();

		private byte[] AddBreakpoint( uint address )
		{
			byte[] orOpCode = ReadProcessMemory( address, 1 );

			WriteProcessMemory( address, BreakCode );

			return orOpCode;
		}

		private void RemoveBreakpoints()
		{
            WriteProcessMemory( m_Address, m_OrCode );
		}

		public void MainLoop()
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
							
							if ( address == m_Address )
							{
								SpyAddress( m_DEventBuffer.dwThreadId, address );
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
			}
		}

		private void SpyAddress( uint threadId, uint address )
		{
			IntPtr hThread = NativeMethods.OpenThread( NativeMethods.DesiredAccessThread.THREAD_GET_CONTEXT | NativeMethods.DesiredAccessThread.THREAD_SET_CONTEXT, false, threadId );

			GetThreadContext( hThread, ref m_ContextBuffer );

			byte[] data = ReadProcessMemory( m_ContextBuffer.Ecx, 8 );
			long hash = ( (long) BitConverter.ToUInt32( data, 4 ) << 32 ) | BitConverter.ToUInt32( data, 0 );

			if ( HashDictionary.Contains( hash ) )
			{
				uint pointer = m_ContextBuffer.Edi - 1;
				uint length = m_ContextBuffer.Ebx & 0xFFFF;
				string name = Encoding.GetString( ReadProcessMemory( pointer, length ) );

				HashDictionary.Set( hash, name );
			}

			#region Breakpoint Recovery
			WriteProcessMemory( address, m_OrCode );

            m_ContextBuffer.Eip--;
			m_ContextBuffer.EFlags |= 0x100; // Single step

			SetThreadContext( hThread, ref m_ContextBuffer );
			ContinueDebugEvent( threadId );

			if ( !NativeMethods.WaitForDebugEvent( ref m_DEventBuffer, uint.MaxValue ) )
				throw new Win32Exception();
			
			WriteProcessMemory( address, BreakCode );
			GetThreadContext( hThread, ref m_ContextBuffer );
			m_ContextBuffer.EFlags &= ~0x100u; // End single step
			SetThreadContext( hThread, ref m_ContextBuffer );
			#endregion

			NativeMethods.CloseHandle( hThread );

			ContinueDebugEvent( threadId );
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