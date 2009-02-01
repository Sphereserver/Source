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

	CListDefContElem(LPCTSTR pszKey);
	~CListDefContElem(void);

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

	CListDefContNum(LPCTSTR pszKey);
	CListDefContNum(LPCTSTR pszKey, int iVal);
	~CListDefContNum(void);

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
	CListDefContStr(LPCTSTR pszKey);
	~CListDefContStr(void);

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

	inline CListDefContElem* ElementAt(int nIndex) const;
	inline void DeleteAtIterator(DefList::iterator it);
	inline DefList::iterator _GetAt(int nIndex);

public:
	static const char *m_sClassName;

	CListDefCont(LPCTSTR pszKey);
	~CListDefCont(void);

	LPCTSTR GetKey() const;
	void SetKey( LPCTSTR pszKey );

	CListDefContElem* GetAt(int nIndex) const;
	bool SetNumAt(int nIndex, int iVal);
	bool SetStrAt(int nIndex, LPCTSTR pszVal);
	int GetCount() const;

	LPCTSTR GetValStr(int nIndex) const;
	int GetValNum(int nIndex) const;

	int FindValNum( int iVal ) const;
	int FindValStr( LPCTSTR pVal ) const;

	bool AddElementNum(int iVal);
	bool AddElementStr(LPCTSTR pszKey);

	bool RemoveElement(int nIndex);
	void RemoveAll();

	bool InsertElementNum(int nIndex, int iVal);
	bool InsertElementStr(int nIndex, LPCTSTR pszKey);

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

private:
	CListDefCont * GetAtKey( LPCTSTR at );
	inline void DeleteAt( int at );
	inline void DeleteAtKey( LPCTSTR at );
	inline void DeleteAtIterator( DefSet::iterator it );

public:
	void Copy( const CListDefMap * pArray );
	void Empty();
	int GetCount() const;

	CListDefMap & operator = ( const CListDefMap & array );
	~CListDefMap();

	LPCTSTR FindValNum( int iVal ) const;
	LPCTSTR FindValStr( LPCTSTR pVal ) const;

	CListDefCont * GetAt( int at );
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