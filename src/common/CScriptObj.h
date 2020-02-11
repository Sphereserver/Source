#ifndef _INC_CSCRIPTOBJ_H
#define _INC_CSCRIPTOBJ_H
#pragma once

#include "CArray.h"
#include "CVarDefMap.h"
#include "CVarFloat.h"

enum TRIGRUN_TYPE
{
	TRIGRUN_SECTION_EXEC,	// Execute this section (first line already read)
	TRIGRUN_SECTION_TRUE,	// Execute this section
	TRIGRUN_SECTION_FALSE,	// Ignore this section
	TRIGRUN_SINGLE_EXEC,	// Execute just this line or blocked segment (first line already read)
	TRIGRUN_SINGLE_TRUE,	// Execute just this line or blocked segment
	TRIGRUN_SINGLE_FALSE	// Ignore just this line or blocked segment
};

enum TRIGRET_TYPE
{
	TRIGRET_RET_FALSE,		// Default return (script might not have been handled)
	TRIGRET_RET_TRUE,
	TRIGRET_RET_DEFAULT,	// Reached the end of the script
	TRIGRET_ENDIF,
	TRIGRET_ELSE,
	TRIGRET_ELSEIF,
	TRIGRET_RET_HALFBAKED,
	TRIGRET_BREAK,
	TRIGRET_CONTINUE
};

enum PLEVEL_TYPE
{
	PLEVEL_Player = 1,
	PLEVEL_Counsel,
	PLEVEL_Seer,
	PLEVEL_GM,
	PLEVEL_Dev,
	PLEVEL_Admin,
	PLEVEL_Owner,
	PLEVEL_QTY
};

class CChar;

class CTextConsole
{
	// A base class for any class that can act like a console and issue commands
	// CClient, CChar, CServer, CFileConsole
public:
	static const char *m_sClassName;

	CTextConsole() { };
	virtual ~CTextConsole() { };

public:
	virtual PLEVEL_TYPE GetPrivLevel() const = 0;
	virtual LPCTSTR GetName() const = 0;	// every object must have at least a type name
	virtual CChar *GetChar() const;			// are we also a CChar? dynamic_cast?

	virtual void SysMessage(LPCTSTR pszMessage) const = 0;	// feedback message
	void VSysMessage(LPCTSTR pszFormat, va_list args) const
	{
		TemporaryString pszTemp;
		_vsnprintf(pszTemp, pszTemp.realLength(), pszFormat, args);
		SysMessage(pszTemp);
	}
	void _cdecl SysMessagef(LPCTSTR pszFormat, ...) const __printfargs(2, 3)
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		VSysMessage(pszFormat, vargs);
		va_end(vargs);
	}

protected:
	int OnConsoleKey(CGString &sText, TCHAR szChar, bool fEcho);

private:
	CTextConsole(const CTextConsole &copy);
	CTextConsole &operator=(const CTextConsole &other);
};

class CScriptTriggerArgs;

class CScriptObj
{
	// This object can be scripted (but might not be)
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szScriptKeys[];
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CScriptObj() { };
	virtual ~CScriptObj() { };

private:
	TRIGRET_TYPE OnTriggerForLoop(CScript &s, int iType, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *psResult);

public:
	TRIGRET_TYPE OnTriggerScript(CScript &s, LPCTSTR pszTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL);
	virtual TRIGRET_TYPE OnTrigger(LPCTSTR pszTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL)
	{
		UNREFERENCED_PARAMETER(pszTrigName);
		UNREFERENCED_PARAMETER(pSrc);
		UNREFERENCED_PARAMETER(pArgs);
		return TRIGRET_RET_DEFAULT;
	}
	bool OnTriggerFind(CScript &s, LPCTSTR pszTrigName);
	TRIGRET_TYPE OnTriggerRun(CScript &s, TRIGRUN_TYPE trigger, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL, CGString *psResult = NULL);
	TRIGRET_TYPE OnTriggerRunVal(CScript &s, TRIGRUN_TYPE trigger, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL);

	size_t ParseText(TCHAR *pszResponse, CTextConsole *pSrc, int iFlags = 0, CScriptTriggerArgs *pArgs = NULL);

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_Load(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// execute command from script

	bool r_Call(LPCTSTR pszFunction, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL, CGString *psVal = NULL, TRIGRET_TYPE *piRet = NULL);
	bool r_SetVal(LPCTSTR pszKey, LPCTSTR pszVal)
	{
		CScript s(pszKey, pszVal);
		return r_LoadVal(s);
	}

	virtual LPCTSTR GetName() const = 0;	// every object must have at least a type name

private:
	CScriptObj(const CScriptObj &copy);
	CScriptObj &operator=(const CScriptObj &other);
};

class CScriptTriggerArgs : public CScriptObj
{
	// All the args an event will need
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];

