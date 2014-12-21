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
	virtual ~CVarDefCont();

private:
	CVarDefCont(const CVarDefCont& copy);
	CVarDefCont& operator=(const CVarDefCont& other);

public:
	LPCTSTR GetKey() const;
	void SetKey( LPCTSTR pszKey );

	virtual LPCTSTR GetValStr() const = 0;
	virtual INT64 GetValNum() const = 0;
	virtual CVarDefCont * CopySelf() const = 0;
};

class CVarDefContNum : public CVarDefCont
{
private:
	INT64 m_iVal;

public:
	static const char *m_sClassName;

	CVarDefContNum( LPCTSTR pszKey, INT64 iVal );
	CVarDefContNum( LPCTSTR pszKey );
	~CVarDefContNum();

private:
	CVarDefContNum(const CVarDefContNum& copy);
	CVarDefContNum& operator=(const CVarDefContNum& other);

public:
	INT64 GetValNum() const;
	void SetValNum( INT64 iVal );
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
	explicit CVarDefContStr( LPCTSTR pszKey );
	~CVarDefContStr();

private:
	CVarDefContStr(const CVarDefContStr& copy);
	CVarDefContStr& operator=(const CVarDefContStr& other);

public:
	LPCTSTR GetValStr() const;
	void SetValStr( LPCTSTR pszVal );
	INT64 GetValNum() const;

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

		private:
			CVarDefContTest(const CVarDefContTest& copy);
			CVarDefContTest& operator=(const CVarDefContTest& other);

		public:
			LPCTSTR GetValStr() const;
			INT64 GetValNum() const;
			virtual CVarDefCont * CopySelf() const;
	};

private:
	DefSet m_Container;

public:
	static const char *m_sClassName;

private:
	CVarDefCont * GetAtKey( LPCTSTR at ) const;
	void DeleteAt( size_t at );
	void DeleteAtKey( LPCTSTR at );
	void DeleteAtIterator( DefSet::iterator it );

	int SetNumOverride( LPCTSTR pszKey, INT64 iVal );
	int SetStrOverride( LPCTSTR pszKey, LPCTSTR pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	bool Compare( const CVarDefMap * pArray );
	void Empty();
	size_t GetCount() const;

public:
	CVarDefMap() { };
	~CVarDefMap();
	CVarDefMap & operator = ( const CVarDefMap & array );

private:
	CVarDefMap(const CVarDefMap& copy);

public:
	LPCTSTR FindValNum( INT64 iVal ) const;
	LPCTSTR FindValStr( LPCTSTR pVal ) const;

	int SetNumNew( LPCTSTR pszKey, INT64 iVal );
	int SetNum( LPCTSTR pszKey, INT64 iVal, bool fZero = false );
	int SetStrNew( LPCTSTR pszKey, LPCTSTR pszVal );
	int SetStr( LPCTSTR pszKey, bool fQuoted, LPCTSTR pszVal, bool fZero = false );

	CVarDefCont * GetAt( size_t at ) const;
	CVarDefCont * GetKey( LPCTSTR pszKey ) const;
	INT64 GetKeyNum( LPCTSTR pszKey, bool fZero = false ) const;
	LPCTSTR GetKeyStr( LPCTSTR pszKey, bool fZero = false ) const;
	CVarDefCont * GetParseKey( LPCTSTR & pArgs ) const;
	CVarDefCont * CheckParseKey( LPCTSTR & pszArgs ) const;
	bool GetParseVal( LPCTSTR & pArgs, long long * plVal ) const;

	void DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix = NULL ) const;
	void ClearKeys(LPCTSTR mask = NULL);
	void DeleteKey( LPCTSTR key );

	bool r_LoadVal( CScript & s );
	void r_WritePrefix( CScript & s, LPCTSTR pszPrefix = NULL, LPCTSTR pszKeyExclude = NULL );
};

#endif
