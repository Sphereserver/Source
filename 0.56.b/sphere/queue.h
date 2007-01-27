#ifndef QUEUE_H
#define QUEUE_H

#include "../common/common.h"

class ByteQueue
{
public:
	ByteQueue();
	~ByteQueue();

	int bytes();
	void empty();
	BYTE *raw();

	void remove(int bytes);
	void append(ByteQueue &data);
	void append(BYTE *data, int length);
	BYTE *appendStart(int bytes);
	void appendFinish(int bytes);

private:
	BYTE	*m_data;
	int		m_bytes;
	int		m_size;
};

#endif
