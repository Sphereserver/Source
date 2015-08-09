//
// CAtom.cpp
//

#include "graycom.h"
#include "CAtom.h"

struct CAtomManager : public CGObSortArray < CAtomDef *, LPCTSTR >
{
public:
	// Create an alpha sorted string lookup table.
	int CompareKey( LPCTSTR pszKey, CAtomDef * pVal, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT(pszKey);
		ASSERT(pVal);
		return( strcmp( pszKey, * ( static_cast <CGString*>( pVal ))));
	}

public:
	CAtomManager() { };
private:
	CAtomManager(const CAtomManager& copy);
	CAtomManager& operator=(const CAtomManager& other);
};

static CAtomManager g_AtomManager;

//*********************************
// CAtomRef

void CAtomRef::ClearRef()
{
	ADDTOCALLSTACK("CAtomRef::ClearRef");
	if ( m_pDef )
	{
		ASSERT(m_pDef->m_iUseCount);
		if ( ! --m_pDef->m_iUseCount )
		{
			g_AtomManager.DeleteOb(m_pDef);
		}
		m_pDef = NULL;
	}
}

void CAtomRef::Copy( const CAtomRef & atom )
{
	ADDTOCALLSTACK("CAtomRef::Copy");
	// Copy's are fast.
	if ( m_pDef == atom.m_pDef )
		return;
	ClearRef();
	m_pDef = atom.m_pDef;
	m_pDef->m_iUseCount++;
}

void CAtomRef::SetStr( LPCTSTR pszText )
{
	ADDTOCALLSTACK("CAtomRef::SetStr");
	ClearRef();
	if ( pszText == NULL )
		return;

	// Find it in the atom table first.
	int iCompareRes;
	size_t index = g_AtomManager.FindKeyNear( pszText, iCompareRes );
	if ( iCompareRes == 0 )
	{
		// already here just increment useage.
		m_pDef = g_AtomManager.GetAt( index );
		m_pDef->m_iUseCount++;
	}
	else
	{
		// Insertion sort.
		m_pDef = new CAtomDef( pszText );
		g_AtomManager.AddPresorted( index, iCompareRes, m_pDef );
	}
}

