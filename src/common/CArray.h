#ifndef _INC_CARRAY_H
#define _INC_CARRAY_H
#pragma once

#if defined(__MINGW32__) || defined(__linux) || defined(__FreeBSD__)
	#define STANDARD_CPLUSPLUS_THIS(_x_) this->_x_
#else
	#define STANDARD_CPLUSPLUS_THIS(_x_) _x_
#endif

///////////////////////////////////////////////////////////
// CGObListRec

class CGObList;

// Generic list record
// Each CGObListRec belongs to just one CGObList
class CGObListRec
{
	friend class CGObList;

public:
	static const char *m_sClassName;

	CGObListRec();
	virtual ~CGObListRec();

private:
	CGObList *m_pParent;
	CGObListRec *m_pNext;
	CGObListRec *m_pPrev;

public:
	inline void RemoveSelf();

public:
	CGObList *GetParent() const
	{
		return m_pParent;
	}
	CGObListRec *GetNext() const
	{
		return m_pNext;
	}
	CGObListRec *GetPrev() const
	{
		return m_pPrev;
	}

private:
	CGObListRec(const CGObListRec &copy);
	CGObListRec &operator=(const CGObListRec &other);
};

///////////////////////////////////////////////////////////
// CGObList

// Generic list of objects
class CGObList
{
	friend class CGObListRec;

public:
	static const char *m_sClassName;

	CGObList();
	virtual ~CGObList();

private:
	CGObListRec *m_pHead;
	CGObListRec *m_pTail;
	size_t m_iCount;

public:
	CGObListRec *GetAt(size_t i) const;
	CGObListRec *GetHead() const;
	CGObListRec *GetTail() const;
	size_t GetCount() const;
	bool IsEmpty() const;

	virtual void InsertAfter(CGObListRec *pNewRec, CGObListRec *pPrev);
	void InsertHead(CGObListRec *pNewRec);
	void InsertTail(CGObListRec *pNewRec);
	void DeleteAll();

private:
	void RemoveAtSpecial(CGObListRec *pObRec);

protected:
	virtual void OnRemoveOb(CGObListRec *pObRec);

private:
	CGObList(const CGObList &copy);
	CGObList &operator=(const CGObList &other);
};

inline void CGObListRec::RemoveSelf()
{
	// Remove from parent list
	if ( GetParent() )
		m_pParent->RemoveAtSpecial(this);
}

///////////////////////////////////////////////////////////
// CGTypedArray

// Array of TYPEs
// NOTE: this will not call true constructors/destructors
template<class TYPE, class ARG_TYPE>
class CGTypedArray
{
public:
	static const char *m_sClassName;

	CGTypedArray();
	virtual ~CGTypedArray();

private:
	TYPE *m_pData;
	size_t m_iCount;
	size_t m_iRealCount;

public:
	TYPE GetAt(size_t i) const;
	TYPE &ElementAt(size_t i);
	const TYPE &ElementAt(size_t i) const;

	TYPE *GetData() const;
	size_t GetCount() const;
	size_t GetRealCount() const;
	void SetCount(size_t iNewCount);

	size_t Add(ARG_TYPE newElement);
	void RemoveAll();

	void InsertAt(size_t i, ARG_TYPE newElement);
	void RemoveAt(size_t i);

	void SetAt(size_t i, ARG_TYPE newElement);
	void SetAtGrow(size_t i, ARG_TYPE newElement);

	void Copy(const CGTypedArray<TYPE, ARG_TYPE> *pArray);

	virtual void ConstructElements(TYPE *pElements, size_t iCount);
	virtual void DestructElements(TYPE *pElements, size_t iCount);

	bool IsValidIndex(size_t i) const;
	inline size_t BadIndex() const
	{
		return SIZE_MAX;
	}

private:
	CGTypedArray<TYPE, ARG_TYPE>(const CGTypedArray<TYPE, ARG_TYPE> &copy);

public:
	const CGTypedArray<TYPE, ARG_TYPE> &operator=(const CGTypedArray<TYPE, ARG_TYPE> &other);
	TYPE operator[](size_t i) const;
	TYPE &operator[](size_t i);
};

///////////////////////////////////////////////////////////
// CGPtrTypeArray

// Array of pointers
template<class TYPE>
class CGPtrTypeArray : public CGTypedArray<TYPE, TYPE>
{
public:
	static const char *m_sClassName;

	CGPtrTypeArray() { };
	virtual ~CGPtrTypeArray() { };

public:
	size_t FindPtr(TYPE pData) const;
	bool ContainsPtr(TYPE pData) const;
	bool RemovePtr(TYPE pData);

