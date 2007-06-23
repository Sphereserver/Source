using System;
using System.IO;
using System.Text;

namespace SpyUO.Packets
{
	public class PacketReader
	{
		private byte[] m_Data;
		private int m_Index;

		public byte[] Data { get { return m_Data; } }

		public PacketReader( byte[] data )
		{
			m_Data = data;
			m_Index = 0;
		}

		public int Seek( int offset, SeekOrigin origin )
		{
			switch ( origin )
			{
				case SeekOrigin.Begin:
					m_Index = offset;
					break;

				case SeekOrigin.Current:
					m_Index += offset;
					break;

				case SeekOrigin.End:
					m_Index = m_Data.Length - offset;
					break;
			}

			return m_Index;
		}

		public int ReadInt32()
		{
			if ( m_Index + 4 > m_Data.Length )
				return 0;
			else
				return m_Data[m_Index++] << 24 | m_Data[m_Index++] << 16 | m_Data[m_Index++] << 8 | m_Data[m_Index++];
		}

		public short ReadInt16()
		{
			if ( m_Index + 2 > m_Data.Length )
				return 0;
			else
				return (short)(m_Data[m_Index++] << 8 | m_Data[m_Index++]);
		}

		public byte ReadByte()
		{
			if ( m_Index + 1 > m_Data.Length )
				return 0;
			else
				return m_Data[m_Index++];
		}

		public byte[] ReadBytes( int count )
		{
			byte[] bytes = new byte[count];

			for ( int i = 0; i < bytes.Length; i++ )
			{
				if ( m_Index + 1 <= m_Data.Length )
					bytes[i] = m_Data[m_Index++];
				else
					break;
			}

			return bytes;
		}

		public uint ReadUInt32()
		{
			if ( m_Index + 4 > m_Data.Length )
				return 0;
			else
				return (uint)(m_Data[m_Index++] << 24 | m_Data[m_Index++] << 16 | m_Data[m_Index++] << 8 | m_Data[m_Index++]);
		}

		public ushort ReadUInt16()
		{
			if ( m_Index + 2 > m_Data.Length )
				return 0;
			else
				return (ushort)(m_Data[m_Index++] << 8 | m_Data[m_Index++]);
		}

		public sbyte ReadSByte()
		{
			if ( m_Index + 1 > m_Data.Length )
				return 0;
			else
				return (sbyte)m_Data[m_Index++];
		}

		public bool ReadBoolean()
		{
			if ( m_Index + 1 > m_Data.Length )
				return false;
			else
				return m_Data[m_Index++] != 0;
		}

		public string ReadASCIIString( int fixedLength )
		{
			int end = m_Index + fixedLength;
			if ( end > m_Data.Length )
				end = m_Data.Length;

			StringBuilder s = new StringBuilder( end - fixedLength );
			while ( m_Index + 1 <= end )
			{
				byte next = m_Data[m_Index++];
				if ( next == 0 )
					break;

				s.Append( Encoding.ASCII.GetString( new byte[] { next } ) );
			}

			m_Index = end;

			return s.ToString();
		}

		public string ReadASCIIString()
		{
			StringBuilder s = new StringBuilder();
			while ( m_Index + 1 <= m_Data.Length )
			{
				byte next = m_Data[m_Index++];
				if ( next == 0 )
					break;

				s.Append( Encoding.ASCII.GetString( new byte[] { next } ) );
			}
			return s.ToString();
		}

		public string ReadUnicodeString( int fixedLength )
		{
			int end = m_Index + fixedLength;
			if ( end > m_Data.Length )
				end = m_Data.Length;

			StringBuilder s = new StringBuilder( end - fixedLength );
			while ( m_Index + 2 <= end )
			{
				ushort next = (ushort)(m_Data[m_Index++] << 8 | m_Data[m_Index++]);
				if ( next == 0 )
					break;

				s.Append( Encoding.Unicode.GetString( BitConverter.GetBytes( next ) ) );
			}

			m_Index = end;

			return s.ToString();
		}

		public string ReadUnicodeString()
		{
			StringBuilder s = new StringBuilder();
			while ( m_Index + 2 <= m_Data.Length )
			{
				ushort next = (ushort)(m_Data[m_Index++] << 8 | m_Data[m_Index++]);
				if ( next == 0 )
					break;

				s.Append( Encoding.Unicode.GetString( BitConverter.GetBytes( next ) ) );
			}
			return s.ToString();
		}

		public string ReadUnicodeStringLE( int fixedLength )
		{
			int end = m_Index + fixedLength;
			if ( end > m_Data.Length )
				end = m_Data.Length;

			StringBuilder s = new StringBuilder( end - fixedLength );
			while ( m_Index + 2 <= end )
			{
				ushort next = (ushort)(m_Data[m_Index++] | m_Data[m_Index++] << 8);
				if ( next == 0 )
					break;

				s.Append( Encoding.Unicode.GetString( BitConverter.GetBytes( next ) ) );
			}

			m_Index = end;

			return s.ToString();
		}

		public string ReadUnicodeStringLE()
		{
			StringBuilder s = new StringBuilder();
			while ( m_Index + 2 <= m_Data.Length )
			{
				ushort next = (ushort)(m_Data[m_Index++] | m_Data[m_Index++] << 8);
				if ( next == 0 )
					break;

				s.Append( Encoding.Unicode.GetString( BitConverter.GetBytes( next ) ) );
			}
			return s.ToString();
		}
	}
}