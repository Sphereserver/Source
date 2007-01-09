#include "packet.h"
#include "network.h"

/***************************************************************************
 *
 *
 *	class Packet				Base packet class for both sending/receiving
 *
 *
 ***************************************************************************/

Packet::Packet(long size)
{
	m_baselen = size;
	clear();
	resize( (size > 0 ) ? size : config.get("network.packet.size") );
#ifdef DEBUGPACKETS
	g_Log.Debug("Packet constructed %x (%d bytes).\n", m_buf, size);
#endif
}

Packet::Packet(BYTE *buf, long size)
{
	clear();
	m_baselen = -1;
	resize(size);
	m_maxlen = m_len = size;
	memcpy(m_buf, buf, m_len);
}

Packet::Packet(Packet &other)
{
	clear();
	copy(other);
#ifdef DEBUGPACKETS
	g_Log.Debug("Packet copy constructed %x from %x (%d bytes).\n", m_buf, other.m_buf, m_len);
#endif
}

Packet::~Packet()
{
	if ( m_buf )
	{
#ifdef DEBUGPACKETS
		g_Log.Debug("Packet destructed %x (%d bytes).\n", m_buf, m_len);
#endif
		free(m_buf);
		clear();
	}
}

void Packet::clear()
{
	m_buf = NULL;
	m_maxlen = m_len = m_index = 0;
}

void Packet::copy(Packet &other)
{
	resize(other.m_len);
	m_maxlen = m_len = other.m_len;
	memcpy(m_buf, other.m_buf, m_len);
	m_index = 0;
	m_baselen = other.m_baselen;
}

bool Packet::valid()
{
	return ( m_buf && m_len );
}

void Packet::seek(long pos)
{
	m_index = pos;
}

long Packet::length()
{
	return m_len;
}

long Packet::baseLength()
{
	return m_baselen;
}

BYTE *Packet::data()
{
	return m_buf;
}

void Packet::resize(long newsize)
{
#ifdef DEBUGPACKETS
	g_Log.Debug("Resizing packet from %d bytes to %d (index=%d)\n", m_len, newsize, m_index);
#endif
	if ( newsize == -1 )			// special case, we running out of space while writing
	{								// auto-expand silently
		int oldIndex = m_index;
		resize(m_maxlen + config.get("network.packet.grow"));
		m_index = oldIndex;
		return;
	}

	if ( newsize > m_maxlen )		// increase buffer, copying the contents
	{
		BYTE *buf = (BYTE *)malloc(newsize);
		if ( m_buf )
		{
			memcpy(buf, m_buf, m_maxlen);
			free(m_buf);
		}
		m_buf = buf;
		m_maxlen = newsize;
		m_len = m_maxlen;
	}
	else							// just change the length
		m_len = newsize;
	seek();
}

BYTE &Packet::operator[](int index)
{
	return m_buf[index];
}

const BYTE &Packet::operator[](int index) const
{
	return m_buf[index];
}

void Packet::write(bool value)
{
	if ( m_index + sizeof(BYTE) > m_maxlen )
		resize(-1);

	m_buf[m_index++] = (BYTE)(value ? 1 : 0);
}

void Packet::write(BYTE value)
{
	if ( m_index + sizeof(BYTE) > m_maxlen )
		resize(-1);

	m_buf[m_index++] = value;
}

void Packet::write(WORD value)
{
	if ( m_index + sizeof(WORD) > m_maxlen )
		resize(-1);

	m_buf[m_index++] = (BYTE)(value >> 8);
	m_buf[m_index++] = (BYTE)(value);
}

void Packet::write(DWORD value)
{
	if ( m_index + sizeof(DWORD) > m_maxlen )
		resize(-1);

	m_buf[m_index++] = (BYTE)(value >> 24);
	m_buf[m_index++] = (BYTE)(value >> 16);
	m_buf[m_index++] = (BYTE)(value >> 8);
	m_buf[m_index++] = (BYTE)(value);
}

void Packet::write(char *buf, long size)
{
	if ( m_index + size > m_maxlen )
		resize(-1);

	for ( long l = 0; l < size; l++ )
	{
		m_buf[m_index++] = buf ? (BYTE)(buf[l]) : (BYTE)(0);
	}
}

void Packet::writeFixed(char *buf, long size)
{
	long len = buf ? strlen(buf) : 0;
	for ( long l = 0; l < size-1; l++ )
	{
		if ( m_index + sizeof(BYTE) > m_maxlen )
			resize(-1);

		if ( l >= len )
			m_buf[m_index++] = (BYTE)(0);
		else
			m_buf[m_index++] = (BYTE)(buf[l]);
	}
	m_buf[m_index++] = (BYTE)(0);
}

void Packet::write(char *buf)
{
	while ( buf && *buf )
	{
		if ( m_index + sizeof(BYTE) > m_maxlen )
			resize(-1);

		m_buf[m_index++] = (BYTE)(*buf);
		buf++;
	}
	m_buf[m_index++] = (BYTE)(0);
}

