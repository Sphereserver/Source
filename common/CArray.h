//
// CArray.h
// Copyright Menace Software (www.menasoft.com).
//
// c++ Collections.

#ifndef _INC_CARRAY_H
#define _INC_CARRAY_H

#ifndef _WIN32
	#define STANDARD_CPLUSPLUS_THIS(_x_) this->_x_
#else
	#define STANDARD_CPLUSPLUS_THIS(_x_) _x_
#endif

///////////////////////////////////////////////////////////
// CGObList

class CGObList;

class CGObListRec	// generic list record. 
{
	// This item belongs to JUST ONE LIST
	friend class CGObList;

	private:
		CGObList  * 	m_pParent;		// link me back to my parent object.
		CGObListRec * 	m_pNext;
		CGObListRec * 	m_pPrev;
	public:
		static const char *m_sClassName;
		CGObList  * 	GetParent() const { return m_pParent; }
		CGObListRec * 	GetNext() const { return m_pNext; }
		CGObListRec * 	GetPrev() const { return m_pPrev; }
	public:
		CGObListRec();
		inline void RemoveSelf();
		virtual ~CGObListRec();
};

class CGObList	// generic list of objects based on CGObListRec.
{
	friend class CGObListRec;

	private:
		CGObListRec * m_pHead;
		CGObListRec * m_pTail;	// Do we really care about tail ? (as it applies to lists anyhow)
		int m_iCount;
	private:
		void RemoveAtSpecial( CGObListRec * pObRec );
	protected:
		// Override this to get called when an item is removed from this list.
		// Never called directly. call pObRec->RemoveSelf()
		virtual void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
	public:
		static const char *m_sClassName;
		CGObListRec * GetAt( int index ) const;
		// pPrev = NULL = first
		virtual void InsertAfter( CGObListRec * pNewRec, CGObListRec * pPrev = NULL );
		void InsertHead( CGObListRec * pNewRec );
		void InsertTail( CGObListRec * pNewRec );
		void DeleteAll();
		void Empty();
		CGObListRec * GetHead() const;
		CGObListRec * GetTail() const;
		int GetCount() const;
		bool IsEmpty() const;
		CGObList();
		virtual ~CGObList();
};

///////////////////////////////////////////////////////////
// CGTypedArray<class TYPE, class ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CGTypedArray
{
	// NOTE: This will not call true constructors or destructors !
	private:
		TYPE* m_pData;			// the actual array of data
		int m_nCount;			// # of elements currently in the list
		int m_nRealCount;		//	real number of allocated elements

	public:
		static const char *m_sClassName;
		CGTypedArray();
		TYPE * GetBasePtr() const;	// This is dangerous to use of course.
		int GetCount() const;
		int GetRealCount() const;
		bool IsValidIndex( int i ) const;
		void SetCount( int nNewCount );
		void RemoveAll();
		void Empty();
		void SetAt( int nIndex, ARG_TYPE newElement );
		void SetAtGrow( int nIndex, ARG_TYPE newElement);
		void InsertAt( int nIndex, ARG_TYPE newElement );
		int Add( ARG_TYPE newElement );
		void RemoveAt( int nIndex );
		TYPE GetAt( int nIndex) const;
		TYPE operator[](int nIndex) const;
		TYPE& ElementAt( int nIndex );
		TYPE& operator[](int nIndex);
		const TYPE& ElementAt( int nIndex ) const;
		virtual void ConstructElements(TYPE* pElements, int nCount );
		virtual void DestructElements(TYPE* pElements, int nCount );
		void Copy( const CGTypedArray<TYPE, ARG_TYPE> * pArray );
		const CGTypedArray<TYPE, ARG_TYPE> & operator=( const CGTypedArray<TYPE, ARG_TYPE> & array );
		~CGTypedArray();
};

/////////////////////////////////////////////////////////////////////////////
// CGPtrTypeArray

template<class TYPE>
class CGPtrTypeArray : public CGTypedArray<TYPE, TYPE>	// void*
{
	protected:
		virtual void DestructElements( TYPE* pElements, int nCount );
	public:
		static const char *m_sClassName;
		int FindPtr( TYPE pData ) const;
		bool RemovePtr( TYPE pData );
		bool IsValidIndex( int i ) const;
};

/////////////////////////////////////////////////////////////////////////////
// CGObArray

