#ifndef _INC_CSCRIPT_H
#define _INC_CSCRIPT_H
#pragma once

#include "CMemBlock.h"
#include "CArray.h"
#include "CFile.h"
#include "CacheableScriptFile.h"

#define GRAY_SCRIPT		".scp"

#define SCRIPT_MAX_SECTION_LEN 128

struct CScriptLineContext
{
public:
	long m_lOffset;
	int m_iLineNum;		// for debug purposes if there is an error.

public:
	void Init()
	{
		m_lOffset = -1;
		m_iLineNum = -1;
	}
	bool IsValid() const
	{
		return( m_lOffset >= 0 );
	}

public:
	CScriptLineContext()
	{
		Init();
	}
};

class CScriptKey
{
	// A single line form a script file.
	// This is usually in the form of KEY=ARG
	// Unknown allocation of pointers.
protected:
	TCHAR * m_pszKey;		// The key. (or just start of the line)
	TCHAR * m_pszArg;		// for parsing the last read line.	KEY=ARG or KEY ARG

public:
	static const char *m_sClassName;
	bool IsKey( LPCTSTR pszName ) const
	{
		ASSERT(m_pszKey);
		return( ! strcmpi( m_pszKey, pszName ));
	}
	bool IsKeyHead( LPCTSTR pszName, size_t len ) const
	{
		ASSERT(m_pszKey);
		return( ! strnicmp( m_pszKey, pszName, len ));
	}

	void InitKey();

	// Query the key.
	LPCTSTR GetKey() const
	{
		// Get the key or section name.
		ASSERT(m_pszKey);
		return(m_pszKey);
	}

	// Args passed with the key.
	bool HasArgs() const
	{
		ASSERT(m_pszArg);
		return(( m_pszArg[0] ) ? true : false );
	}
	TCHAR * GetArgRaw() const	// Not need to parse at all.
	{
		ASSERT(m_pszArg);
		return(m_pszArg);
	}

	TCHAR * GetArgStr( bool * fQuoted );	// this could be a quoted string ?
	TCHAR * GetArgStr()
	{
		return GetArgStr( NULL );
	}
	long long GetArgLLVal();
	long GetArgVal();
	long GetArgRange();
	DWORD GetArgFlag( DWORD dwStart, DWORD dwMask );

public:
	CScriptKey() : m_pszKey(NULL), m_pszArg(NULL)
	{
	}

	CScriptKey( TCHAR * pszKey, TCHAR * pszArg ) :
		m_pszKey( pszKey ),
		m_pszArg( pszArg )
	{
	}

	virtual ~CScriptKey()
	{
	}

private:
	CScriptKey(const CScriptKey& copy);
	CScriptKey& operator=(const CScriptKey& other);
};

class CScriptKeyAlloc : public CScriptKey
{
	// Dynamic allocated script key.
protected:
	CMemLenBlock m_Mem;	// the buffer to hold data read.

protected:
	TCHAR * GetKeyBufferRaw( size_t iSize );

	bool ParseKey( LPCTSTR pszKey, LPCTSTR pszArgs );
	size_t ParseKeyEnd();

public:
	static const char *m_sClassName;
	TCHAR * GetKeyBuffer()
	{
		// Get the buffer the key is in. 
		ASSERT(m_Mem.GetData());
		return reinterpret_cast<TCHAR *>(m_Mem.GetData());
	}
	bool ParseKey( LPCTSTR pszKey );
	void ParseKeyLate();

public:
	CScriptKeyAlloc() { }
	virtual ~CScriptKeyAlloc() { }

private:
	CScriptKeyAlloc(const CScriptKeyAlloc& copy);
	CScriptKeyAlloc& operator=(const CScriptKeyAlloc& other);
};

#ifdef _NOSCRIPTCACHE
 #define PhysicalScriptFile CFileText
#else
 #define PhysicalScriptFile CacheableScriptFile
#endif

class CScript : public PhysicalScriptFile, public CScriptKeyAlloc
{
private:
	bool m_fSectionHead;	// File Offset to current section header. [HEADER]
	long m_lSectionData;	// File Offset to current section data, under section header.

public:
	static const char *m_sClassName;
	int m_iLineNum;		// for debug purposes if there is an error.
protected:
	void InitBase();

	virtual DWORD Seek( long offset = 0, UINT origin = SEEK_SET );

public:
	// text only functions:
	virtual bool ReadTextLine( bool fRemoveBlanks );	// looking for a section or reading strangly formated section.
	bool FindTextHeader( LPCTSTR pszName ); // Find a section in the current script

public:
	virtual bool Open( LPCTSTR szFilename = NULL, UINT Flags = OF_READ|OF_TEXT );
	virtual void Close();
	virtual void CloseForce()
	{
		CScript::Close();
	}

	bool SeekContext( CScriptLineContext LineContext )
	{
		m_iLineNum = LineContext.m_iLineNum;
		return Seek( LineContext.m_lOffset, SEEK_SET ) == static_cast<DWORD>(LineContext.m_lOffset);
	}
	CScriptLineContext GetContext() const
	{
		CScriptLineContext LineContext;
		LineContext.m_iLineNum = m_iLineNum;
		LineContext.m_lOffset = GetPosition();
		return( LineContext );
	}

	// Find sections.
	bool FindNextSection();
	virtual bool FindSection( LPCTSTR pszName, UINT uModeFlags ); // Find a section in the current script
	LPCTSTR GetSection() const
	{
		ASSERT(m_pszKey);
		return( m_pszKey );
	}
	bool IsSectionType( LPCTSTR pszName ) //const
	{
		// Only valid after FindNextSection()
		return( ! strcmpi( GetKey(), pszName ));
	}

	// Find specific keys in the current section.
	bool FindKey( LPCTSTR pszName ); // Find a key in the current section

	// read the sections keys.
	bool ReadKey( bool fRemoveBlanks = true );
	bool ReadKeyParse();

	// Write stuff out to a script file.
	bool _cdecl WriteSection( LPCTSTR pszSection, ... ) __printfargs(2,3);
	bool WriteKey( LPCTSTR pszKey, LPCTSTR pszVal );
	void _cdecl WriteKeyFormat( LPCTSTR pszKey, LPCTSTR pszFormat, ... ) __printfargs(3,4);

	void WriteKeyVal( LPCTSTR pszKey, INT64 dwVal )
	{
		WriteKeyFormat( pszKey, "%lld", dwVal );
	}
	void WriteKeyHex( LPCTSTR pszKey, INT64 dwVal )
	{
		WriteKeyFormat( pszKey, "0%llx", dwVal );
	}

	CScript();
	CScript( LPCTSTR pszKey );
	CScript( LPCTSTR pszKey, LPCTSTR pszVal );
	virtual ~CScript()
	{
	}
};

#endif // _INC_CSCRIPT_H
