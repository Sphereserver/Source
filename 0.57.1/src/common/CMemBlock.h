#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H
#pragma once

struct CMemoryBlock
{
public:
	CMemoryBlock()
	{
		m_dwLength = 0;
		m_pData = NULL;
	}
	~CMemoryBlock()
	{
		Free();
	}

	void Free()
	{
		m_dwLength = 0;
		if ( m_pData != NULL )
		{
			delete [] m_pData;
			m_pData = NULL;
		}
	}

	void Alloc(DWORD dwSize)
	{
		Free();
		m_dwLength = dwSize;
		if ( dwSize )
		{
			m_pData = AllocBase(dwSize);
		}
	}

	BYTE *GetData() const
	{
		return m_pData;
	}

	DWORD GetDataLength() const
	{
		return m_dwLength;
	}

	void Resize(DWORD dwSizeNew)
	{
		BYTE *pDataNew = AllocBase(dwSizeNew);
		if ( m_pData )
		{
			// move any existing data.
			memcpy(pDataNew, m_pData, min(dwSizeNew, m_dwLength));
			Free();
		}
		m_dwLength = dwSizeNew;
		m_pData = pDataNew;
	}

protected:
	BYTE *AllocBase(DWORD dwSize)
	{
		BYTE *pData = new BYTE[dwSize];
		return pData;
	}

private:
	BYTE	*m_pData;
	DWORD	m_dwLength;
};

#endif
