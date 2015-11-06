/**
* @file CQueue.h
*/

#ifndef _INC_CQUEUE_H
#define _INC_CQUEUE_H

#include "CMemBlock.h"

/**
* @brief FIFO data buffer
*/
class CQueueBytes
{
	// Create an arbitrary queue of data.
	// NOTE: I know this is not a real queue yet, but i'm working on it.
private:
	CMemLenBlock m_Mem;  ///< Data buffer.
	size_t m_iDataQty;  ///< Item count of the data queue.
public:
	static const char *m_sClassName;
	CQueueBytes();
	~CQueueBytes();

private:
	/**
	* @brief No copy on construction allowed.
    */
	CQueueBytes(const CQueueBytes& copy);
	/**
	* @brief No copy allowed.
    */
	CQueueBytes& operator=(const CQueueBytes& other);

public:
	/**
	* @brief Clear the queue.
	*/
	void Empty();
	/**
	* @brief Get the element count from the queue.
	* @return Element count.
	*/
	size_t GetDataQty() const { return m_iDataQty; }
	/**
	* @brief Get the internal data pointer.
	* @return Pointer to internal data.
	*/
	const BYTE * RemoveDataLock() const;
	/**
	* @brief Remove an amount of data from the queue.
	*
	* If amount is greater than current element count, all will be removed.
	* @param iSize amount of data to remove.
	*/
	void RemoveDataAmount( size_t iSize );

	/**
	* @brief Add an amount of data, resizing the buffer if needed.
	* @param pData pointer to the data to add.
	* @param iLen amount of data to add.
	*/
	void AddNewData( const BYTE * pData, size_t iLen );
	/**
	* @brief Get the position to add an amount of data, resizing the buffer if needed.
	* @param iLen length needed.
	* @return pointer to the position where the data can be added.
	*/
	BYTE * AddNewDataLock( size_t iLen );
	/**
	* @brief Increment the data counter.
	* @param iLen increment to add.
	*/
	void AddNewDataFinish( size_t iLen );
};

#endif
