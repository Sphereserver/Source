#include "packet.h"
#include "network.h"

extern int CvtSystemToNUNICODE( NCHAR * pOut, int iSizeOutChars, LPCTSTR pInp, int iSizeInBytes );
extern int CvtNUNICODEToSystem( TCHAR * pOut, int iSizeOutBytes, const NCHAR * pInp, int iSizeInChars );

// on windows we can use the win32 api for converting between unicode<->ascii,
// otherwise we need to convert with our own functions (gcc uses utf32 instead
// of utf16)
// win32 api seems to fail to convert a lot of characters properly, so it is
// better to leave this #define disabled.
#ifdef _WIN32
//#define USE_UNICODE_LIB
#else
#undef USE_UNICODE_LIB
#endif

Packet::Packet(long size) : m_buffer(NULL)
{
	m_expectedLength = size;
	clear();
	resize(size > 0 ? size : PACKET_BUFFERDEFAULT);
}

Packet::Packet(const Packet& other) : m_buffer(NULL)
{
	clear();
	copy(other);
}

Packet::Packet(const BYTE* data, long size) : m_buffer(NULL)
{
	clear();
	m_expectedLength = -1;
	resize(size);
	memcpy(m_buffer, data, size);
}

Packet::~Packet(void)
{
	clear();
}

bool Packet::isValid(void) const
{
	return m_buffer != NULL && m_length > 0;
}

long Packet::getLength(void) const
{
	return m_length;
}

long Packet::getPosition(void) const
{
	return m_position;
}

BYTE* Packet::getData(void) const
{
	return m_buffer;
}

void Packet::clear(void)
{
	if (m_buffer != NULL)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}

	m_bufferSize = 0;
	m_position = 0;
}

void Packet::copy(const Packet& other)
{
	resize(other.getLength());
	m_expectedLength = other.m_expectedLength;
	memcpy(m_buffer, other.getData(), other.getLength());
	seek();
}

void Packet::expand(long size)
{
	if (size < PACKET_BUFFERGROWTH)
		size = PACKET_BUFFERGROWTH;

	long oldPosition = m_position;
	resize(m_bufferSize + size);
	m_position = oldPosition;
}

void Packet::resize(long newsize)
{
	if ( newsize == -1 )			// special case, we running out of space while writing
	{								// auto-expand silently
		long oldPosition = m_position;
		resize(m_bufferSize + PACKET_BUFFERGROWTH);
		m_position = oldPosition;
		return;
	}

	if ( newsize > m_bufferSize )		// increase buffer, copying the contents
	{
		BYTE* buffer = new BYTE[newsize];
		if (m_buffer != NULL)
		{
			memcpy(buffer, m_buffer, m_bufferSize);
			delete[] m_buffer;
		}
		
		m_buffer = buffer;
		m_bufferSize = newsize;
		m_length = m_bufferSize;
	}
	else							// just change the length
		m_length = newsize;

	seek();
}

void Packet::seek(long pos)
{
	m_position = pos;
}

void Packet::skip(long count)
{
	m_position += count;
}

BYTE &Packet::operator[](long index)
{
	return m_buffer[index];
}

const BYTE &Packet::operator[](long index) const
{
	return m_buffer[index];
}

void Packet::writeBool(const bool value)
{
	if ((m_position + sizeof(BYTE)) > m_bufferSize)
		expand(sizeof(BYTE));

	m_buffer[m_position++] = (BYTE)(value ? 1 : 0);
}

void Packet::writeCharASCII(const char value)
{
	if ((m_position + sizeof(char)) > m_bufferSize)
		expand(sizeof(char));

	m_buffer[m_position++] = (BYTE)(value);
}

void Packet::writeCharUNICODE(const WCHAR value)
{
	if ((m_position + sizeof(WCHAR)) > m_bufferSize)
		expand(sizeof(WCHAR));

	m_buffer[m_position++] = (BYTE)(value);
	m_buffer[m_position++] = (BYTE)(value >> 8);
}

void Packet::writeCharNUNICODE(const WCHAR value)
{
	if ((m_position + sizeof(WCHAR)) > m_bufferSize)
		expand(sizeof(WCHAR));

	m_buffer[m_position++] = (BYTE)(value >> 8);
	m_buffer[m_position++] = (BYTE)(value);
}

