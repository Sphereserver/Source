#include "graycom.h"
#include "CArray.h"

//***************************************************************************
// -CGObList

void CGObList::DeleteAll()
{
	Lock();
	// delete all entries.
	CGObListRec *pRec;
	while ( pRec = GetHead() )
	{
		delete pRec;
	}
	m_iCount = 0;
	m_pHead = NULL;
	m_pTail = NULL;
	Release();
}

void CGObList::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	// just remove from list. DON'T delete !
	if ( !pObRec )
		return;

	Lock();
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
	Release();
}

void CGObList::InsertAfter( CGObListRec * pNewRec, CGObListRec * pPrev )
{
	// Add after pPrev.
	// pPrev = NULL == add to the start.
	pNewRec->RemoveSelf();	// Get out of previous list first.

	Lock();
	pNewRec->m_pParent = this;

	CGObListRec * pNext;
	if ( pPrev != NULL )		// Its the first.
	{
		pNext = pPrev->GetNext();
		pPrev->m_pNext = pNewRec;
	}
	else
	{
		pNext = GetHead();
		m_pHead = pNewRec;
	}

	pNewRec->m_pPrev = pPrev;
	if ( pNext )
		pNext->m_pPrev = pNewRec;
	else
		m_pTail = pNewRec;

	pNewRec->m_pNext = pNext;
	m_iCount ++;
	Release();
}

CGObListRec * CGObList::GetAt( int index ) const
{
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
	// only called by pObRec->RemoveSelf()
	OnRemoveOb(pObRec);	// call any approriate virtuals.
}

void CGObList::InsertHead( CGObListRec * pNewRec )
{
	InsertAfter(pNewRec, NULL);
}

void CGObList::InsertTail( CGObListRec * pNewRec )
{
	InsertAfter(pNewRec, GetTail());
}

void CGObList::Empty()
{
	DeleteAll();
}

CGObListRec * CGObList::GetHead() const
{
	return m_pHead;
}

CGObListRec * CGObList::GetTail() const
{
	return m_pTail;
}

int CGObList::GetCount() const
{
	return m_iCount;
}

bool CGObList::IsEmpty() const
{
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