	bool IsValidIndex(size_t i) const;

protected:
	virtual void DestructElements(TYPE *pElements, size_t iCount);

private:
	CGPtrTypeArray<TYPE>(const CGPtrTypeArray<TYPE> &copy);
	CGPtrTypeArray<TYPE> &operator=(const CGPtrTypeArray<TYPE> &other);
};

///////////////////////////////////////////////////////////
// CGObArray

// Array of objects
// The point of this type is that the array now owns the element (it will get deleted when the array is deleted)
template<class TYPE>
class CGObArray : public CGPtrTypeArray<TYPE>
{
public:
	static const char *m_sClassName;

	CGObArray() { };
	virtual ~CGObArray();

public:
	void DeleteAt(size_t i);
	bool DeleteOb(TYPE pData);
	void Clean(bool fElements = false);

protected:
	virtual void DestructElements(TYPE *pElements, size_t iCount);

private:
	CGObArray<TYPE>(const CGObArray<TYPE> &copy);
	CGObArray<TYPE> &operator=(const CGObArray<TYPE> &other);
};

///////////////////////////////////////////////////////////
// CGObSortArray

// Array of objects (sorted)
// The point of this type is that the array now owns the element (it will get deleted when the array is deleted)
template<class TYPE, class KEY_TYPE>
struct CGObSortArray : public CGObArray<TYPE>
{
public:
	CGObSortArray() { };
	virtual ~CGObSortArray() { };

public:
	size_t FindKey(KEY_TYPE key) const;
	size_t FindKeyNear(KEY_TYPE key, int &iCompareRes, bool fNoSpaces = false) const;
	bool ContainsKey(KEY_TYPE key) const;
	virtual int CompareKey(KEY_TYPE, TYPE, bool fNoSpaces) const = 0;

	size_t AddSortKey(TYPE pNew, KEY_TYPE key);
	void DeleteKey(KEY_TYPE key);

private:
	CGObSortArray<TYPE, KEY_TYPE>(const CGObSortArray<TYPE, KEY_TYPE> &copy);
	CGObSortArray<TYPE, KEY_TYPE> &operator=(const CGObSortArray<TYPE, KEY_TYPE> &other);
};

///////////////////////////////////////////////////////////
// CGTypedArray

template<class TYPE, class ARG_TYPE>
CGTypedArray<TYPE, ARG_TYPE>::CGTypedArray()
{
	m_pData = NULL;
	m_iCount = 0;
	m_iRealCount = 0;
}

template<class TYPE, class ARG_TYPE>
CGTypedArray<TYPE, ARG_TYPE>::~CGTypedArray()
{
	SetCount(0);
}

template<class TYPE, class ARG_TYPE>
TYPE CGTypedArray<TYPE, ARG_TYPE>::GetAt(size_t i) const
{
	if ( !IsValidIndex(i) )
		return *reinterpret_cast<TYPE *>(BadIndex());

	return m_pData[i];
}

template<class TYPE, class ARG_TYPE>
TYPE &CGTypedArray<TYPE, ARG_TYPE>::ElementAt(size_t i)
{
	if ( !IsValidIndex(i) )
		return *reinterpret_cast<TYPE *>(BadIndex());

	return m_pData[i];
}

template<class TYPE, class ARG_TYPE>
const TYPE &CGTypedArray<TYPE, ARG_TYPE>::ElementAt(size_t i) const
{
	if ( !IsValidIndex(i) )
		return *reinterpret_cast<const TYPE *>(BadIndex());

	return m_pData[i];
}

template<class TYPE, class ARG_TYPE>
TYPE *CGTypedArray<TYPE, ARG_TYPE>::GetData() const
{
	// This is dangerous to use of course
	return m_pData;
}

template<class TYPE, class ARG_TYPE>
size_t CGTypedArray<TYPE, ARG_TYPE>::GetCount() const
{
	return m_iCount;
}

