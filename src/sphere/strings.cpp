#include "../common/graycom.h"
#include "strings.h"
#include "threads.h"

#define	STRING_DEFAULT_SIZE	40

/*
 * AbstractString
*/

AbstractString::AbstractString()
{
	m_buf = 0;
	m_length = 0;
	m_realLength = 0;
	ensureLength(STRING_DEFAULT_SIZE);
}

AbstractString::~AbstractString()
{
	destroy();
}

void AbstractString::ensureLength(size_t newLength)
{
	// child class implements this
	UNREFERENCED_PARAMETER(newLength);
}

void AbstractString::destroy()
{
	// child class implements this
}

size_t AbstractString::length()
{
	return m_length;
}

size_t AbstractString::realLength()
{
	return m_realLength;
}

bool AbstractString::isEmpty()
{
	return m_length != 0;
}

const char *AbstractString::toBuffer()
{
	return m_buf;
}

char AbstractString::charAt(size_t index)
{
	return m_buf[index];
}

void AbstractString::setAt(size_t index, char c)
{
	m_buf[index] = c;
}

void AbstractString::append(const char *s)
{
	ensureLength(m_length + strlen(s));
	strcat(m_buf, s);
}

void AbstractString::replace(char what, char toWhat)
{
	for ( size_t i = 0; i < m_length; ++i )
	{
		if( m_buf[i] == what )
			m_buf[i] = toWhat;
	}
}

int AbstractString::compareTo(const char *s)
{
	return strcmp(m_buf, s);
}

int AbstractString::compareToIgnoreCase(const char *s)
{
	return strcmpi(m_buf, s);
}

bool AbstractString::equals(const char *s)
{
	return strcmp(m_buf, s) == 0;
}

bool AbstractString::equalsIgnoreCase(const char *s)
{
	return strcmpi(m_buf, s) == 0;
}

bool AbstractString::startsWith(const char *s)
{
	return strnicmp(m_buf, s, strlen(s)) == 0;
}

bool AbstractString::startsWithHead(const char *s)
{
	for( size_t i = 0; ; ++i )
	{
		char ch1 = static_cast<unsigned char>(tolower(m_buf[0]));
		char ch2 = static_cast<unsigned char>(tolower(s[0]));
		if( ch2 == '\0' )
		{
			if( !isalnum(ch1) )
			{
				return true;
			}
			return false;
		}
		if( ch1 != ch2 )
		{
			return false;
		}
	}
}

int AbstractString::indexOf(char c)
{
	char *pos = strchr(m_buf, c);
	return ( pos == NULL ) ? -1 : pos - m_buf;
}

int AbstractString::indexOf(const char *s)
{
	char *pos = strstr(m_buf, s);
	return ( pos == NULL ) ? -1 : pos - m_buf;
}

int AbstractString::lastIndexOf(char c)
{
	char *pos = strrchr(m_buf, c);
	return ( pos == NULL ) ? -1 : pos - m_buf;
}

AbstractString::operator LPCTSTR() const
{
	return m_buf;
}

AbstractString::operator char*()
{
	return m_buf;
}

AbstractString::operator const char*&() const
{
	return const_cast<const char *&>(m_buf);
}

/*
 * String
*/

String::String()
{
}

void String::destroy()
{
	if( m_realLength && ( m_buf != NULL ))
	{
		delete[] m_buf;
		m_buf = NULL;
		m_realLength = 0;
		m_length = 0;
	}
}

void String::ensureLength(size_t newLength)
{
	if( newLength >= m_realLength )
	{
		// always grow with 20% extra space to decrease number of future grows
		m_realLength = newLength + newLength/5;
		char *newBuf = new char[m_realLength+1];

		if( newBuf == NULL )
		{
			throw CGrayError(LOGL_CRIT, 0, "Run out of memory while allocating memory for string");
		}

		if( m_buf != NULL )
		{
			strncpy(newBuf, m_buf, m_length);
			delete[] m_buf;
		}
		newBuf[m_length] = 0;
		m_buf = newBuf;
	}
	m_length = newLength;
	m_buf[m_length] = 0;
}

/*
 * TemporaryString
*/

size_t TemporaryString::m_tempPosition = 0;
char TemporaryString::m_tempStrings[MAX_TEMP_LINES_NO_CONTEXT][THREAD_STRING_LENGTH];

TemporaryString::TemporaryString()
{
	m_useHeap = true;
	AbstractSphereThread *current = static_cast<AbstractSphereThread*> (ThreadHolder::current());
	if( current != NULL )
	{
		// allocate from thread context
		current->allocateString(*this);
	}
	else
	{
		// allocate from static buffer when context is not available
		init(&m_tempStrings[m_tempPosition++][0], NULL);
		if( m_tempPosition >= MAX_TEMP_LINES_NO_CONTEXT )
		{
			m_tempPosition = 0;
		}
	}
}

TemporaryString::TemporaryString(char *buffer, char *state)
{
	init(buffer, state);
}

TemporaryString::~TemporaryString()
{
	destroy();
}

void TemporaryString::init(char *buffer, char *state)
{
	m_useHeap = false;
	m_buf = buffer;
	m_state = state;
	if( m_state != NULL )
	{
		*m_state = 'U';
		m_realLength = THREAD_STRING_LENGTH;
	}
	else
	{
		m_realLength = THREAD_STRING_LENGTH;
	}
	m_length = 0;
}

void TemporaryString::destroy()
{
	if( m_useHeap )
	{
		String::destroy();
	}
	else
	{
		if( m_state != NULL )
		{
			*m_state = '\0';
		}
	}
}

void TemporaryString::ensureLength(size_t newLength)
{
	if( m_useHeap )
	{
		String::ensureLength(newLength);
	}
	else
	{
		if( newLength >= m_realLength )
		{
			// switch back to behaving like a normal string, since the thread context does not have the
			// capacity we need. To accomplish this we:
			// 1. create a new buffer with the desired length (+20%)
			// 2. copy the old buffer content to the new buffer
			// 3. replace the old buffer with the new buffer
			// 4. clear the state to allow the old buffer to be used elsewhere
			// 5. flag this string instance to use the heap (String::)
			
			m_realLength = newLength + newLength/5;
			char *newBuf = new char[m_realLength+1];
			if ( newBuf == NULL )
				throw CGrayError(LOGL_CRIT, 0, "Run out of memory while allocating memory for string");

			strncpy(newBuf, m_buf, m_length);
			newBuf[m_length] = '\0';
			
			m_buf = newBuf;
			if( m_state != NULL )
			{
				*m_state = '\0';
				m_state = NULL;
			}

			m_useHeap = true;
		}

		m_length = newLength;
		m_buf[m_length] = '\0';
	}
}