void Packet::fill()
{
	while ( m_index < m_maxlen )
	{
		m_buf[m_index++] = (BYTE)(0);
	}
}

long Packet::sync()
{
	if ( m_len < m_index )
		m_len = m_index;

	return m_len;
}

DWORD Packet::readDWORD()
{
	if ( m_index + sizeof(DWORD) > m_len )
		return 0;

	return (( m_buf[m_index++] << 24 ) |
			( m_buf[m_index++] << 16 ) |
			( m_buf[m_index++] <<  8 ) |
			( m_buf[m_index++]));
}

WORD Packet::readWORD()
{
	if ( m_index + sizeof(WORD) > m_len )
		return 0;

	return (( m_buf[m_index++] << 8 ) |
			( m_buf[m_index++]));
}

BYTE Packet::readBYTE()
{
	if ( m_index + sizeof(BYTE) > m_len )
		return 0;

	return m_buf[m_index++];
}

bool Packet::readBOOL()
{
	if ( m_index + sizeof(BYTE) > m_len )
		return false;

	return ( m_buf[m_index++] != 0 );
}

void Packet::readString(char *buf, int fixedLen)
{
	char c;
	long maxLen = ( fixedLen > 0 ) ? m_index+fixedLen : m_len;

	while (( m_index < maxLen ) && ( c = m_buf[m_index++] ))
		*buf++ = c;
	*buf = c;
}

void Packet::readStringSafe(char *buf, int fixedLen)
{
	char c;
	long maxLen = ( fixedLen > 0 ) ? m_index+fixedLen : m_len;

	while (( m_index < maxLen ) && ( c = m_buf[m_index++] ))
	{
		if ( c >= 0x20 )
			*buf++ = c;
	}
	*buf = c;
}

char *Packet::dump()
{
	TEMPSTRING(timefmt);
	TEMPSTRING(output);

	time_t timer = time(NULL);
	formattime(timefmt, localtime(&timer), TIME_EXACT);

	sprintf(output, "Packet len=%d id=0x%02x [%s]\n", m_len, m_buf[0], timefmt);
	strcat(output, "        0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");
	strcat(output, "       -- -- -- -- -- -- -- --  -- -- -- -- -- -- -- --\n");

	int byteIndex = 0;
	int whole = m_len >> 4;
	int rem = m_len & 0x0f;
	int idx = 0;

	char bytes[50];
	char chars[17];
	TEMPSTRING(z);
	for ( int i = 0; i < whole; ++i, byteIndex += 16 )
	{
		memset(bytes, 0, sizeof(bytes));
		memset(chars, 0, sizeof(chars));

		for ( int j = 0; j < 16; ++j )
		{
			BYTE c = m_buf[idx++];

			sprintf(z, "%02x", (int)c);
			strcat(bytes, z);
			strcat(bytes, ( j == 7 ) ? "  " : " ");

			if (( c >= 0x20 ) && ( c <= 0x80 ))
			{
				z[0] = c;
				z[1] = 0;
				strcat(chars, z);
			}
			else
				strcat(chars, ".");
		}

		sprintf(z, "%04x   ", byteIndex);
		strcat(output, z);
		strcat(output, bytes);
		strcat(output, "  ");
		strcat(output, chars);
		strcat(output, "\n");
	}

	if ( rem != 0)
	{
		memset(bytes, 0, sizeof(bytes));
		memset(chars, 0, sizeof(chars));

		for ( int j = 0; j < 16; ++j )
		{
			if ( j < rem )
			{
				BYTE c = m_buf[idx++];

				sprintf(z, "%02x", (int)c);
				strcat(bytes, z);
				strcat(bytes, ( j == 7 ) ? "  " : " ");

				if (( c >= 0x20 ) && ( c <= 0x80 ))
				{
					z[0] = c;
					z[1] = 0;
					strcat(chars, z);
				}
				else
					strcat(chars, ".");
			}
			else
				strcat(bytes, "   ");
		}

		sprintf(z, "%04x   ", byteIndex);
		strcat(output, z);
		strcat(output, bytes);
		strcat(output, "  ");
		strcat(output, chars);
		strcat(output, "\n");
	}

	return output;
}

bool Packet::onReceive(NetState *client)
{
	return true;
}

long Packet::getValidLength()
{
	//	NOTE:
	//	since it is used when we are not yet sure how many packets have we received, we should not move current pointer
	long oldIndex = m_index;
	long len = m_len - m_index;
	BYTE packetID = readBYTE();
	Packet *handler = g_Network->getHandler(packetID);

	if ( !handler )
		return len;

	long packetLen = handler->baseLength();
	bool correctLength = true;

	if ( packetLen > 0 )
	{
		if ( len < packetLen )
			correctLength = false;
	}
	else
	{
		if ( len < 3 )
			correctLength = false;
		else
		{
			packetLen = readWORD();
			if ( packetLen < 3 )
				correctLength = false;
		}
	}

	m_index = oldIndex;
	return ( correctLength ? packetLen : 0 );
}

