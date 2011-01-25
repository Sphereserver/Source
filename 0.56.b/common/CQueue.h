//
// CQueue.h
//

#ifndef _INC_CQUEUE_H
#define _INC_CQUEUE_H

#include "CMemBlock.h"

class CQueueBytes
{
	// Create an arbitrary queue of data.
	// NOTE: I know this is not a real queue yet, but i'm working on it.
private:
	CMemLenBlock m_Mem;
	size_t m_iDataQty;
public:
	static const char *m_sClassName;
	CQueueBytes();
	~CQueueBytes();

private:
	CQueueBytes(const CQueueBytes& copy);
	CQueueBytes& operator=(const CQueueBytes& other);

public:
	void Empty();

	// Peak into/read from the Queue's data.
	size_t GetDataQty() const
	{
		// How much data is avail?
		return m_iDataQty;
	}
	const BYTE * RemoveDataLock() const;
	void RemoveDataAmount( size_t iSize );

	// Write to the Queue.
	void AddNewData( const BYTE * pData, size_t iLen );
	BYTE * AddNewDataLock( size_t iLen );
	void AddNewDataFinish( size_t iLen );
};

#endif
