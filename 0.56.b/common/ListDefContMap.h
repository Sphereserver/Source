#ifndef _INC_CLISTDEFMAP_H
#define _INC_CLISTDEFMAP_H
#pragma once

#include <list>
#include <set>
//#include "../common/cstring.h"
//#include "../common/cscript.h"

class CListDefContElem
{
private:
	CGString m_Key;	// reference to map key

public:
	static const char *m_sClassName;

	explicit CListDefContElem(LPCTSTR pszKey);
	virtual ~CListDefContElem(void);

private:
	CListDefContElem(const CListDefContElem& copy);
	CListDefContElem& operator=(const CListDefContElem& other);

public:
	LPCTSTR GetKey() const;
	void SetKey( LPCTSTR pszKey );

	virtual LPCTSTR GetValStr() const = 0;
	virtual int GetValNum() const = 0;
	virtual CListDefContElem * CopySelf() const = 0;
};

class CListDefContNum: public CListDefContElem
{
private:
	int m_iVal;

public:
	static const char *m_sClassName;

	explicit CListDefContNum(LPCTSTR pszKey);
	CListDefContNum(LPCTSTR pszKey, int iVal);
	~CListDefContNum(void);

private:
	CListDefContNum(const CListDefContNum& copy);
	CListDefContNum& operator=(const CListDefContNum& other);

public:
	int GetValNum() const;
	void SetValNum( int iVal );
	LPCTSTR GetValStr() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );

	virtual CListDefContElem * CopySelf() const;
};

class CListDefContStr: public CListDefContElem
{
private:
	CGString m_sVal;	// the assigned value. (What if numeric?)

public:
	static const char *m_sClassName;

	CListDefContStr(LPCTSTR pszKey, LPCTSTR pszVal);
	explicit CListDefContStr(LPCTSTR pszKey);
	~CListDefContStr(void);

private:
	CListDefContStr(const CListDefContStr& copy);
	CListDefContStr& operator=(const CListDefContStr& other);

public:
	LPCTSTR GetValStr() const;
	void SetValStr( LPCTSTR pszVal );
	int GetValNum() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );

	virtual CListDefContElem * CopySelf() const;
};

class CListDefCont
{
private:
	CGString m_Key;	// reference to map key

	typedef std::list<CListDefContElem *> DefList;

protected:
	DefList	m_listElements;

	inline CListDefContElem* ElementAt(size_t nIndex) const;
	inline void DeleteAtIterator(DefList::iterator it);
	inline DefList::iterator _GetAt(size_t nIndex);

public:
	static const char *m_sClassName;

	explicit CListDefCont(LPCTSTR pszKey);
	~CListDefCont(void);

private:
	CListDefCont(const CListDefCont& copy);
	CListDefCont& operator=(const CListDefCont& other);

public:
	LPCTSTR GetKey() const;
	void SetKey( LPCTSTR pszKey );

	CListDefContElem* GetAt(size_t nIndex) const;
	bool SetNumAt(size_t nIndex, int iVal);
	bool SetStrAt(size_t nIndex, LPCTSTR pszVal);
	size_t GetCount() const;

	LPCTSTR GetValStr(size_t nIndex) const;
	int GetValNum(size_t nIndex) const;

	int FindValNum( int iVal, size_t nStartIndex = 0 ) const;
	int FindValStr( LPCTSTR pVal, size_t nStartIndex = 0 ) const;

	bool AddElementNum(int iVal);
	bool AddElementStr(LPCTSTR pszKey);

	bool RemoveElement(size_t nIndex);
	void RemoveAll();

	bool InsertElementNum(size_t nIndex, int iVal);
	bool InsertElementStr(size_t nIndex, LPCTSTR pszKey);

	CListDefCont * CopySelf();
	void PrintElements(CGString& strElements) const;
	void DumpElements( CTextConsole * pSrc, LPCTSTR pszPrefix = NULL ) const;
	void r_WriteSave( CScript& s );
	bool r_LoadVal( CScript& s );
};

class CListDefMap
{
private:
	struct ltstr
	{
		bool operator()(CListDefCont * s1, CListDefCont * s2) const;
	};

	typedef std::set<CListDefCont *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

private:
	DefSet m_Container;

public:
	static const char *m_sClassName;
	CListDefMap & operator = ( const CListDefMap & array );
	CListDefMap() { };
	~CListDefMap();

private:
	CListDefMap(const CListDefMap& copy);

private:
	CListDefCont * GetAtKey( LPCTSTR at );
	inline void DeleteAt( size_t at );
	inline void DeleteAtKey( LPCTSTR at );
	inline void DeleteAtIterator( DefSet::iterator it );

public:
	void Copy( const CListDefMap * pArray );
	void Empty();
	size_t GetCount() const;

	LPCTSTR FindValNum( int iVal ) const;
	LPCTSTR FindValStr( LPCTSTR pVal ) const;

	CListDefCont * GetAt( size_t at );
	CListDefCont * GetKey( LPCTSTR pszKey ) const;

	CListDefCont* AddList(LPCTSTR pszKey);

	void DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix = NULL );
	void ClearKeys(LPCTSTR mask = NULL);
	void DeleteKey( LPCTSTR key );

	bool r_LoadVal( LPCTSTR pszKey, CScript & s );
	bool r_Write( CTextConsole *pSrc, LPCTSTR pszString, CGString& strVal );
	void r_WriteSave( CScript& s );
};

#endif