template<class TYPE>
class CGObArray : public CGPtrTypeArray<TYPE>
{
	// The point of this type is that the array now OWNS the element.
	// It will get deleted when the array is deleted.
	protected:
		virtual void DestructElements( TYPE* pElements, int nCount );
	public:
		static const char *m_sClassName;
		void Clean(bool bElements = false);
		bool DeleteOb( TYPE pData );
		void DeleteAt( int nIndex );
		~CGObArray();
};

////////////////////////////////////////////////////////////
// CGObSortArray = A sorted array of objects.

template<class TYPE,class KEY_TYPE>
struct CGObSortArray : public CGObArray<TYPE>
{
	int FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces = false ) const;
	int FindKey( KEY_TYPE key ) const;
	int AddPresorted( int index, int iCompareRes, TYPE pNew );
	int AddSortKey( TYPE pNew, KEY_TYPE key );
	virtual int CompareKey( KEY_TYPE, TYPE, bool fNoSpaces ) const = 0;
	void DeleteKey( KEY_TYPE key );
};

///////////////////////////////////////////////////////////
// CGTypedArray<class TYPE, class ARG_TYPE>

template<class TYPE, class ARG_TYPE>
CGTypedArray<TYPE,ARG_TYPE>::CGTypedArray()
{
	m_pData = NULL;
	m_nCount = 0;
	m_nRealCount = 0;
}

template<class TYPE, class ARG_TYPE>
TYPE * CGTypedArray<TYPE,ARG_TYPE>::GetBasePtr() const	// This is dangerous to use of course.
{
	return m_pData;
}

template<class TYPE, class ARG_TYPE>
int CGTypedArray<TYPE,ARG_TYPE>::GetRealCount() const
{
	return m_nRealCount;
}


template<class TYPE, class ARG_TYPE>
int CGTypedArray<TYPE,ARG_TYPE>::GetCount() const
{
	return m_nCount;
}

