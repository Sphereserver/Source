#include "queue.h"

ByteQueue::ByteQueue()
{
	m_data = NULL;
	m_bytes = 0;
	m_size = 0;
}

ByteQueue::~ByteQueue()
{
	if( m_data != NULL )
	{
		delete []m_data;
		m_data = NULL;
	}
}

int ByteQueue::bytes()
{
	return m_bytes;
}

void ByteQueue::empty() {
	m_bytes = 0;
}

void ByteQueue::remove(int bytes)
{
	if( bytes > m_bytes )
	{
		bytes = m_bytes;
	}
	m_bytes -= bytes;
	memmove(m_data, m_data + bytes, m_bytes);
}

BYTE *ByteQueue::raw()
{
	return m_data;
}

BYTE *ByteQueue::appendStart(int bytes)
{
	if( m_bytes + bytes >= m_size )
	{
		int newSize = (m_size + bytes + 0x100) &~ 0xff;
		BYTE *newBuf = new BYTE[newSize];
		if( m_data != NULL )
		{
			memmove(newBuf, m_data, m_size);
		}
		m_data = newBuf;
		m_size = newSize;
	}
	return m_data + bytes;
}

void ByteQueue::appendFinish(int bytes)
{
	m_bytes += bytes;
}

void ByteQueue::append(BYTE *data, int length)
{
	memcpy(appendStart(length), data, length);
	appendFinish(length);
}

void ByteQueue::append(ByteQueue &data)
{
	append(data.raw(), data.bytes());
}
