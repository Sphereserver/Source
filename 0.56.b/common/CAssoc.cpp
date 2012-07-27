//
// CAssoc.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graycom.h"
#include "CAssoc.h"

//***************************************************************************
// -CValStr

LPCTSTR CValStr::FindName( int iVal ) const
{
	ADDTOCALLSTACK("CValStr::FindName");
	size_t i = 0;
	ASSERT(this[i].m_pszName != NULL);
	for ( ; this[i].m_pszName; i++ )
	{
		if ( iVal < this[i + 1].m_iVal )
			return( this[i].m_pszName );
	}
	return( this[i - 1].m_pszName );
}

//***************************************************************************
// -CElementDef
// Describe the elements of a structure/class

const int CElementDef::sm_Lengths[ELEM_QTY] =
{
	0,	// ELEM_VOID:
	-1,	// ELEM_CSTRING,
	-1, // ELEM_STRING,	// Assume max size of REG_SIZE
	sizeof(bool),	// ELEM_BOOL
	sizeof(BYTE), // ELEM_BYTE,		// 1 byte.
	sizeof(BYTE), // ELEM_MASK_BYTE,	// bits in a BYTE
	sizeof(WORD), // ELEM_WORD,		// 2 bytes
	sizeof(WORD), // ELEM_MASK_WORD,	// bits in a WORD
	sizeof(int),  // ELEM_INT,		// Whatever the int size is. 4 i assume
	sizeof(int),  // ELEM_MASK_INT,
	sizeof(DWORD), // ELEM_DWORD,		// 4 bytes.
	sizeof(DWORD), // ELEM_MASK_DWORD,	// bits in a DWORD
};

bool CElementDef::SetValStr( void * pBase, LPCTSTR pszVal ) const
{
	ADDTOCALLSTACK("CElementDef::SetValStr");
	// Set the element value as a string.
	DWORD dwVal = 0;
	ASSERT(m_offset>=0);
	void * pValPtr = GetValPtr(pBase);
	switch ( m_type )
	{
		case ELEM_VOID:
			return false;
		case ELEM_STRING:
			strcpylen(static_cast<TCHAR *>(pValPtr), pszVal, GetValLength() - 1);
			return( true );
		case ELEM_CSTRING:
			*static_cast<CGString *>(pValPtr) = pszVal;
			return true;
		case ELEM_BOOL:
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_INT: // signed ?
		case ELEM_DWORD:
			dwVal = Exp_GetVal( pszVal );
			memcpy( pValPtr, &dwVal, GetValLength());
			return true;
		case ELEM_MASK_BYTE:	// bits in a BYTE
		case ELEM_MASK_WORD:	// bits in a WORD
		case ELEM_MASK_INT:
		case ELEM_MASK_DWORD:	// bits in a DWORD
			return false;
		default:
			break;
	}
	return false;
}

bool CElementDef::GetValStr( const void * pBase, CGString & sVal ) const
{
	ADDTOCALLSTACK("CElementDef::GetValStr");
	// Get the element value as a string.

	DWORD dwVal = 0;
	ASSERT(m_offset>=0);
	void * pValPtr = GetValPtr(pBase);
	switch ( m_type )
	{
		case ELEM_VOID:
			return false;
		case ELEM_STRING:
			sVal = static_cast<TCHAR *>(pValPtr);
			return( true );
		case ELEM_CSTRING:
			sVal = *static_cast<CGString *>(pValPtr);
			return true;
		case ELEM_BOOL:
		case ELEM_BYTE:
		case ELEM_WORD:
		case ELEM_INT: // signed ?
		case ELEM_DWORD:
			memcpy( &dwVal, pValPtr, GetValLength());
			sVal.Format("%lu", dwVal);
			return true;
		case ELEM_MASK_BYTE:	// bits in a BYTE
		case ELEM_MASK_WORD:	// bits in a WORD
		case ELEM_MASK_INT:
		case ELEM_MASK_DWORD:	// bits in a DWORD
			return false;
		default:
			break;
	}
	return false;
}
