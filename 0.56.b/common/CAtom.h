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
	explicit CAtomDef( LPCTSTR pszStr ) :
		CGString( pszStr )
	{
		m_iUseCount = 1;	
	}
private:
	CAtomDef(const CAtomDef& copy);
	CAtomDef& operator=(const CAtomDef& other);
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

	CAtomRef()
	{
		m_pDef = NULL;
	}
	explicit CAtomRef( LPCTSTR pszName )
	{
		m_pDef = NULL;
		SetStr(pszName);
	}
	~CAtomRef()
	{
		ClearRef();
	}
	CAtomRef & operator = ( const CAtomRef & atom )
	{
		// Copy operator is quick.
		Copy( atom );
		return( *this );
	}
private:
	CAtomRef(const CAtomRef& copy);
};

#endif // _INC_CATOM_H

