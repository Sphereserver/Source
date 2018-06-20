#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H
#pragma once

struct CMemBlock
{
public:
	CMemBlock()
	{
		m_pbData = NULL;
	}
	virtual ~CMemBlock()
	{
		Free();
	}

private:
	BYTE *m_pbData;	// the actual data bytes of the bitmap

protected:
	/**
	* @brief Sets the internal data pointer. Fails when internal data pointer is not NULL.
	*/
	void MemLink(BYTE *pbData)
	{
		ASSERT(!m_pbData);
		m_pbData = pbData;
	}

	/**
	* @brief Alloc mem (new BYTE[*] wrapper). Fails if can not alloc or if size is invalid.
	* @param uSize size to alloc.
	* @return pointer to the allocated data.
	*/
	BYTE *AllocBase(size_t uSize)
	{
		ASSERT(uSize > 0);
		BYTE *pbData = new BYTE[uSize];
		ASSERT(pbData);
		return pbData;
	}

public:
	/**
	* @brief Clear internal data pointer and, if size is valid, alloc mem, updating internal data pointer.
	* @param uSize size to alloc.
	*/
	void Alloc(size_t uSize)
	{
		Free();
		if ( uSize > 0 )
			m_pbData = AllocBase(uSize);
	}

	/**
	* @brief Clear internal data pointer if it is not NULL.
	*/
	void Free()
	{
		if ( m_pbData )
		{
			delete[] m_pbData;
			m_pbData = NULL;
		}
	}

	/**
	* @brief Gets the internal data pointer.
	* @return The internal data pointer (can be NULL).
	*/
	BYTE *GetData() const
	{
		return m_pbData;
	}

private:
	CMemBlock(const CMemBlock &copy);
	CMemBlock &operator=(const CMemBlock &other);
};

struct CMemLenBlock : public CMemBlock
{
public:
	CMemLenBlock()
	{
		m_uLength = 0;
	}

private:
	size_t m_uLength;	// buffer length

public:
	/**
	* @brief Set the size of the buffer and alloc mem.
	* @param uSize new size of the buffer.
	*/
	void Alloc(size_t uSize)
	{
		m_uLength = uSize;
		CMemBlock::Alloc(uSize);
	}

	/**
	* @brief Clears the buffer.
	*/
	void Free()
	{
		m_uLength = 0;
		CMemBlock::Free();
	}

	/**
	* @brief Get the buffer length.
	* @return Length of the buffer.
	*/
	size_t GetDataLength() const
	{
		return m_uLength;
	}

	/**
	* @brief Resizes the buffer, maintaining the current data.
	* @param uSizeNew new size of the buffer.
	*/
	void Resize(size_t uSizeNew)
	{
		ASSERT(uSizeNew != m_uLength);
		BYTE *pbDataNew = AllocBase(uSizeNew);
		ASSERT(pbDataNew);
		if ( GetData() )
		{
			// Move any existing data
			ASSERT(m_uLength);
			memcpy(pbDataNew, GetData(), minimum(uSizeNew, m_uLength));
			CMemBlock::Free();
		}
		else
			ASSERT(!m_uLength);

		m_uLength = uSizeNew;
		MemLink(pbDataNew);
	}

private:
	CMemLenBlock(const CMemLenBlock &copy);
	CMemLenBlock &operator=(const CMemLenBlock &other);
};

#endif	// _INC_CMEMBLOCK_H
