/**
* @file CMemBlock.h
*/

#pragma once
#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H

#ifndef minimum					// limits.h ?
	#define minimum(x,y)	((x)<(y)?(x):(y))
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif	// minimum
#define medium(x,y,z)	((x)>(y)?(x):((z)<(y)?(z):(y)))

struct CMemBlock
{
private:
	BYTE * m_pData;	 ///< the actual data bytes of the bitmap.

protected:
	// rather dangerous functions.
	/**
	* @brief Sets the internal data pointer. Fails when internal data pointer is not NULL.
	*/
	void MemLink( BYTE * pData )
	{
		ASSERT( m_pData == NULL );
		m_pData = pData;
	}
	/**
	* @brief Alloc mem (new BYTE[*] wrapper). Fails if can not alloc or if size is invalid.
	* @param dwSize size to alloc.
	* @return pointer to the allocated data.
	*/
	BYTE * AllocBase( size_t dwSize )
	{
		ASSERT(dwSize > 0);
		BYTE * pData = new BYTE[ dwSize ];
		ASSERT( pData != NULL );
		return( pData );
	}

public:
	/**
	* @brief Clear internal data pointer and, if size is valid, alloc mem, updating internal data pointer.
	* @param dwSize size to alloc.
	*/
	void Alloc( size_t dwSize )
	{
		Free();
		if ( dwSize > 0 )
		{
			m_pData = AllocBase(dwSize);
		}
	}

	/**
	* @brief Clear internal data pointer if it is not NULL.
	*/
	void Free()
	{
		if ( m_pData != NULL )
		{
			delete[] m_pData;
			m_pData = NULL;
		}
	}
	/**
	* @brief Gets the internal data pointer.
	* @return The internal data pointer (can be NULL).
	*/
	BYTE * GetData() const
	{
		return( m_pData );
	}

public:
	CMemBlock()
	{
		m_pData = NULL;
	}
	virtual ~CMemBlock()
	{
		Free();
	}

private:
	/**
	* @brief No copy on construction allowed.
    */
	CMemBlock(const CMemBlock& copy);
	/**
	* @brief No copy allowed.
    */
	CMemBlock& operator=(const CMemBlock& other);
};

/**
* @brief Buffer implementation.
*/
struct CMemLenBlock : public CMemBlock
{
private:
	size_t m_dwLength;  ///< Buffer len.

public:
	CMemLenBlock()
	{
		m_dwLength = 0;
	}

private:
	/**
	* @brief No copy on construction allowed.
    */
	CMemLenBlock(const CMemLenBlock& copy);
	/**
	* @brief No copy allowed.
    */
	CMemLenBlock& operator=(const CMemLenBlock& other);

public:
	/**
	* @brief Get the buffer len.
	+ @return Length of the buffer.
    */
	size_t GetDataLength() const
	{
		return( m_dwLength );
	}
	/**
	* @brief Set the size of the buffer and alloc mem.
	+ @param dwSize new size of the buffer.
    */
	void Alloc( size_t dwSize )
	{
		m_dwLength = dwSize;
		CMemBlock::Alloc(dwSize);
	}
	/**
	* @brief Clears the buffer.
    */
	void Free()
	{
		m_dwLength = 0;
		CMemBlock::Free();
	}
	/**
	* @brief Resizes the buffer, maintaining the current data.
	+ @param dwSizeNew new size of the buffer.
    */
	void Resize( size_t dwSizeNew )
	{
		ASSERT( dwSizeNew != m_dwLength );
		BYTE * pDataNew = AllocBase( dwSizeNew );
		ASSERT(pDataNew);
		if ( GetData())
		{
			// move any existing data.
			ASSERT(m_dwLength);
			memcpy( pDataNew, GetData(), minimum( dwSizeNew, m_dwLength ));
			CMemBlock::Free();
		}
		else
		{
			ASSERT(!m_dwLength);
		}
		m_dwLength = dwSizeNew;
		MemLink( pDataNew );
	}
};


#endif	// _INC_CMEMBLOCK_H
