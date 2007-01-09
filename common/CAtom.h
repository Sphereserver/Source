//
// CAtom.h
//

#ifndef _INC_CATOM_H
#define _INC_CATOM_H

#include "CArray.h"

class CAtomDef : public CGString
{
	friend class CAtomRef;
private:
	int m_iUseCount;
public:
	CAtomDef( LPCTSTR pszStr ) :
		CGString( pszStr )
	{
		m_iUseCount = 1;	
	}
};

class CAtomRef
{
private:
	CAtomDef * m_pDef;
private:
	void ClearRef();
public:
	const TCHAR * GetStr() const
	{
		if ( m_pDef == NULL )
			return( NULL );
		return( *m_pDef );
	}
	void SetStr( LPCTSTR pszText );

	void Copy( const CAtomRef & atom );
	CAtomRef & operator = ( const CAtomRef & atom )
	{
		// Copy operator is quick.
		Copy( atom );
		return( *this );
	}

	~CAtomRef()
	{
		ClearRef();
	}
	CAtomRef()
	{
		m_pDef = NULL;
	}
	CAtomRef( LPCTSTR pszName )
	{
		m_pDef = NULL;
		SetStr(pszName);
	}
};

#endif // _INC_CATOM_H

