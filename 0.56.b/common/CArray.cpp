//
// CArray.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graycom.h"
#include "CArray.h"

//***************************************************************************
// -CGObList

void CGObList::DeleteAll()
{
	ADDTOCALLSTACK("CGObList::DeleteAll");
	// delete all entries.
	for (;;)	// iterate the list.
	{
		CGObListRec * pRec = GetHead();
		if ( pRec == NULL )
			break;
		ASSERT( pRec->GetParent() == this );
		delete pRec;
	}
	m_iCount = 0;
	m_pHead = NULL;
	m_pTail = NULL;
}

void CGObList::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CGObList::OnRemoveOb");
	// just remove from list. DON'T delete !
	if ( pObRec == NULL )
		return;
	ASSERT( pObRec->GetParent() == this );

	CGObListRec * pNext = pObRec->GetNext();
	CGObListRec * pPrev = pObRec->GetPrev();

	if ( pNext != NULL )
		pNext->m_pPrev = pPrev;
	else
		m_pTail = pPrev;
	if ( pPrev != NULL )
		pPrev->m_pNext = pNext;
	else
		m_pHead = pNext;

	pObRec->m_pNext = NULL;	// this should not really be necessary.
	pObRec->m_pPrev = NULL;
	pObRec->m_pParent = NULL;	// We are now unlinked.
	m_iCount --;
}

void CGObList::InsertAfter( CGObListRec * pNewRec, CGObListRec * pPrev )
{
	ADDTOCALLSTACK("CGObList::InsertAfter");
	// Add after pPrev.
	// pPrev = NULL == add to the start.
	ASSERT( pNewRec != NULL );
	pNewRec->RemoveSelf();	// Get out of previous list first.
	ASSERT( pPrev != pNewRec );
	ASSERT( pNewRec->GetParent() == NULL );

	pNewRec->m_pParent = this;

	CGObListRec * pNext;
	if ( pPrev != NULL )		// Its the first.
	{
		ASSERT( pPrev->GetParent() == this );
		pNext = pPrev->GetNext();
		pPrev->m_pNext = pNewRec;
	}
	else
	{
		pNext = GetHead();
		m_pHead = pNewRec;
	}

	pNewRec->m_pPrev = pPrev;

	if ( pNext != NULL )
	{
		ASSERT( pNext->GetParent() == this );
		pNext->m_pPrev = pNewRec;
	}
	else
	{
		m_pTail = pNewRec;
	}

	pNewRec->m_pNext = pNext;
	m_iCount ++;
}

CGObListRec * CGObList::GetAt( int index ) const
{
	ADDTOCALLSTACK("CGObList::GetAt");
	CGObListRec * pRec = GetHead();
	while ( index-- > 0 && pRec != NULL )
	{
		pRec = pRec->GetNext();
	}
	return pRec;
}

//***************************************************************************
// -CObListRec

CGObListRec::CGObListRec()
{
	m_pParent = NULL;
	m_pNext = m_pPrev = NULL;
}

CGObListRec::~CGObListRec()
{
	RemoveSelf();
}

//***************************************************************************
// -CObList

void CGObList::RemoveAtSpecial( CGObListRec * pObRec )
{
	ADDTOCALLSTACK("CGObList::RemoveAtSpecial");
	// only called by pObRec->RemoveSelf()
	OnRemoveOb(pObRec);	// call any approriate virtuals.
}

void CGObList::InsertHead( CGObListRec * pNewRec )
{
	ADDTOCALLSTACK("CGObList::InsertHead");
	InsertAfter(pNewRec, NULL);
}

void CGObList::InsertTail( CGObListRec * pNewRec )
{
	ADDTOCALLSTACK("CGObList::InsertTail");
	InsertAfter(pNewRec, GetTail());
}

void CGObList::Empty()
{
	ADDTOCALLSTACK("CGObList::Empty");
	DeleteAll();
}

CGObListRec * CGObList::GetHead() const
{
	ADDTOCALLSTACK("CGObList::GetHead");
	return m_pHead;
}

CGObListRec * CGObList::GetTail() const
{
	ADDTOCALLSTACK("CGObList::GetTail");
	return m_pTail;
}

int CGObList::GetCount() const
{
	ADDTOCALLSTACK("CGObList::GetCount");
	return m_iCount;
}

bool CGObList::IsEmpty() const
{
	ADDTOCALLSTACK("CGObList::IsEmpty");
	return !GetCount();
}

CGObList::CGObList()
{
	m_pHead = m_pTail = NULL;
	m_iCount = 0;
}

CGObList::~CGObList()
{
	DeleteAll();
}
