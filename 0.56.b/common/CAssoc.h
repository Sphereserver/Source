//
// CAssoc.H
// Simple shared usefull base classes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _INC_CASSOC_H
#define _INC_CASSOC_H
#pragma once

#include "CFile.h"
#include "CArray.h"

////////////////////////////////////////////////////////////////////////

#ifndef PT_REG_STRMAX
#define PT_REG_STRMAX		128
#endif
#ifndef PT_REG_ROOTKEY
#define PT_REG_ROOTKEY		HKEY_LOCAL_MACHINE
#endif

////////////////////////////////////////////////////////////////////////

// #include <shellapi.h>

enum ELEM_TYPE	// define types of structure/record elements.
{
	ELEM_VOID = 0,	// unknown what this might be. (or just 'other') (must be handled manually)
	ELEM_CSTRING,	// Size prefix.
	ELEM_STRING,	// Assume max size of REG_SIZE. NULL TERM string.
	ELEM_BOOL,		// bool = just 1 byte i guess.
	ELEM_BYTE,		// 1 byte.
	ELEM_MASK_BYTE,	// bits in a BYTE
	ELEM_WORD,		// 2 bytes
	ELEM_MASK_WORD,	// bits in a WORD
	ELEM_INT,		// Whatever the int size is. 4 i assume
	ELEM_MASK_INT,
	ELEM_DWORD,		// 4 bytes.
	ELEM_MASK_DWORD,	// bits in a DWORD

	ELEM_QTY,
};

#ifndef OFFSETOF			// stddef.h ?
	#define OFFSETOF(s,m)   	(int)( (BYTE *)&(((s *)0)->m) - (BYTE *)0 )
#endif

struct CElementDef
{
	static const int sm_Lengths[ELEM_QTY];
	ELEM_TYPE m_type;
	UINT	m_offset;	// The offset into the class instance for this item.
	// ELEM_STRING = max size.
	// ELEM_MASK_WORD etc. = Extra masking info if needed. 
	DWORD   m_extra;

public:
	// get structure value.
	void * GetValPtr( const void * pBaseInst ) const
	{
		return( ((BYTE *)pBaseInst) + m_offset );
	}
	int GetValLength() const
	{
		ASSERT(m_type<ELEM_QTY);
		if ( m_type == ELEM_STRING )
		{
			return(m_extra);
		}
		return( sm_Lengths[m_type] );
	}

	bool GetValStr( const void * pBase, CGString & sVal ) const;
	bool SetValStr( void * pBase, LPCTSTR pszVal ) const;
};

class CAssocReg	// associate members of some class/structure with entries in the registry.
{
	// LAST = { NULL, 0, ELEM_VOID }
public:
	LPCTSTR m_pszKey;	// A single key identifier to be cat to a base key. NULL=last
	CElementDef m_elem;
public:
	static const char *m_sClassName;
	operator LPCTSTR() const
	{
		return( m_pszKey );
	}
	// get structure value.
	void * GetValPtr( const void * pBaseInst ) const
	{
		return( m_elem.GetValPtr( pBaseInst ));
	}
};

////////////////////////////////////////////////////////////////////////

class CGStringListRec : public CGObListRec, public CGString
{
	friend class CGStringList;
public:
	static const char *m_sClassName;
	CGStringListRec * GetNext() const
	{
		return( (CGStringListRec *) CGObListRec :: GetNext());
	}
	CGStringListRec( LPCTSTR pszVal ) : CGString( pszVal )
	{
	}
};

class CGStringList : public CGObList 	// obviously a list of strings.
{
public:
	static const char *m_sClassName;
	CGStringListRec * GetHead() const
	{
		return( (CGStringListRec *) CGObList::GetHead() );
	}
	void AddHead( LPCTSTR pszVal )
	{
		InsertHead( new CGStringListRec( pszVal ));
	}
	void AddTail( LPCTSTR pszVal )
	{
		InsertTail( new CGStringListRec( pszVal ));
	}
};

#endif // _INC_CASSOC_H