void Packet::writeByte(const BYTE value)
{
	if ((m_position + sizeof(BYTE)) > m_bufferSize)
		expand(sizeof(BYTE));

	m_buffer[m_position++] = value;
}

void Packet::writeData(const BYTE* buffer, long size)
{
	if ((m_position + (sizeof(BYTE) * size)) > m_bufferSize)
		expand((sizeof(BYTE) * size));

	memcpy(&m_buffer[m_position], buffer, sizeof(BYTE) * size);
	m_position += size;
}

void Packet::writeInt16(const WORD value)
{
	if ((m_position + sizeof(WORD)) > m_bufferSize)
		expand(sizeof(WORD));

	m_buffer[m_position++] = (BYTE)(value >> 8);
	m_buffer[m_position++] = (BYTE)(value);
}

void Packet::writeInt32(const DWORD value)
{
	if ((m_position + sizeof(DWORD)) > m_bufferSize)
		expand(sizeof(DWORD));

	m_buffer[m_position++] = (BYTE)(value >> 24);
	m_buffer[m_position++] = (BYTE)(value >> 16);
	m_buffer[m_position++] = (BYTE)(value >> 8);
	m_buffer[m_position++] = (BYTE)(value);
}

void Packet::writeInt64(const DWORD hi, const DWORD lo)
{
	if ((m_position + sizeof(DWORD) + sizeof(DWORD)) > m_bufferSize)
		expand(sizeof(DWORD) + sizeof(DWORD));

	m_buffer[m_position++] = (BYTE)(hi);
	m_buffer[m_position++] = (BYTE)(hi >> 8);
	m_buffer[m_position++] = (BYTE)(hi >> 16);
	m_buffer[m_position++] = (BYTE)(hi >> 24);
	m_buffer[m_position++] = (BYTE)(lo);
	m_buffer[m_position++] = (BYTE)(lo >> 8);
	m_buffer[m_position++] = (BYTE)(lo >> 16);
	m_buffer[m_position++] = (BYTE)(lo >> 24);
}

void Packet::writeStringASCII(const char* value, bool terminate)
{
	while (value && *value)
	{
		writeCharASCII(*value);
		value++;
	}

	if (terminate)
		writeCharASCII('\0');
}

void Packet::writeStringFixedASCII(const char* value, long size, bool terminate)
{
	long valueLength = value ? strlen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharASCII('\0');
		else
			writeCharASCII(value[l]);
	}
}

void Packet::writeStringASCII(const WCHAR* value, bool terminate)
{
#ifdef USE_UNICODE_LIB

	char* buffer = new char[MB_CUR_MAX];
	while (value && *value)
	{
		int len = wctomb(buffer, *value);
		for (int i = 0; i < len; i++)
			writeCharASCII(buffer[i]);

		value++;
	}
	delete[] buffer;
	
	if (terminate)
		writeCharASCII('\0');
#else

	char* buffer = Str_GetTemp();

	// need to flip byte order to convert UNICODE to ASCII
	{
		int i;
		for (i = 0; value[i]; i++)
			((WCHAR*)buffer)[i] = ((NWORD*)value)[i];
		((WCHAR*)buffer)[i] = '\0';
	}

	CvtNUNICODEToSystem(buffer, THREAD_STRING_LENGTH, (NWORD*)buffer, THREAD_STRING_LENGTH);

	writeStringASCII(buffer, terminate);
#endif
}

void Packet::writeStringFixedASCII(const WCHAR* value, long size, bool terminate)
{
#ifdef USE_UNICODE_LIB

	char* buffer = new char[MB_CUR_MAX];
	long valueLength = value ? wcslen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharASCII('\0');
		else
		{
			int len = wctomb(buffer, value[l]);
			for (int i = 0; i < len; i++)
				writeCharASCII(buffer[i]);
		}
	}

	delete[] buffer;
