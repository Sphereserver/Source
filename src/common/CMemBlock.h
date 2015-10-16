#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H
#pragma once

#ifndef minimum					// limits.h ?
	#define minimum(x,y)	((x)<(y)?(x):(y))
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif	// minimum
#define medium(x,y,z)	((x)>(y)?(x):((z)<(y)?(z):(y)))

struct CMemBlock
{
private:
	BYTE * m_pData;	// the actual data bytes of the bitmap.

protected:
	// rather dangerous functions.
	void MemLink( BYTE * pData )
	{
		ASSERT( m_pData == NULL );
		m_pData = pData;
	}
	BYTE * AllocBase( size_t dwSize )
	{
		ASSERT(dwSize > 0);
		BYTE * pData = new BYTE[ dwSize ];
		ASSERT( pData != NULL );
		return( pData );
	}

public:
	void Alloc( size_t dwSize )
	{
		Free();
		if ( dwSize > 0 )
		{
			m_pData = AllocBase(dwSize);
		}
	}

	void Free()
	{
		if ( m_pData != NULL )
		{
			delete[] m_pData;
			m_pData = NULL;
		}
	}
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
	CMemBlock(const CMemBlock& copy);
	CMemBlock& operator=(const CMemBlock& other);
};

struct CMemLenBlock : public CMemBlock
{
private:
	size_t m_dwLength;

public:
	CMemLenBlock()
	{
		m_dwLength = 0;
	}

private:
	CMemLenBlock(const CMemLenBlock& copy);
	CMemLenBlock& operator=(const CMemLenBlock& other);

public:
	size_t GetDataLength() const
	{
		return( m_dwLength );
	}
	void Alloc( size_t dwSize )
	{
		m_dwLength = dwSize;
		CMemBlock::Alloc(dwSize);
	}
	void Free()
	{
		m_dwLength = 0;
		CMemBlock::Free();
	}
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