template<class TYPE, class ARG_TYPE>
size_t CGTypedArray<TYPE, ARG_TYPE>::GetRealCount() const
{
	return m_iRealCount;
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::SetCount(size_t iNewCount)
{
	ASSERT(iNewCount != STANDARD_CPLUSPLUS_THIS(BadIndex()));

	if ( iNewCount == 0 )
	{
		// Shrink to nothing
		if ( m_iCount > 0 )
		{
			DestructElements(m_pData, m_iCount);
			delete[] reinterpret_cast<BYTE *>(m_pData);
			m_pData = NULL;					// before Clean(true) in CGObArray
			m_iCount = m_iRealCount = 0;	// that's probably wrong.. but SetCount(0) should be never called
		}
		return;
	}

	if ( iNewCount > m_iCount )
	{
		TYPE *pNewData = reinterpret_cast<TYPE *>(new BYTE[iNewCount * sizeof(TYPE)]);
		if ( m_iCount )
		{
			// Copy the old stuff to the new array
			memcpy(static_cast<void *>(pNewData), m_pData, sizeof(TYPE) * m_iCount);
			delete[] reinterpret_cast<BYTE *>(m_pData);		// don't call any destructor
		}

		// Just construct or init the new stuff
		ConstructElements(pNewData + m_iCount, iNewCount - m_iCount);
		m_pData = pNewData;
		m_iRealCount = iNewCount;
	}

	m_iCount = iNewCount;
}

template<class TYPE, class ARG_TYPE>
size_t CGTypedArray<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
{
	// Add to the end
	SetAtGrow(GetCount(), newElement);
	return m_iCount - 1;
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::RemoveAll()
{
	SetCount(0);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::InsertAt(size_t i, ARG_TYPE newElement)
{
	// Bump the existing entry here forward
	ASSERT(i != STANDARD_CPLUSPLUS_THIS(BadIndex()));

	SetCount((i >= m_iCount) ? i + 1 : m_iCount + 1);
	memmove(static_cast<void *>(&m_pData[i + 1]), &m_pData[i], sizeof(TYPE) * (m_iCount - i - 1));
	m_pData[i] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::RemoveAt(size_t i)
{
	if ( !IsValidIndex(i) )
		return;

	DestructElements(&m_pData[i], 1);
	memmove(static_cast<void *>(&m_pData[i]), &m_pData[i + 1], sizeof(TYPE) * (m_iCount - i - 1));
	SetCount(m_iCount - 1);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::SetAt(size_t i, ARG_TYPE newElement)
{
	if ( !IsValidIndex(i) )
		return;

	DestructElements(&m_pData[i], 1);
	m_pData[i] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::SetAtGrow(size_t i, ARG_TYPE newElement)
{
	ASSERT(i != STANDARD_CPLUSPLUS_THIS(BadIndex()));

	if ( i >= m_iCount )
		SetCount(i + 1);
	SetAt(i, newElement);
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::Copy(const CGTypedArray<TYPE, ARG_TYPE> *pArray)
{
	if ( !pArray || (pArray == this) )
		return;

	RemoveAll();
	SetCount(pArray->GetCount());
	memcpy(static_cast<void *>(GetData()), pArray->GetData(), GetCount() * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::ConstructElements(TYPE *pElements, size_t iCount)
{
	// First do bit-wise zero initialization
	memset(static_cast<void *>(pElements), 0, iCount * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::DestructElements(TYPE *pElements, size_t iCount)
{
	UNREFERENCED_PARAMETER(pElements);
	UNREFERENCED_PARAMETER(iCount);
	//memset(static_cast<void *>(pElements), 0, iCount * sizeof(*pElements));
}

template<class TYPE, class ARG_TYPE>
bool CGTypedArray<TYPE, ARG_TYPE>::IsValidIndex(size_t i) const
{
	return (i < m_iCount);
}

template<class TYPE, class ARG_TYPE>
const CGTypedArray<TYPE, ARG_TYPE> &CGTypedArray<TYPE, ARG_TYPE>::operator=(const CGTypedArray<TYPE, ARG_TYPE> &other)
{
	Copy(&other);
	return *this;
}

template<class TYPE, class ARG_TYPE>
TYPE CGTypedArray<TYPE, ARG_TYPE>::operator[](size_t i) const
{
	return GetAt(i);
}

template<class TYPE, class ARG_TYPE>
TYPE &CGTypedArray<TYPE, ARG_TYPE>::operator[](size_t i)
{
	return ElementAt(i);
}

///////////////////////////////////////////////////////////
// CGPtrTypeArray

template<class TYPE>
size_t CGPtrTypeArray<TYPE>::FindPtr(TYPE pData) const
{
	if ( pData )
	{
		for ( size_t i = 0; i < STANDARD_CPLUSPLUS_THIS(GetCount()); ++i )
		{
			if ( STANDARD_CPLUSPLUS_THIS(GetAt(i)) == pData )
				return i;
		}
	}
	return STANDARD_CPLUSPLUS_THIS(BadIndex());
}

template<class TYPE>
bool CGPtrTypeArray<TYPE>::ContainsPtr(TYPE pData) const
{
	return (FindPtr(pData) != STANDARD_CPLUSPLUS_THIS(BadIndex()));
}

template<class TYPE>
bool CGPtrTypeArray<TYPE>::RemovePtr(TYPE pData)
{
	size_t i = FindPtr(pData);
	if ( i == STANDARD_CPLUSPLUS_THIS(BadIndex()) )
		return false;

	STANDARD_CPLUSPLUS_THIS(RemoveAt(i));
	return true;
}

template<class TYPE>
bool CGPtrTypeArray<TYPE>::IsValidIndex(size_t i) const
{
	if ( i >= STANDARD_CPLUSPLUS_THIS(GetCount()) )
		return false;

	return (STANDARD_CPLUSPLUS_THIS(GetAt(i)) != NULL);
}

template<class TYPE>
void CGPtrTypeArray<TYPE>::DestructElements(TYPE *pElements, size_t iCount)
{
	memset(static_cast<void *>(pElements), 0, iCount * sizeof(*pElements));
}

///////////////////////////////////////////////////////////
// CGObArray

template<class TYPE>
CGObArray<TYPE>::~CGObArray()
{
	// Make sure the virtuals get called
	STANDARD_CPLUSPLUS_THIS(SetCount(0));
}

template<class TYPE>
void CGObArray<TYPE>::DeleteAt(size_t i)
{
	STANDARD_CPLUSPLUS_THIS(RemoveAt(i));
}

template<class TYPE>
bool CGObArray<TYPE>::DeleteOb(TYPE pData)
{
	return STANDARD_CPLUSPLUS_THIS(RemovePtr(pData));
}

template<class TYPE>
void CGObArray<TYPE>::Clean(bool fElements)
{
	if ( fElements && (STANDARD_CPLUSPLUS_THIS(GetRealCount()) > 0) )
		DestructElements(STANDARD_CPLUSPLUS_THIS(GetData()), STANDARD_CPLUSPLUS_THIS(GetRealCount()));

	STANDARD_CPLUSPLUS_THIS(RemoveAll());
}

template<class TYPE>
void CGObArray<TYPE>::DestructElements(TYPE *pElements, size_t iCount)
{
	for ( size_t i = 0; i < iCount; ++i )
	{
		if ( pElements[i] != NULL )
			delete pElements[i];
	}
	CGPtrTypeArray<TYPE>::DestructElements(pElements, iCount);
}

///////////////////////////////////////////////////////////
// CGObSortArray

template<class TYPE, class KEY_TYPE>
size_t CGObSortArray<TYPE, KEY_TYPE>::FindKey(KEY_TYPE key) const
{
	// Find exact key
	int iCompareRes;
	size_t i = FindKeyNear(key, iCompareRes, false);
	return (iCompareRes == 0) ? i : STANDARD_CPLUSPLUS_THIS(BadIndex());
}

template<class TYPE, class KEY_TYPE>
size_t CGObSortArray<TYPE, KEY_TYPE>::FindKeyNear(KEY_TYPE key, int &iCompareRes, bool fNoSpaces) const
{
	// Do a binary search for the key
	// ARGS:
	//  iCompareRes =
	//	  -1 = key should be less than index
	//	   0 = match with index
	//	  +1 = key should be greater than index
	// RETURN: index

	if ( STANDARD_CPLUSPLUS_THIS(GetCount()) <= 0 )
	{
		iCompareRes = -1;
		return 0;
	}

	size_t i = 0;
	size_t iLow = 0;
	size_t iHigh = STANDARD_CPLUSPLUS_THIS(GetCount()) - 1;

	while ( iLow <= iHigh )
	{
		i = (iHigh + iLow) / 2;
		iCompareRes = CompareKey(key, STANDARD_CPLUSPLUS_THIS(GetAt(i)), fNoSpaces);
		if ( iCompareRes == 0 )
			break;

		if ( iCompareRes > 0 )
			iLow = i + 1;
		else if ( i == 0 )
			break;
		else
			iHigh = i - 1;
	}
	return i;
}

template<class TYPE, class KEY_TYPE>
bool CGObSortArray<TYPE, KEY_TYPE>::ContainsKey(KEY_TYPE key) const
{
	return (FindKey(key) != STANDARD_CPLUSPLUS_THIS(BadIndex()));
}

template<class TYPE, class KEY_TYPE>
size_t CGObSortArray<TYPE, KEY_TYPE>::AddSortKey(TYPE pNew, KEY_TYPE key)
{
	int iCompareRes;
	size_t i = FindKeyNear(key, iCompareRes);
	if ( iCompareRes == 0 )
		STANDARD_CPLUSPLUS_THIS(SetAt(i, pNew));
	else
	{
		if ( iCompareRes > 0 )
			++i;
		STANDARD_CPLUSPLUS_THIS(InsertAt(i, pNew));
	}
	return i;
}

template<class TYPE, class KEY_TYPE>
void CGObSortArray<TYPE, KEY_TYPE>::DeleteKey(KEY_TYPE key)
{
	DeleteAt(FindKey(key));
}

#undef STANDARD_CPLUSPLUS_THIS

#endif	// _INC_CARRAY_H