#else

	char* buffer = Str_GetTemp();

	// need to flip byte order to convert UNICODE to ASCII
	{
		int i;
		for (i = 0; value[i]; i++)
			((WCHAR*)buffer)[i] = ((NWORD*)value)[i];
		((WCHAR*)buffer)[i] = '\0';
	}
	
	CvtNUNICODEToSystem(buffer, THREAD_STRING_LENGTH, (NWORD*)buffer, THREAD_STRING_LENGTH);

	writeStringFixedASCII(buffer, size, terminate);
#endif
}

void Packet::writeStringUNICODE(const char* value, bool terminate)
{
#ifdef USE_UNICODE_LIB

	WCHAR c;
	while (value && *value)
	{
		mbtowc(&c, value, MB_CUR_MAX);
		writeCharUNICODE(c);
		value++;
	}

	if (terminate)
		writeCharUNICODE('\0');
#else

	WCHAR* buffer = (WCHAR*)Str_GetTemp();
	CvtSystemToNUNICODE((NWORD*)buffer, THREAD_STRING_LENGTH / sizeof(WCHAR), value, strlen(value));
	
	writeStringNUNICODE(buffer, terminate);
#endif
}

void Packet::writeStringFixedUNICODE(const char* value, long size, bool terminate)
{
#ifdef USE_UNICODE_LIB
	WCHAR c;
	long valueLength = value ? strlen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharUNICODE('\0');
		else
		{
			mbtowc(&c, &value[l], MB_CUR_MAX);
			writeCharUNICODE(c);
		}
	}
#else

	WCHAR* buffer = (WCHAR*)Str_GetTemp();
	CvtSystemToNUNICODE((NWORD*)buffer, THREAD_STRING_LENGTH / sizeof(WCHAR), value, strlen(value));
	
	writeStringFixedNUNICODE(buffer, size, terminate);
#endif
}

void Packet::writeStringUNICODE(const WCHAR* value, bool terminate)
{
	while (value && *value)
	{
		writeCharUNICODE(*value);
		value++;
	}

	if (terminate)
		writeCharUNICODE('\0');
}

void Packet::writeStringFixedUNICODE(const WCHAR* value, long size, bool terminate)
{
#ifdef USE_UNICODE_LIB

	long valueLength = value ? wcslen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharUNICODE('\0');
		else
			writeCharUNICODE(value[l]);
	}
#else

	if (size <= 0)
		return;
	else if (terminate)
		size--;

	bool zero = false;
	for (long l = 0; l < size; l++)
	{
		if (zero == false)
		{
			if (value[l] == '\0')
				zero = true;

			writeCharUNICODE(value[l]);
		}
		else
			writeCharUNICODE('\0');
	}

	if (terminate)
		writeCharUNICODE('\0');
#endif
}

void Packet::writeStringNUNICODE(const char* value, bool terminate)
{
#ifdef USE_UNICODE_LIB

	WCHAR c;
	while (value && *value)
	{
		mbtowc(&c, value, MB_CUR_MAX);
		writeCharNUNICODE(c);
		value++;
	}

	if (terminate)
		writeCharNUNICODE('\0');
#else

	WCHAR* buffer = (WCHAR*)Str_GetTemp();
	CvtSystemToNUNICODE((NWORD*)buffer, THREAD_STRING_LENGTH / sizeof(WCHAR), value, strlen(value));
	
	writeStringUNICODE(buffer, terminate);
#endif
}

void Packet::writeStringFixedNUNICODE(const char* value, long size, bool terminate)
{
#ifdef USE_UNICODE_LIB
	WCHAR c;
	long valueLength = value ? strlen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharNUNICODE('\0');
		else
		{
			mbtowc(&c, &value[l], MB_CUR_MAX);
			writeCharNUNICODE(c);
		}
	}
#else
	
	WCHAR* buffer = (WCHAR*)Str_GetTemp();
	CvtSystemToNUNICODE((NWORD*)buffer, THREAD_STRING_LENGTH / sizeof(WCHAR), value, strlen(value));
	
	writeStringFixedUNICODE(buffer, size, terminate);
#endif
}

void Packet::writeStringNUNICODE(const WCHAR* value, bool terminate)
{
	while (value && *value)
	{
		writeCharNUNICODE(*value);
		value++;
	}

	if (terminate)
		writeCharNUNICODE('\0');
}

