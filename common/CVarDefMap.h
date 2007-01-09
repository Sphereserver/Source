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

	CVarDefCont( LPCTSTR pszKey ) : m_Key( pszKey ) { m_Key.MakeLower(); }
	~CVarDefCont() {}

	LPCTSTR GetKey() const { return( m_Key.GetPtr() ); }

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

	CVarDefContNum( LPCTSTR pszKey, int iVal ) : CVarDefCont( pszKey ), m_iVal( iVal ) {}
	CVarDefContNum( LPCTSTR pszKey ) : CVarDefCont( pszKey ) {}
	~CVarDefContNum() {}

	int GetValNum() const { return( m_iVal ); }
	void SetValNum( int iVal ) { m_iVal = iVal; }
	LPCTSTR GetValStr() const;

	bool r_LoadVal( CScript & s )
	{
		SetValNum( s.GetArgVal());
		return( true );
	}
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
	{
		UNREFERENCED_PARAMETER(pKey);
		UNREFERENCED_PARAMETER(pSrc);
		sVal.FormatVal( GetValNum() );
		return( true );
	}

	virtual CVarDefCont * CopySelf() const { return new CVarDefContNum( GetKey(), m_iVal ); }
};

class CVarDefContStr : public CVarDefCont
{
private:
	CGString m_sVal;	// the assigned value. (What if numeric?)

public:
	static const char *m_sClassName;

	CVarDefContStr( LPCTSTR pszKey, LPCTSTR pszVal ) : CVarDefCont( pszKey ), m_sVal( pszVal ) {}
	CVarDefContStr( LPCTSTR pszKey ) : CVarDefCont( pszKey ) {}
	~CVarDefContStr() {}

	LPCTSTR GetValStr() const { return( m_sVal ); }
	void SetValStr( LPCTSTR pszVal ) { m_sVal.Copy( pszVal ); }
	int GetValNum() const;

	bool r_LoadVal( CScript & s )
	{
		SetValStr( s.GetArgStr());
		return( true );
	}
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc = NULL )
	{
		UNREFERENCED_PARAMETER(pKey);
		UNREFERENCED_PARAMETER(pSrc);
		sVal = GetValStr();
		return( true );
	}

	virtual CVarDefCont * CopySelf() const { return new CVarDefContStr( GetKey(), m_sVal ); }
};


class CVarDefMap
{
private:
	struct ltstr
	{
		bool operator()(CVarDefCont * s1, CVarDefCont * s2) const
		{
			return( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
		}
	};

	typedef std::set<CVarDefCont *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

	class CVarDefContTest : public CVarDefCont // This is to alloc CVarDefCont without allocing any other things
	{
		public:
			static const char *m_sClassName;

			CVarDefContTest( LPCTSTR pszKey ) : CVarDefCont( pszKey ) {}
			~CVarDefContTest() {}

			LPCTSTR GetValStr() const { return NULL; }
			int GetValNum() const { return -1; }
			virtual CVarDefCont * CopySelf() const { return new CVarDefContTest( GetKey() ); }
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

	bool SetNumOverride( LPCTSTR pszKey, int iVal );
	bool SetStrOverride( LPCTSTR pszKey, LPCTSTR pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	void Empty();
	int GetCount() const;

	CVarDefMap & operator = ( const CVarDefMap & array )
	{
		Copy( &array );
		return( *this );
	}

	~CVarDefMap()
	{
		Empty();
	}

	LPCTSTR FindValNum( int iVal ) const;
	LPCTSTR FindValStr( LPCTSTR pVal ) const;

	bool SetNumNew( LPCTSTR pszKey, int iVal );
	int SetNum( LPCTSTR pszKey, int iVal, bool fZero = false );
	bool SetStrNew( LPCTSTR pszKey, LPCTSTR pszVal );
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