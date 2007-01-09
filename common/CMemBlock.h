//
// CMemBlock.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H
#pragma once

#ifndef minimum					// limits.h ?
	#define minimum(x,y)	((x)<(y)?(x):(y))
	#define maximum(x,y)	((x)>(y)?(x):(y))
#endif	// minimum

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
	BYTE * AllocBase( DWORD dwSize )
	{
		ASSERT(dwSize);
		BYTE * pData = new BYTE [ dwSize ];
		ASSERT( pData );
		return( pData );
	}
public:
	void Alloc( DWORD dwSize )
	{
		Free();
		if ( dwSize )
		{
			m_pData = AllocBase(dwSize);
		}
	}

	void Free()
	{
		if ( m_pData != NULL )
		{
			delete [] m_pData;
			m_pData = NULL;
		}
	}
	BYTE * GetData() const
	{
		return( m_pData );
	}
	CMemBlock()
	{
		m_pData = NULL;
	}
	~CMemBlock()
	{
		Free();
	}
};

struct CMemLenBlock : public CMemBlock
{
private:
	DWORD m_dwLength;
public:
	CMemLenBlock()
	{
		m_dwLength = 0;
	}
	DWORD GetDataLength() const
	{
		return( m_dwLength );
	}
	void Alloc( DWORD dwSize )
	{
		m_dwLength = dwSize;
		CMemBlock::Alloc(dwSize);
	}
	void Free()
	{
		m_dwLength = 0;
		CMemBlock::Free();
	}
	void Resize( DWORD dwSizeNew )
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