void Packet::writeStringFixedNUNICODE(const WCHAR* value, long size, bool terminate)
{
#ifdef USE_UNICODE_LIB

	long valueLength = value ? wcslen(value) : 0;
	if (terminate && valueLength >= size)
		valueLength = size - 1;

	for (long l = 0; l < size; l++)
	{
		if (l >= valueLength)
			writeCharNUNICODE('\0');
		else
			writeCharNUNICODE(value[l]);
	}
#else

	if (size <= 0)
		return;
	else if (terminate)
		size--;

	bool zero = false;
	for (long l = 0; l < size; l++)
	{
		if (zero == false)
		{
			if (value[l] == '\0')
				zero = true;

			writeCharNUNICODE(value[l]);
		}
		else
			writeCharNUNICODE('\0');
	}

	if (terminate)
		writeCharNUNICODE('\0');
#endif
}

void Packet::fill(void)
{
	while (m_position < m_bufferSize)
		writeByte(0);
}

long Packet::sync(void)
{
	if (m_length < m_position)
		m_length = m_position;

	return m_length;
}

void Packet::trim(void)
{
	if (m_length > m_position)
		m_length = m_position;
}

bool Packet::readBool(void)
{
	if (m_position + sizeof(BYTE) > m_length)
		return false;

	return (m_buffer[m_position++] != 0);
}

char Packet::readCharASCII(void)
{
	if (m_position + sizeof(char) > m_length)
		return '\0';

	return m_buffer[m_position++];
}

WCHAR Packet::readCharUNICODE(void)
{
	if (m_position + sizeof(WCHAR) > m_length)
		return '\0';

	WCHAR wc = ((m_buffer[m_position + 1] << 8) |
			   (m_buffer[m_position]));

	m_position += 2;
	return wc;
}

WCHAR Packet::readCharNUNICODE(void)
{
	if (m_position + sizeof(WCHAR) > m_length)
		return '\0';

	WCHAR wc = ((m_buffer[m_position] << 8) |
			   (m_buffer[m_position + 1]));

	m_position += 2;
	return wc;
}

BYTE Packet::readByte(void)
{
	if (m_position + sizeof(BYTE) > m_length)
		return 0;

	return m_buffer[m_position++];
}

WORD Packet::readInt16(void)
{
	if (m_position + sizeof(WORD) > m_length)
		return 0;

	WORD w =(( m_buffer[m_position] <<  8 ) |
			 ( m_buffer[m_position + 1]));

	m_position += 2;
	return w;
}

DWORD Packet::readInt32(void)
{
	if (m_position + sizeof(DWORD) > m_length)
		return 0;

	DWORD dw = ((m_buffer[m_position] << 24) |
			   (m_buffer[m_position + 1] << 16) |
			   (m_buffer[m_position + 2] << 8) |
			   (m_buffer[m_position + 3]));

	m_position += 4;
	return dw;
}

void Packet::readStringASCII(char* buffer, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer = '\0';
		return;
	}

	long l;
	for (l = 0; l < length; l++)
		buffer[l] = readCharASCII();

	// ensure text is null-terminated
	if (includeNull)
		buffer[l-1] = '\0';
	else
		buffer[l] = '\0';
}

void Packet::readStringASCII(WCHAR* buffer, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer[0] = '\0';
		return;
	}

#ifdef USE_UNICODE_LIB

	char* bufferReal = new char[length + 1];
	readStringASCII(bufferReal, length, includeNull);
	long l = mbstowcs(buffer, bufferReal, length + 1);
	delete[] bufferReal;
#else
	
	char* bufferReal = new char[length + 1];
	readStringASCII(bufferReal, length, includeNull);
	CvtSystemToNUNICODE((NWORD*)buffer, length, bufferReal, length + 1);
	delete[] bufferReal;

	// need to flip byte order to convert NUNICODE to UNICODE
	{
		int i;
		for (i = 0; buffer[i]; i++)
			buffer[i] = ((NWORD*)buffer)[i];
		buffer[i] = '\0';
	}
#endif
}

