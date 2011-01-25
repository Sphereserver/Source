//
// CString.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CSTRING_H
#define _INC_CSTRING_H
#pragma once

#include "common.h"

class CGString
{
	private:
		TCHAR	*m_pchData;
		int		m_iLength;
		int		m_iMaxLength;

	public:
		static const char *m_sClassName;

	private:
		void Init();

	public:
		~CGString();
		CGString();
		CGString(LPCTSTR pStr);
		CGString(const CGString &s);

		bool IsValid() const;
		int SetLength( int iLen );
		int GetLength() const;
		bool IsEmpty() const;
		void Empty(bool bTotal = false);
		void Copy( LPCTSTR pStr );

		TCHAR & ReferenceAt(int nIndex);       // 0 based
		TCHAR GetAt(int nIndex) const;      // 0 based
		void SetAt(int nIndex, TCHAR ch);
		LPCTSTR GetPtr() const;

		void FormatV( LPCTSTR pStr, va_list args );
		void _cdecl Format( LPCTSTR pStr, ... );
		void FormatVal( long iVal );
		void FormatUVal( unsigned long iVal );
		void FormatHex( DWORD dwVal );

		int Compare( LPCTSTR pStr ) const;
		int CompareNoCase( LPCTSTR pStr ) const;
		int indexOf( TCHAR c );
		int indexOf( TCHAR c, int offset );
		int indexOf( CGString str, int offset );
		int indexOf( CGString str ) ;
		int lastIndexOf( TCHAR c );
		int lastIndexOf( TCHAR c, int from );
		int lastIndexOf(CGString str, int from);
		int lastIndexOf( CGString str ) ;
		void Add( TCHAR ch );
		void Add( LPCTSTR pszStr );
		void Reverse();
		void MakeUpper() { _strupr(m_pchData); }
		void MakeLower() { _strlwr(m_pchData); }

		TCHAR operator[](int nIndex) const // same as GetAt
		{
			return GetAt( nIndex );
		}
		TCHAR & operator[](int nIndex)
		{
			return ReferenceAt(nIndex);
		}
		operator LPCTSTR() const       // as a C string
		{
			return( GetPtr());
		}
		const CGString& operator+=(LPCTSTR psz)	// like strcat
		{
			Add(psz);
			return( *this );
		}
		const CGString& operator+=(TCHAR ch)
		{
			Add(ch);
			return( *this );
		}
		const CGString& operator=( LPCTSTR pStr )
		{
			Copy( pStr );
			return( *this );
		}
		const CGString& operator=( const CGString &s )
		{
			Copy( s.GetPtr());
			return( *this );
		}
};

enum MATCH_TYPE			// match result defines
{
	MATCH_INVALID = 0,
	MATCH_VALID,		// valid match
	MATCH_END,			// premature end of pattern string
	MATCH_ABORT,		// premature end of text string
	MATCH_RANGE,		// match failure on [..] construct
	MATCH_LITERAL,		// match failure on literal match
	MATCH_PATTERN,		// bad pattern
};
extern MATCH_TYPE Str_Match( LPCTSTR pPattern, LPCTSTR pText );

extern int Str_RegExMatch( LPCTSTR pPattern, LPCTSTR pText, TCHAR * lastError );

extern size_t strcpylen( TCHAR * pDst, LPCTSTR pSrc );
extern size_t strcpylen( TCHAR * pDst, LPCTSTR pSrc, size_t imaxlen );

extern LPCTSTR Str_GetArticleAndSpace( LPCTSTR pszWords );
extern size_t Str_GetBare( TCHAR * pszOut, LPCTSTR pszInp, size_t iMaxSize, LPCTSTR pszStrip = NULL );
extern bool Str_Check( LPCTSTR pszIn );
extern bool Str_CheckName(LPCTSTR pszIn);
extern TCHAR * Str_MakeFiltered( TCHAR * pStr );
extern void Str_MakeUnFiltered( TCHAR * pStrOut, LPCTSTR pStrIn, int iSizeMax );
extern size_t Str_TrimEndWhitespace( TCHAR * pStr, size_t len );
extern TCHAR * Str_TrimWhitespace( TCHAR * pStr );
extern int Str_IndexOf( TCHAR * pStr1, TCHAR * pStr2, int offset = 0 );
extern bool Str_Parse( TCHAR * pLine, TCHAR ** ppArg = NULL, LPCTSTR pSep = NULL );
extern int Str_ParseCmds( TCHAR * pCmdLine, TCHAR ** ppCmd, int iMax, LPCTSTR pSep = NULL );
extern int Str_ParseCmds( TCHAR * pCmdLine, int * piCmd, int iMax, LPCTSTR pSep = NULL );

extern int FindTable( LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));
extern int FindTableSorted( LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));
extern int FindTableHead( LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));
extern int FindTableHeadSorted( LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));

extern void CharToMultiByteNonNull( BYTE* , const char* , size_t);

// extern TCHAR * Str_GetTemporary(int amount = 1);
#define Str_GetTemp STATIC_CAST<AbstractSphereThread *>(ThreadHolder::current())->allocateBuffer

#endif // _INC_CSTRING_H
