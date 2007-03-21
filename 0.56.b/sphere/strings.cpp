#include "strings.h"
#include "threads.h"

#define	STRING_DEFAULT_SIZE	40

/*
 * IString
*/

IString::IString()
{
	m_buf = 0;
	m_length = 0;
	m_realLength = 0;
	ensureLength(STRING_DEFAULT_SIZE);
}

IString::~IString()
{
	destroy();
}

void IString::ensureLength(int newLength)
{
	// child class implements this
}

void IString::destroy()
{
	// child class implements this
}

int IString::length()
{
	return m_length;
}

/*
 * String
*/

String::String()
{
}

void String::destroy()
{
	//	TODO:
}

void String::ensureLength(int newLength)
{
	//	TODO:
}

/*
 * TemporaryString
*/

TemporaryString::TemporaryString()
{
	m_useHeap = true;
	AbstractSphereThread *current = static_cast<AbstractSphereThread*> (ThreadHolder::current());
	if( current != NULL )
	{
		current->allocateString(*this);
	}
}

TemporaryString::TemporaryString(char *buffer, char *state)
{
	init(buffer, state);
}

void TemporaryString::init(char *buffer, char *state)
{
	m_useHeap = false;
	m_buf = buffer;
	m_state = state;
	*m_state = 'U';
	m_length = THREAD_STRING_LENGTH;
	m_realLength = THREAD_STRING_LENGTH;
}

void TemporaryString::destroy()
{
	if( m_useHeap )
	{
		String::destroy();
	}
	else
	{
		*m_state = '\0';
	}
}

void TemporaryString::ensureLength(int newLength)
{
	if( m_useHeap )
	{
		String::ensureLength(newLength);
	}
}