void Packet::readStringUNICODE(WCHAR* buffer, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer[0] = '\0';
		return;
	}

	long l;
	for (l = 0; l < length; l++)
		buffer[l] = readCharUNICODE();

	// ensure text is null-terminated
	if (includeNull)
		buffer[l-1] = '\0';
	else
		buffer[l] = '\0';
}

void Packet::readStringUNICODE(char* buffer, long bufferSize, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer[0] = '\0';
		return;
	}

#ifdef USE_UNICODE_LIB

	WCHAR* bufferReal = new WCHAR[length + 1];
	readStringUNICODE(bufferReal, length, includeNull);
	long l = wcstombs(buffer, bufferReal, bufferSize);
	delete[] bufferReal;
#else

	WCHAR* bufferReal = new WCHAR[length + 1];
	readStringNUNICODE(bufferReal, length, includeNull);
	CvtNUNICODEToSystem(buffer, bufferSize, (NWORD*)bufferReal, length + 1);
	delete[] bufferReal;
#endif
}

void Packet::readStringNUNICODE(WCHAR* buffer, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer[0] = '\0';
		return;
	}

	long l;
	for (l = 0; l < length; l++)
		buffer[l] = readCharNUNICODE();

	// ensure text is null-terminated
	if (includeNull)
		buffer[l-1] = '\0';
	else
		buffer[l] = '\0';
}

void Packet::readStringNUNICODE(char* buffer, long bufferSize, long length, bool includeNull)
{
	if (length < 1)
	{
		buffer[0] = '\0';
		return;
	}

#ifdef USE_UNICODE_LIB

	WCHAR* bufferReal = new WCHAR[length + 1];
	readStringNUNICODE(bufferReal, length, includeNull);
	long l = wcstombs(buffer, bufferReal, bufferSize);
	delete[] bufferReal;
#else

	WCHAR* bufferReal = new WCHAR[length + 1];
	readStringUNICODE(bufferReal, length, includeNull);
	CvtNUNICODEToSystem(buffer, bufferSize, (NWORD*)bufferReal, length + 1);
	delete[] bufferReal;
#endif
}

long Packet::readStringNullASCII(char* buffer, long maxlength)
{
	long l;
	for (l = 0; l < maxlength; l++)
	{
		buffer[l] = readCharASCII();
		if (buffer[l] == '\0')
			return l;
	}

	// ensure text is null-terminated
	buffer[l] = '\0';
	return l;
}

long Packet::readStringNullASCII(WCHAR* buffer, long maxlength)
{
#ifdef USE_UNICODE_LIB

	char* bufferReal = new char[maxlength + 1];
	long length = readStringNullASCII(bufferReal, maxlength);
	long l = mbstowcs(buffer, bufferReal, maxlength + 1);
	delete[] bufferReal;
#else

	char* bufferReal = new char[maxlength + 1];
	readStringNullASCII(bufferReal, maxlength);
	long l = CvtSystemToNUNICODE((NWORD*)buffer, maxlength, bufferReal, maxlength + 1);
	delete[] bufferReal;

	// need to flip byte order to convert NUNICODE to UNICODE
	{
		int i;
		for (i = 0; buffer[i]; i++)
			buffer[i] = ((NWORD*)buffer)[i];
		buffer[i] = '\0';
	}
#endif

	return l;
}

long Packet::readStringNullUNICODE(WCHAR* buffer, long maxlength)
{
	long l;
	for (l = 0; l < maxlength; l++)
	{
		buffer[l] = readCharUNICODE();
		if (buffer[l] == '\0')
			return l;
	}

	// ensure text is null-terminated
	buffer[l] = '\0';
	return l;
}

long Packet::readStringNullUNICODE(char* buffer, long bufferSize, long maxlength)
{
#ifdef USE_UNICODE_LIB

	WCHAR* bufferReal = new WCHAR[maxlength + 1];
	long length = readStringNullUNICODE(bufferReal, maxlength);
	long l = wcstombs(buffer, bufferReal, bufferSize);
	delete[] bufferReal;
#else

	WCHAR* bufferReal = new WCHAR[maxlength + 1];
	long length = readStringNullNUNICODE(bufferReal, maxlength);
	long l = CvtNUNICODEToSystem(buffer, bufferSize, (NWORD*)bufferReal, maxlength + 1);
	delete[] bufferReal;
#endif

	return l;
}

