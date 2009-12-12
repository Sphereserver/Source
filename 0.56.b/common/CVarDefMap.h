#ifndef _INC_CVARDEFMAP_H
#define _INC_CVARDEFMAP_H
#pragma once

#include <set>

class CVarDefCont
{
private:
	CGString m_Key;	// reference to map key

public:
	static const char *m_sClassName;

	CVarDefCont( LPCTSTR pszKey );
	~CVarDefCont();

	LPCTSTR GetKey() const;
	void SetKey( LPCTSTR pszKey );

	virtual LPCTSTR GetValStr() const = 0;
	virtual int GetValNum() const = 0;
	virtual CVarDefCont * CopySelf() const = 0;
};

class CVarDefContNum : public CVarDefCont
{
private:
	int m_iVal;

public:
	static const char *m_sClassName;

	CVarDefContNum( LPCTSTR pszKey, int iVal );
	CVarDefContNum( LPCTSTR pszKey );
	~CVarDefContNum();

	int GetValNum() const;
	void SetValNum( int iVal );
	LPCTSTR GetValStr() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );

	virtual CVarDefCont * CopySelf() const;
};

class CVarDefContStr : public CVarDefCont
{
private:
	CGString m_sVal;	// the assigned value. (What if numeric?)

public:
	static const char *m_sClassName;

	CVarDefContStr( LPCTSTR pszKey, LPCTSTR pszVal );
	CVarDefContStr( LPCTSTR pszKey );
	~CVarDefContStr();

	LPCTSTR GetValStr() const;
	void SetValStr( LPCTSTR pszVal );
	int GetValNum() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );

	virtual CVarDefCont * CopySelf() const;
};


class CVarDefMap
{
private:
	struct ltstr
	{
		bool operator()(CVarDefCont * s1, CVarDefCont * s2) const;
	};

	typedef std::set<CVarDefCont *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

	class CVarDefContTest : public CVarDefCont // This is to alloc CVarDefCont without allocing any other things
	{
		public:
			static const char *m_sClassName;

			CVarDefContTest( LPCTSTR pszKey );
			~CVarDefContTest();

			LPCTSTR GetValStr() const;
			int GetValNum() const;
			virtual CVarDefCont * CopySelf() const;
	};

private:
	DefSet m_Container;

public:
	static const char *m_sClassName;

private:
	CVarDefCont * GetAtKey( LPCTSTR at );
	void DeleteAt( int at );
	void DeleteAtKey( LPCTSTR at );
	void DeleteAtIterator( DefSet::iterator it );

	int SetNumOverride( LPCTSTR pszKey, int iVal );
	int SetStrOverride( LPCTSTR pszKey, LPCTSTR pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	void Empty();
	int GetCount() const;

	CVarDefMap & operator = ( const CVarDefMap & array );
	~CVarDefMap();

	LPCTSTR FindValNum( int iVal ) const;
	LPCTSTR FindValStr( LPCTSTR pVal ) const;

	int SetNumNew( LPCTSTR pszKey, int iVal );
	int SetNum( LPCTSTR pszKey, int iVal, bool fZero = false );
	int SetStrNew( LPCTSTR pszKey, LPCTSTR pszVal );
	int SetStr( LPCTSTR pszKey, bool fQuoted, LPCTSTR pszVal, bool fZero = false );

	CVarDefCont * GetAt( int at );
	CVarDefCont * GetKey( LPCTSTR pszKey ) const;
	int GetKeyNum( LPCTSTR pszKey, bool fZero = false ) const;
	LPCTSTR GetKeyStr( LPCTSTR pszKey, bool fZero = false ) const;
	CVarDefCont * GetParseKey( LPCTSTR & pArgs ) const;
	bool GetParseVal( LPCTSTR & pArgs, long * plVal ) const;

	void DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix = NULL );
	void ClearKeys(LPCTSTR mask = NULL);
	void DeleteKey( LPCTSTR key );

	bool r_LoadVal( CScript & s );
	void r_WritePrefix( CScript & s, LPCTSTR pszPrefix, LPCTSTR pszKeyExclude = NULL );
};

#endif