	CScriptTriggerArgs() : m_iN1(0), m_iN2(0), m_iN3(0)
	{
		m_pO1 = NULL;
	}
	explicit CScriptTriggerArgs(LPCTSTR pszStr);
	explicit CScriptTriggerArgs(CScriptObj *pObj) : m_iN1(0), m_iN2(0), m_iN3(0), m_pO1(pObj)
	{
	}
	explicit CScriptTriggerArgs(INT64 iVal1) : m_iN1(iVal1), m_iN2(0), m_iN3(0)
	{
		m_pO1 = NULL;
	}
	CScriptTriggerArgs(INT64 iVal1, INT64 iVal2, INT64 iVal3 = 0) : m_iN1(iVal1), m_iN2(iVal2), m_iN3(iVal3)
	{
		m_pO1 = NULL;
	}
	CScriptTriggerArgs(INT64 iVal1, INT64 iVal2, CScriptObj *pObj) : m_iN1(iVal1), m_iN2(iVal2), m_iN3(0), m_pO1(pObj)
	{
	}
	virtual ~CScriptTriggerArgs() { };

public:
	INT64 m_iN1;		// ARGN1 or ARGN = a modifying numeric arg to the current trigger
	INT64 m_iN2;		// ARGN2 = a modifying numeric arg to the current trigger
	INT64 m_iN3;		// ARGN3 = a modifying numeric arg to the current trigger

	CScriptObj *m_pO1;	// ARGO1 or ARGO = object 1

	// These can go out of date ! get deleted etc.
	CGString m_s1;		// ARGS1 or ARGS = string 1
	CGString m_s1_raw;	// RAW, used to build argv in runtime

	CGPtrTypeArray<LPCTSTR> m_v;

	CVarDefMap m_VarsLocal;		// LOCAL.x = local variable x
	CVarFloat m_VarsFloat;		// FLOAT.x = float local variable x
	CLocalObjMap m_VarObjs;		// REFx = local object x

public:
	void Init(LPCTSTR pszStr);

	bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	bool r_LoadVal(CScript &s);
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_Verb(CScript &s, CTextConsole *pSrc);

	void getArgNs(INT64 *piVar1 = NULL, INT64 *piVar2 = NULL, INT64 *piVar3 = NULL)		// puts ARGN's into the specified variables
	{
		if ( piVar1 )
			*piVar1 = m_iN1;
		if ( piVar2 )
			*piVar2 = m_iN2;
		if ( piVar3 )
			*piVar3 = m_iN3;
	}

	LPCTSTR GetName() const
	{
		return "ARG";
	}

private:
	CScriptTriggerArgs(const CScriptTriggerArgs &copy);
	CScriptTriggerArgs &operator=(const CScriptTriggerArgs &other);
};

class CFileObj : public CScriptObj
{
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CFileObj();
	~CFileObj();

private:
	CFileText *m_pFile;
	bool m_fAppend;
	bool m_fCreate;
	bool m_fRead;
	bool m_fWrite;
	TCHAR *m_pszBuffer;
	CGString *m_psWriteBuffer;

private:
	void SetDefaultMode();
	bool FileOpen(LPCTSTR pszPath);
	TCHAR *GetReadBuffer(bool fDelete = false);
	CGString *GetWriteBuffer();

public:
	bool OnTick();
	int FixWeirdness();
	bool IsInUse();
	void FlushAndClose();

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	LPCTSTR GetName() const
	{
		return "FILE_OBJ";
	}

private:
	CFileObj(const CFileObj &copy);
	CFileObj &operator=(const CFileObj &other);
};

class CFileObjContainer : public CScriptObj
{
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];
	static const LPCTSTR sm_szVerbKeys[];

	CFileObjContainer();
	~CFileObjContainer();

private:
	std::vector<CFileObj *> m_FileList;
	int m_iFileNumber;
	int m_iGlobalTimeout;
	int m_iCurrentTick;

private:
	void ResizeContainer(size_t iNewRange);

public:
	int GetFileNumber();
	void SetFileNumber(int iNewRange);

public:
	bool OnTick();
	int FixWeirdness();

	virtual bool r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);

	LPCTSTR GetName() const
	{
		return "FILE_OBJCONTAINER";
	}

private:
	CFileObjContainer(const CFileObjContainer &copy);
	CFileObjContainer &operator=(const CFileObjContainer &other);
};

///////////////////////////////////////////////////////////

int FindTable(LPCTSTR pszFind, const LPCTSTR *ppszTable, int iCount);
int FindTableSorted(LPCTSTR pszFind, const LPCTSTR *ppszTable, int iCount);
int FindTableHead(LPCTSTR pszFind, const LPCTSTR *ppszTable, int iCount);
int FindTableHeadSorted(LPCTSTR pszFind, const LPCTSTR *ppszTable, int iCount);
int FindTableHeadSortedRes(LPCTSTR pszFind, const LPCTSTR *ppszTable, int iCount);

#endif	// _INC_CSCRIPTOBJ_H