long Packet::readStringNullNUNICODE(WCHAR* buffer, long maxlength)
{
	long l;
	for (l = 0; l < maxlength; l++)
	{
		buffer[l] = readCharNUNICODE();
		if (buffer[l] == '\0')
			return l;
	}

	// ensure text is null-terminated
	buffer[l] = '\0';
	return l;
}

long Packet::readStringNullNUNICODE(char* buffer, long bufferSize, long maxlength)
{
#ifdef USE_UNICODE_LIB

	WCHAR* bufferReal = new WCHAR[maxlength + 1];
	long length = readStringNullNUNICODE(bufferReal, maxlength);
	long l = wcstombs(buffer, bufferReal, bufferSize);
	delete[] bufferReal;
#else

	WCHAR* bufferReal = new WCHAR[maxlength + 1];
	long length = readStringNullUNICODE(bufferReal, maxlength);
	long l =CvtNUNICODEToSystem(buffer, bufferSize, (NWORD*)bufferReal, maxlength + 1);
	delete[] bufferReal;
#endif

	return l;
}

TCHAR* Packet::dump(void) const
{
	// macro to hide password bytes in release mode
#ifndef _DEBUG
#	define PROTECT_BYTE(_b_)  if ((m_buffer[0] == XCMD_ServersReq && idx >= 32 && idx <= 61) || \
						       (m_buffer[0] == XCMD_CharListReq && idx >= 36 && idx <= 65)) _b_ = '*'
