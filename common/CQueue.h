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
	int m_iDataQty;
public:
	static const char *m_sClassName;
	CQueueBytes();
	~CQueueBytes();

	void Empty();

	// Peak into/read from the Queue's data.
	int GetDataQty() const
	{
		// How much data is avail?
		return m_iDataQty;
	}
	const BYTE * RemoveDataLock() const;
	void RemoveDataAmount( int iSize );

	// Write to the Queue.
	void AddNewData( const BYTE * pData, int iLen );
	BYTE * AddNewDataLock( int iLen );
	void AddNewDataFinish( int iLen );
};

#endif