/***************************************************************************
 *
 *
 *	class SendPacket			Send-type packet with priority and target client
 *
 *
 ***************************************************************************/

PacketSend::PacketSend(BYTE id, long len, Priority priority)
{
	m_target = NULL;
	m_priority = priority;
	m_lenPos = -1;
	if ( len > 0 )
		resize(len);
	write(id);
}

PacketSend::PacketSend(PacketSend *other)
{
	copy(*other);
	m_target = other->m_target;
	m_priority = other->m_priority;
	m_lenPos = other->m_lenPos;
}

void PacketSend::initLength()
{
#ifdef DEBUGPACKETS
	g_Log.Debug("Packet %x starts dynamic with pos %d.\n", *this, m_index);
#endif
	m_lenPos = m_index;
	write((WORD)m_lenPos);
}

void PacketSend::fixLength()
{
	if ( m_lenPos != -1 )
	{
#ifdef DEBUGPACKETS
	g_Log.Debug("Packet %x closes dynamic data writing %d as length to pos %d.\n", *this, m_index, m_lenPos);
#endif
		int oldindex = m_index;
		m_index = m_lenPos;
		write((WORD)oldindex);
		m_index = oldindex;
		m_lenPos = -1;
		m_len = m_index;
	}
}

void PacketSend::send(CClient *client)
{
	fixLength();
	if ( client )
		target(client);

	if ( m_target )
	{
		if ( sync() > config.get("network.packet.limit") )
			return;

		g_Sender->schedule(this);
	}
}

void PacketSend::push(CClient *client)
{
	fixLength();
	if ( client )
		target(client);

	if ( m_target )
	{
		if ( sync() > config.get("network.packet.limit") )
		{
			// since we are pushing this packet, we should clear the memory
			//instantly if this packet will never be sent
			delete this;
			return;
		}
		g_Sender->scheduleOnce(this);
	}
	else
		delete this;
}

void PacketSend::target(CClient *client)
{
	m_target = NULL;

	//	validate that the current slot is still taken by this client
	if ( client && client->m_net->isValid(client) )
	{
		m_target = client->m_net;
	}
}

bool PacketSend::onSend(CClient *client)
{
#ifdef DEBUGPACKETS
	g_Log.Debug("TX SEND %s", dump());
#endif

	return true;
}

long PacketSend::getPriority()
{
	return m_priority;
}

/*
stolen from CByteStream.cpp, not yet embedded

  
int CByteStream::AddStringUnicode( const CGString sIn, int maxsize, bool bSwap, int offset )
{
	return( AddStringUnicode(sIn.GetPtr(), sIn.GetLength(), maxsize, bSwap, offset) );
}

int CByteStream::AddStringUnicode( const TCHAR * sIn, int size, int maxsize, bool bSwap, int offset )
{
	int iReturnLen = 0;
	size_t iInSize = min(size,maxsize);
	size_t iOutSize = iInSize * 2 + 1;
	wchar_t * outbuf = new wchar_t[iOutSize];

#ifdef _WIN32

	iReturnLen = MultiByteToWideChar(
		CP_UTF8,          // code page
		0,				  // character-type options
		sIn,			  // address of string to map
		iInSize,		  // number of bytes in string
		(LPWSTR) outbuf,  // address of wide-character buffer
		iOutSize          // size of buffer
		);

	if (( iReturnLen <= 0 ) || ( iReturnLen > iOutSize ))
	{
		outbuf[0] = 0;
		return -1;
	}
	
	iReturnLen = AddRawBytes((BYTE*)outbuf,iReturnLen,offset);

#else

	iconv_t cd = iconv_open("WCHAR_T", sIn);
	if (cd == (iconv_t) -1)
	{
		// Log something
		return -1;
	}

	iconv(cd, NULL, NULL, &outbuf, &iOutSize);
	size_t resConv = iconv(cd, &sIn, &iInSize, &outbuf, &iOutSize);

	if (resConv == (size_t) -1)
	{
		if (errno == EINVAL)
			// memmove (inbuf, inptr, insize);
		else
			return -1;
	}

	if (iconv_close (cd) != 0)
		return -1;
#endif

	return( iReturnLen );
}

int CByteStream::AddRawBytes( const BYTE * bIn, int size, int offset )
{
	if ( sizeof(bIn) > size )
		return 0;

	if ( offset == -1 )
	{
		for ( int i = 0; i < size; i++ )
			m_vBytes->push_back(bIn[i]);
	}
	else
	{
		for ( int i = 0; i < size; i++ )
			m_vBytes->at(offset+i) = bIn[i];
	}

	return( size );
}

*/