#else
#	define PROTECT_BYTE(_b_)
#endif
	

	TemporaryString z;
	TemporaryString output;

	sprintf(output, "Packet len=%d id=0x%02x [%s]\n", m_length, m_buffer[0], CGTime::GetCurrentTime().Format(NULL));
	strcat(output, "        0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F\n");
	strcat(output, "       -- -- -- -- -- -- -- --  -- -- -- -- -- -- -- --\n");

	int byteIndex = 0;
	int whole = m_length >> 4;
	int rem = m_length & 0x0f;
	int idx = 0;

	TCHAR bytes[50];
	TCHAR chars[17];

	for (int i = 0; i < whole; i++, byteIndex += 16 )
	{
		memset(bytes, 0, sizeof(bytes));
		memset(chars, 0, sizeof(chars));

		for (int j = 0; j < 16; j++)
		{
			BYTE c = m_buffer[idx++];
			PROTECT_BYTE(c);

			sprintf(z, "%02x", (int)c);
			strcat(bytes, z);
			strcat(bytes, (j == 7) ? "  " : " ");

			if ((c >= 0x20) && (c <= 0x80))
			{
				z[0] = c;
				z[1] = '\0';
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

	if (rem != 0)
	{
		memset(bytes, 0, sizeof(bytes));
		memset(chars, 0, sizeof(chars));

		for (int j = 0; j < 16; j++)
		{
			if (j < rem)
			{
				BYTE c = m_buffer[idx++];
				PROTECT_BYTE(c);

				sprintf(z, "%02x", (int)c);
				strcat(bytes, z);
				strcat(bytes, (j == 7) ? "  " : " ");

				if ((c >= 0x20) && (c <= 0x80))
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
#undef PROTECT_BYTE
}

long Packet::checkLength(NetState* client, Packet* packet)
{
	long packetLength = getExpectedLength(client, packet);

	if (packetLength < 0)
	{
		// dynamic length
		if (packet->getLength() < 3)
			return 0;

		long pos = packet->getPosition();
		packet->skip(1);
		packetLength = packet->readInt16();
		packet->seek(pos);

		if ( packetLength < 3 )
			return 0;
	}

	if (packet->getLength() < packetLength)
		return 0;

	return packetLength;
}

long Packet::getExpectedLength(NetState* client, Packet* packet)
{
	return m_expectedLength;
}

bool Packet::onReceive(NetState* client)
{
	return true;
}


/***************************************************************************
 *
 *
 *	class SendPacket			Send-type packet with priority and target client
 *
 *
 ***************************************************************************/
PacketSend::PacketSend(BYTE id, long len, Priority priority)
	: m_priority(priority), m_target(NULL), m_lengthPosition(-1)
{
	if (len > 0)
		resize(len);

	writeByte(id);
}

PacketSend::PacketSend(const PacketSend *other)
{
	copy(*other);
	m_target = other->m_target;
	m_priority = other->m_priority;
	m_lengthPosition = other->m_lengthPosition;
	m_position = other->m_position;
}

void PacketSend::initLength(void)
{
//	DEBUGNETWORK(("Packet %x starts dynamic with pos %ld.\n", m_buffer[0], m_position));

	m_lengthPosition = m_position;
	writeInt16(m_lengthPosition);;
}

void PacketSend::fixLength()
{
	if (m_lengthPosition >= 0)
	{
//		DEBUGNETWORK(("Packet %x closes dynamic data writing %ld as length to pos %d.\n", m_buffer[0], m_position, m_lengthPosition));

		long oldPosition = m_position;
		m_position = m_lengthPosition;
		writeInt16(oldPosition);
		m_position = oldPosition;
		m_lengthPosition = -1;
		m_length = m_position;
	}
}

PacketSend* PacketSend::clone(void) const
{
	return new PacketSend(this);
}

void PacketSend::send(const CClient *client, bool appendTransaction)
{
	ADDTOCALLSTACK("PacketSend::send");

	fixLength();
	if (client != NULL)
		target(client);

	if (m_target != NULL)
	{
		if (sync() > NETWORK_MAXPACKETLEN)
			return;

		g_NetworkOut.schedule(this, appendTransaction);
	}
}

void PacketSend::push(const CClient *client, bool appendTransaction)
{
	ADDTOCALLSTACK("PacketSend::push");

	fixLength();
	if (client != NULL)
		target(client);

	if (m_target != NULL)
	{
		if (sync() > NETWORK_MAXPACKETLEN)
		{
			// since we are pushing this packet, we should clear the memory
			//instantly if this packet will never be sent
			DEBUGNETWORK(("Packet deleted due to exceeding maximum packet size (%ld/%d).\n", m_length, NETWORK_MAXPACKETLEN));
			delete this;
			return;
		}

		g_NetworkOut.scheduleOnce(this, appendTransaction);
	}
	else
	{
		//DEBUGNETWORK(("Packet deleted due to no target set.\n"));
		delete this;
	}
}

void PacketSend::target(const CClient* client)
{
	ADDTOCALLSTACK("PacketSend::target");

	m_target = NULL;

	//	validate that the current slot is still taken by this client
	if (client != NULL && client->GetNetState()->isValid(client))
		m_target = client->GetNetState();
}

bool PacketSend::onSend(const CClient* client)
{
	ADDTOCALLSTACK("PacketSend::onSend");
	return true;
}

void PacketSend::onSent(CClient* client)
{
	ADDTOCALLSTACK("PacketSend::onSent");
}


/***************************************************************************
 *
 *
 *	class SimplePacketTransaction		Class for defining a single packet to be sent
 *
 *
 ***************************************************************************/
SimplePacketTransaction::~SimplePacketTransaction(void)
{
	if (m_packet != NULL)
		delete m_packet;
}


/***************************************************************************
 *
 *
 *	class ExtendedPacketTransaction		Class for defining a set of packets to be sent together
 *
 *
 ***************************************************************************/
ExtendedPacketTransaction::~ExtendedPacketTransaction(void)
{
	for (std::list<PacketSend*>::iterator it = m_packets.begin(); it != m_packets.end(); it++)
		delete *it;

	m_packets.clear();
}


/***************************************************************************
 *
 *
 *	class OpenPacketTransaction		Class to automatically begin and end a transaction
 *
 *
 ***************************************************************************/
OpenPacketTransaction::OpenPacketTransaction(const CClient* client, long priority)
{
	ASSERT(client != NULL);

	m_client = client->GetNetState();
	if (m_client != NULL)
		m_client->beginTransaction(priority);
}

OpenPacketTransaction::~OpenPacketTransaction(void)
{
	if (m_client != NULL)
		m_client->endTransaction();
}
