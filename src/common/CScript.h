#ifndef _INC_CSCRIPT_H
#define _INC_CSCRIPT_H
#pragma once

#include "CMemBlock.h"
#include "CCacheableScriptFile.h"

struct CScriptLineContext
{
public:
	CScriptLineContext()
	{
		Init();
	}

	long m_lOffset;
	int m_iLineNum;		// for debug purposes if there is an error

	void Init()
	{
		m_lOffset = -1;
		m_iLineNum = -1;
	}
	bool IsValid() const
	{
		return (m_lOffset >= 0);
	}
};

class CScriptKey
{
	// A single line form a script file
	// This is usually in the form of KEY=ARG
	// Unknown allocation of pointers
public:
	CScriptKey() : m_pszKey(NULL), m_pszArg(NULL) { }
	CScriptKey(TCHAR *pszKey, TCHAR *pszArg) : m_pszKey(pszKey), m_pszArg(pszArg) { }
	virtual ~CScriptKey() { }

private:
	CScriptKey(const CScriptKey &copy);
	CScriptKey &operator=(const CScriptKey &other);

protected:
	TCHAR *m_pszKey;		// the key (or just start of the line)
	TCHAR *m_pszArg;		// for parsing the last read line (KEY=ARG or KEY ARG)

public:
	static const char *m_sClassName;
	bool IsKey(LPCTSTR pszName) const
	{
		ASSERT(m_pszKey);
		return !strcmpi(m_pszKey, pszName);
	}
	bool IsKeyHead(LPCTSTR pszName, size_t len) const
	{
		ASSERT(m_pszKey);
		return !strnicmp(m_pszKey, pszName, len);
	}

	void InitKey();

	LPCTSTR GetKey() const
	{
		ASSERT(m_pszKey);
		return m_pszKey;
	}

	bool HasArgs() const
	{
		ASSERT(m_pszArg);
		return m_pszArg[0] ? true : false;
	}

	TCHAR *GetArgRaw() const
	{
		ASSERT(m_pszArg);
		return m_pszArg;
	}
	TCHAR *GetArgStr(bool *fQuoted);
	TCHAR *GetArgStr()
	{
		return GetArgStr(NULL);
	}
	UINT64 GetArgFlag(UINT64 uiStart, UINT64 uiMask);
	long long GetArgLLVal();
	long GetArgVal();
	long GetArgRange();
};

class CScriptKeyAlloc : public CScriptKey
{
	// Dynamic allocated script key
public:
	CScriptKeyAlloc() { }
	virtual ~CScriptKeyAlloc() { }

private:
	CScriptKeyAlloc(const CScriptKeyAlloc &copy);
	CScriptKeyAlloc &operator=(const CScriptKeyAlloc &other);

protected:
	CMemLenBlock m_Mem;		// the buffer to hold data read

	TCHAR *GetKeyBufferRaw(size_t iLen);
	bool ParseKey(LPCTSTR pszKey, LPCTSTR pszVal);
	size_t ParseKeyEnd();

public:
	static const char *m_sClassName;
	TCHAR *GetKeyBuffer()
	{
		ASSERT(m_Mem.GetData());
		return reinterpret_cast<TCHAR *>(m_Mem.GetData());
	}
	bool ParseKey(LPCTSTR pszKey);
	void ParseKeyLate();
};

class CScript : public CCacheableScriptFile, public CScriptKeyAlloc
{
public:
	CScript();
	CScript(LPCTSTR pszKey);
	CScript(LPCTSTR pszKey, LPCTSTR pszVal);
	virtual ~CScript() { }

private:
	bool m_fSectionHead;	// file offset to current section header [HEADER]
	long m_lSectionData;	// file offset to current section data, under section header

public:
	static const char *m_sClassName;
	int m_iLineNum;		// for debug purposes if there is an error

protected:
	void InitBase();
	virtual DWORD Seek(long lOffset = 0, UINT uiOrigin = SEEK_SET);

public:
	// Text only functions
	virtual bool ReadTextLine(bool fRemoveBlanks);
	bool FindTextHeader(LPCTSTR pszName);

public:
	virtual bool Open(LPCTSTR pszFilename = NULL, UINT uiFlags = OF_READ|OF_TEXT);
	virtual void Close();
	virtual void CloseForce()
	{
		CScript::Close();
	}

	bool SeekContext(CScriptLineContext LineContext)
	{
		m_iLineNum = LineContext.m_iLineNum;
		return (Seek(LineContext.m_lOffset, SEEK_SET) == static_cast<DWORD>(LineContext.m_lOffset));
	}
	CScriptLineContext GetContext() const
	{
		CScriptLineContext LineContext;
		LineContext.m_iLineNum = m_iLineNum;
		LineContext.m_lOffset = GetPosition();
		return LineContext;
	}

	// Find sections
	bool FindNextSection();
	LPCTSTR GetSection() const
	{
		ASSERT(m_pszKey);
		return m_pszKey;
	}
	bool IsSectionType(LPCTSTR pszName) //const
	{
		// Only valid after FindNextSection()
		return !strcmpi(GetKey(), pszName);
	}

	// Read the sections keys
	bool ReadKey(bool fRemoveBlanks = true);
	bool ReadKeyParse();

	// Write stuff out to a script file
	bool _cdecl WriteSection(LPCTSTR pszSection, ...) __printfargs(2, 3);
	bool WriteKey(LPCTSTR pszKey, LPCTSTR pszVal);
	void _cdecl WriteKeyFormat(LPCTSTR pszKey, LPCTSTR pszVal, ...) __printfargs(3, 4);

	void WriteKeyVal(LPCTSTR pszKey, INT64 iVal)
	{
#ifdef __MINGW32__
		WriteKeyFormat(pszKey, "%I64d", iVal);
#else
		WriteKeyFormat(pszKey, "%lld", iVal);
#endif
	}
	void WriteKeyHex(LPCTSTR pszKey, INT64 iVal)
	{
#ifdef __MINGW32__
		WriteKeyFormat(pszKey, "0%I64x", iVal);
#else
		WriteKeyFormat(pszKey, "0%llx", iVal);
#endif
	}
};

#endif // _INC_CSCRIPT_H
