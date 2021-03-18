#include "graycom.h"
#include "CArray.h"

///////////////////////////////////////////////////////////
// CGObListRec

CGObListRec::CGObListRec()
{
	m_pParent = NULL;
	m_pNext = NULL;
	m_pPrev = NULL;
}

CGObListRec::~CGObListRec()
{
	RemoveSelf();
}

///////////////////////////////////////////////////////////
// CGObList

CGObList::CGObList()
{
	m_pHead = NULL;
	m_pTail = NULL;
	m_iCount = 0;
}

CGObList::~CGObList()
{
	DeleteAll();
}

CGObListRec *CGObList::GetAt(size_t i) const
{
	ADDTOCALLSTACK("CGObList::GetAt");
	CGObListRec *pRec = GetHead();
	while ( (i-- > 0) && pRec )
	{
		pRec = pRec->GetNext();
	}
	return pRec;
}

CGObListRec *CGObList::GetHead() const
{
	return m_pHead;
}

CGObListRec *CGObList::GetTail() const
{
	return m_pTail;
}

size_t CGObList::GetCount() const
{
	return m_iCount;
}

bool CGObList::IsEmpty() const
{
	return !GetCount();
}

void CGObList::InsertAfter(CGObListRec *pNewRec, CGObListRec *pPrev)
{
	ADDTOCALLSTACK("CGObList::InsertAfter");
	// Insert after pPrev
	ASSERT(pNewRec);
	pNewRec->RemoveSelf();		// get out of previous list first
	ASSERT(pPrev != pNewRec);
	ASSERT(!pNewRec->GetParent());

	pNewRec->m_pParent = this;

	CGObListRec *pNext;
	if ( pPrev )
	{
		ASSERT(pPrev->GetParent() == this);
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
	{
		ASSERT(pNext->GetParent() == this);
		pNext->m_pPrev = pNewRec;
	}
	else
		m_pTail = pNewRec;

	pNewRec->m_pNext = pNext;
	++m_iCount;
}

void CGObList::InsertHead(CGObListRec *pNewRec)
{
	ADDTOCALLSTACK("CGObList::InsertHead");
	InsertAfter(pNewRec, NULL);
}

void CGObList::InsertTail(CGObListRec *pNewRec)
{
	ADDTOCALLSTACK("CGObList::InsertTail");
	InsertAfter(pNewRec, GetTail());
}

void CGObList::DeleteAll()
{
	ADDTOCALLSTACK("CGObList::DeleteAll");
	// Delete all entries
	for (;;)
	{
		CGObListRec *pRec = GetHead();
		if ( !pRec )
			break;

		ASSERT(pRec->GetParent() == this);
		delete pRec;
	}
	m_pHead = NULL;
	m_pTail = NULL;
	m_iCount = 0;
}

void CGObList::RemoveAtSpecial(CGObListRec *pObRec)
{
	ADDTOCALLSTACK("CGObList::RemoveAtSpecial");
	// Only called by CGObListRec::RemoveSelf()
	OnRemoveOb(pObRec);		// call any appropriate virtuals
}

void CGObList::OnRemoveOb(CGObListRec *pObRec)
{
	ADDTOCALLSTACK("CGObList::OnRemoveOb");
	// Just remove from list (don't delete)
	if ( !pObRec )
		return;

	ASSERT(pObRec->GetParent() == this);
	CGObListRec *pNext = pObRec->GetNext();
	CGObListRec *pPrev = pObRec->GetPrev();

	if ( pNext )
		pNext->m_pPrev = pPrev;
	else
		m_pTail = pPrev;

	if ( pPrev )
		pPrev->m_pNext = pNext;
	else
		m_pHead = pNext;

	pObRec->m_pParent = NULL;
	pObRec->m_pNext = NULL;
	pObRec->m_pPrev = NULL;
	--m_iCount;
}