template<class TYPE, class ARG_TYPE>
bool CGTypedArray<TYPE,ARG_TYPE>::IsValidIndex( int i ) const
{
	return ( i>=0 && i<m_nCount );
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::RemoveAll()
{
	SetCount(0);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::Empty()
{
	RemoveAll();
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::SetAt( int nIndex, ARG_TYPE newElement )
{
	ASSERT(IsValidIndex(nIndex));

	DestructElements(&m_pData[nIndex], 1);
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::SetAtGrow( int nIndex, ARG_TYPE newElement)
{
	ASSERT(nIndex >= 0);

	if ( nIndex >= m_nCount )
		SetCount(nIndex+1);
	SetAt(nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::InsertAt( int nIndex, ARG_TYPE newElement )
{	// Bump the existing entry here forward.
	ASSERT(nIndex >= 0);

	SetCount( (nIndex >= m_nCount) ? (nIndex+1) : (m_nCount+1) );
	memmove( &m_pData[nIndex+1], &m_pData[nIndex], sizeof(TYPE)*(m_nCount-nIndex-1));
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
int CGTypedArray<TYPE,ARG_TYPE>::Add( ARG_TYPE newElement )
{
	// Add to the end.
	SetAtGrow(GetCount(), newElement);
	return (m_nCount-1);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::RemoveAt( int nIndex )
{
	if ( !IsValidIndex(nIndex) )
		return;

	DestructElements(&m_pData[nIndex], 1);
	memmove(&m_pData[nIndex], &m_pData[nIndex+1], sizeof(TYPE)*(m_nCount-nIndex-1));
	SetCount(m_nCount-1);
}

template<class TYPE, class ARG_TYPE>
TYPE CGTypedArray<TYPE,ARG_TYPE>::GetAt( int nIndex) const
{
	return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
TYPE CGTypedArray<TYPE,ARG_TYPE>::operator[](int nIndex) const
{
	return GetAt(nIndex);
}

template<class TYPE, class ARG_TYPE>
TYPE& CGTypedArray<TYPE,ARG_TYPE>::ElementAt( int nIndex )
{
	ASSERT(IsValidIndex(nIndex));
	return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
TYPE& CGTypedArray<TYPE,ARG_TYPE>::operator[](int nIndex)
{
	return ElementAt(nIndex);
}

template<class TYPE, class ARG_TYPE>
const TYPE& CGTypedArray<TYPE,ARG_TYPE>::ElementAt( int nIndex ) const
{
	ASSERT(IsValidIndex(nIndex));
	return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::ConstructElements(TYPE* pElements, int nCount )
{
	// first do bit-wise zero initialization
	memset((void*)pElements, 0, nCount * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::DestructElements(TYPE* pElements, int nCount )
{
	//memset((void*)pElements, 0, nCount * sizeof(*pElements));
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE,ARG_TYPE>::Copy(const CGTypedArray<TYPE, ARG_TYPE> * pArray)
{
	if ( this == pArray ) // it was !=
		return;

	Empty();
	SetCount(pArray->GetCount());
	memcpy(GetBasePtr(), pArray->GetBasePtr(), GetCount() * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
const CGTypedArray<TYPE, ARG_TYPE> & CGTypedArray<TYPE, ARG_TYPE>::operator=( const CGTypedArray<TYPE, ARG_TYPE> & array )
{
	Copy(&array);
	return *this;
}

template<class TYPE, class ARG_TYPE>
CGTypedArray<TYPE,ARG_TYPE>::~CGTypedArray()
{
	SetCount(0);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::SetCount( int nNewCount )
{
	if ( nNewCount < 0 )
		return;

	if (nNewCount == 0)
	{
		// shrink to nothing
		if (m_nCount)
		{
			//DestructElements( m_pData, m_nCount );
			delete [] (BYTE*) m_pData;
			m_nCount = m_nRealCount = 0;	// that's probably wrong.. but SetCount(0) should be never called
			m_pData = NULL;					// before Clean(true) in CGObArray
		}
		return;
	}

	if ( nNewCount > m_nCount )
	{
		TYPE * pNewData = (TYPE *) new BYTE[ nNewCount * sizeof( TYPE ) ];
		if ( m_nCount )
		{
			// copy the old stuff to the new array.
			memcpy( pNewData, m_pData, sizeof(TYPE)*m_nCount );
			delete [] (BYTE*) m_pData;	// don't call any destructors.
		}

		// Just construct or init the new stuff.
		ConstructElements( pNewData + m_nCount, nNewCount - m_nCount );
		m_pData = pNewData;

		m_nRealCount = nNewCount;
	}

	m_nCount = nNewCount;

/*	if ( nNewCount < 0 )
		return;

	// shrink to nothing?
	if ( !nNewCount )
	{
		if ( m_nRealCount )
		{
			delete [] (BYTE*) m_pData;
			m_nCount = m_nRealCount = 0;
			m_pData = NULL;
		}
		return;
	}

	//	do we need to resize the array?
	if (( nNewCount > m_nRealCount ) || ( nNewCount < m_nRealCount-10 ))
	{
		m_nRealCount = nNewCount + 5;	// auto-allocate space for 5 extra elements
		TYPE	*pNewData = (TYPE*) new BYTE[m_nRealCount * sizeof(TYPE)];

		// i have already data inside, so move to the new place
		if ( m_nCount )
			memcpy(pNewData, m_pData, sizeof(TYPE)*m_nCount);
		delete [] (BYTE*) m_pData;
		m_pData = pNewData;
	}

	//	construct new element
	if ( nNewCount > m_nCount )
	{
		ConstructElements(m_pData + m_nCount, nNewCount - m_nCount);
	}

	m_nCount = nNewCount;
*/
}

/////////////////////////////////////////////////////////////////////////////
// CGPtrTypeArray

template<class TYPE>
void CGPtrTypeArray<TYPE>::DestructElements( TYPE* pElements, int nCount )
{
	memset(pElements, 0, nCount * sizeof(*pElements));
}

template<class TYPE>
int CGPtrTypeArray<TYPE>::FindPtr( TYPE pData ) const
{
	if ( !pData )
		return -1;

	for ( int nIndex=0; nIndex < STANDARD_CPLUSPLUS_THIS(GetCount()); nIndex++ )
	{
		if ( STANDARD_CPLUSPLUS_THIS(GetAt(nIndex)) == pData )
			return nIndex;
	}
	return -1;
}

template<class TYPE>
bool CGPtrTypeArray<TYPE>::RemovePtr( TYPE pData )
{
	int nIndex = FindPtr( pData );
	if ( nIndex < 0 )
		return false;
	
	STANDARD_CPLUSPLUS_THIS(RemoveAt(nIndex));
	return true;
}

template<class TYPE>
bool CGPtrTypeArray<TYPE>::IsValidIndex( int i ) const
{
	if (( i < 0 ) || ( i >= STANDARD_CPLUSPLUS_THIS(GetCount()) ))
		return false;
	return ( STANDARD_CPLUSPLUS_THIS(GetAt(i)) != NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CGObArray

template<class TYPE>
void CGObArray<TYPE>::DestructElements( TYPE* pElements, int nCount )
{
	// delete the objects that we own.
	for ( int i=0; i<nCount; i++ )
	{
		if ( pElements[i] != NULL )
			delete pElements[i];
	}
	CGPtrTypeArray<TYPE>::DestructElements(pElements,nCount);
}

template<class TYPE>
void CGObArray<TYPE>::Clean(bool bElements)
{
	if ( bElements && STANDARD_CPLUSPLUS_THIS(GetRealCount()) )
		DestructElements( STANDARD_CPLUSPLUS_THIS(GetBasePtr()), STANDARD_CPLUSPLUS_THIS(GetRealCount()) );
	
	STANDARD_CPLUSPLUS_THIS(Empty());
}

template<class TYPE>
bool CGObArray<TYPE>::DeleteOb( TYPE pData )
{
	return RemovePtr(pData);
}

template<class TYPE>
void CGObArray<TYPE>::DeleteAt( int nIndex )
{
	STANDARD_CPLUSPLUS_THIS(RemoveAt(nIndex));
}

template<class TYPE>
CGObArray<TYPE>::~CGObArray()
{
	// Make sure the virtuals get called.
	STANDARD_CPLUSPLUS_THIS(SetCount(0));
}

////////////////////////////////////////////////////////////
// CGObSortArray = A sorted array of objects.

template<class TYPE,class KEY_TYPE>
int CGObSortArray<TYPE,KEY_TYPE>::FindKey( KEY_TYPE key ) const
{
	// Find exact key
	int iCompareRes;
	int index = FindKeyNear(key, iCompareRes, false);
	return (iCompareRes ? -1 : index);
}

template<class TYPE,class KEY_TYPE>
int CGObSortArray<TYPE,KEY_TYPE>::AddPresorted( int index, int iCompareRes, TYPE pNew )
{
	if ( iCompareRes > 0 )
		index++;

	InsertAt(index, pNew);
	return index;
}

template<class TYPE,class KEY_TYPE>
void CGObSortArray<TYPE,KEY_TYPE>::DeleteKey( KEY_TYPE key )
{
	DeleteAt(FindKey(key));
}

template<class TYPE, class KEY_TYPE>
int CGObSortArray<TYPE, KEY_TYPE>::FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces ) const
{
	int i = 0;

	// Do a binary search for the key.
	// RETURN: index
	//  iCompareRes =
	//		0 = match with index.
	//		-1 = key should be less than index.
	//		+1 = key should be greater than index
	//

	int iHigh = STANDARD_CPLUSPLUS_THIS(GetCount()) - 1;
	if ( iHigh < 0 )
	{
		iCompareRes = -1;
		return( 0 );
	}

	int iLow = 0;
	while ( iLow <= iHigh )
	{
		i = (iHigh+iLow)/2;
		iCompareRes = CompareKey( key, STANDARD_CPLUSPLUS_THIS(GetAt(i)), fNoSpaces );
		if ( iCompareRes == 0 )
			break;
		if ( iCompareRes > 0 )
		{
			iLow = i+1;
		}
		else
		{
			iHigh = i-1;
		}
	}
	return i;
}

template<class TYPE, class KEY_TYPE>
int CGObSortArray<TYPE, KEY_TYPE>::AddSortKey( TYPE pNew, KEY_TYPE key )
{
	// Insertion sort.
	int iCompareRes;
	int index = FindKeyNear(key, iCompareRes);
	if ( !iCompareRes )
	{
		// duplicate should not happen ?!? DestructElements is called automatically for previous.
		SetAt(index, pNew);
		return index;
	}
	return AddPresorted(index, iCompareRes, pNew);
}

inline void CGObListRec::RemoveSelf()
{
	// Remove myself from my parent list (if i have one)
	if ( GetParent() )
		m_pParent->RemoveAtSpecial(this);
}

#undef STANDARD_CPLUSPLUS_THIS

#endif	// _INC_CARRAY_H