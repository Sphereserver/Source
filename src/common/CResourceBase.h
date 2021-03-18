#ifndef _INC_CRESOURCEBASE_H
#define _INC_CRESOURCEBASE_H
#pragma once

enum RES_TYPE	// all the script resource blocks we know how to deal with
{
	// NOTE: sphere.ini and spheretables.scp are read at start
	// All other files are indexed from ScpFiles directories
	// (SI) = Single instance types
	// (SL) = single line multiple definitions
	RES_UNKNOWN,		// Not to be used
	RES_ACCOUNT,		// Define an account instance
	RES_ADVANCE,		// Define the advance rates for stats
	RES_AREA,			// Complex region (w/ extra tags)
	RES_BLOCKIP,		// (SL) List of IP's to block
	RES_BOOK,			// A book or a page from a book
	RES_CHARDEF,		// Define a char type
	RES_COMMENT,		// A commented out block type
	RES_DEFNAME,		// (SL) Just add a bunch of new defs and equivs str/values
	RES_DIALOG,			// A scriptable gump dialog, text or handler block
	RES_EVENTS,			// (SL) Preload these event files
	RES_FAME,
	RES_FUNCTION,		// Define a new command verb script that applies to a char
	RES_GMPAGE,			// A GM page (SAVED in World)
	RES_ITEMDEF,		// Define an item type
	RES_KARMA,
	RES_KRDIALOGLIST,	// List of KR dialog ID's
	RES_MENU,			// General scriptable menus
	RES_MOONGATES,		// (SL) Define where the moongates are
	RES_NAMES,			// A block of possible names for a NPC type (read as needed)
	RES_NEWBIE,			// Triggers to execute on player creation (based on skills selected)
	RES_NOTOTITLES,		// (SI) Define the noto titles used
	RES_OBSCENE,		// (SL) A list of obscene words
	RES_PLEVEL,			// Define the list of commands that a PLEVEL can access (or not access)
	RES_REGIONRESOURCE,	// Define ore types
	RES_REGIONTYPE,		// Triggers etc. that can be assinged to a RES_AREA
	RES_RESOURCELIST,
	RES_RESOURCES,		// (SL) List of all the resource files to index
	RES_ROOM,			// Non-complex region (no extra tags)
	RES_RUNES,			// (SI) Define list of the magic runes
	RES_SCROLL,			// Message scroll sent to player at login (SCROLL_GUEST, SCROLL_MOTD, SCROLL_NEWBIE)
	RES_SECTOR,			// Make changes to a sector (SAVED in World)
	RES_SERVERS,		// List a number of servers in 3 line format
	RES_SKILL,			// Define attributes for a skill (how fast it raises, etc)
	RES_SKILLCLASS,		// Define class specifics for a char with this skill class
	RES_SKILLMENU,		// A menu that is attached to a skill. Special arguments over other menus
	RES_SPAWN,			// Define a list of NPC's and how often they may spawn
	RES_SPEECH,			// (SL) Preload these speech files
	RES_SPELL,			// Define a magic spell (0-64 are reserved)
	RES_SPHERE,			// Main server INI block
	RES_SPHERECRYPT,	// Encryption keys
	RES_STARTS,			// (SI) List of starting locations on account character creation
	RES_STAT,			// Stats elements like KARMA/STR/DEX/FOOD/FAME/etc. Used for resource and desire scripts
	RES_TELEPORTERS,	// (SL) List of teleporters in the world
	RES_TEMPLATE,		// Define a list of items (for filling loot, etc)
	RES_TIMERF,
	RES_TIP,			// Tips that can come up at startup
	RES_TYPEDEF,		// Define a trigger block for a RES_WORLDITEM m_type
	RES_TYPEDEFS,
	RES_WC,				// =WORLDCHAR
	RES_WEBPAGE,		// Define a web page template
	RES_WI,				// =WORLDITEM
	RES_WORLDCHAR,		// Define instance of char in the world (SAVED in World)
	RES_WORLDITEM,		// Define instance of item in the world (SAVED in World)
	RES_WORLDLISTS,		// Define instance of list in the world (SAVED in World)
	RES_WORLDSCRIPT,	// Define instance of resource in the world (SAVED in World)
	RES_WORLDVARS,		// Block of global variables
	RES_WS,				// =WORLDSCRIPT
	RES_QTY
};

#define RES_DIALOG_TEXT				1	// sub page for the section
#define RES_DIALOG_BUTTON			2

#define RES_NEWBIE_MALE_DEFAULT		10001	// just an unused number for the range
#define RES_NEWBIE_FEMALE_DEFAULT	10002
#define RES_NEWBIE_PROF_ADVANCED	10003
#define RES_NEWBIE_PROF_WARRIOR		10004
#define RES_NEWBIE_PROF_MAGE		10005
#define RES_NEWBIE_PROF_BLACKSMITH	10006
#define RES_NEWBIE_PROF_NECROMANCER	10007
#define RES_NEWBIE_PROF_PALADIN		10008
#define RES_NEWBIE_PROF_SAMURAI		10009
#define RES_NEWBIE_PROF_NINJA		10010

struct RESOURCE_ID_BASE : public CGrayUIDBase
{
#define RES_INDEX_SHIFT		0	// leave 18 bits = 262144 entries
#define RES_INDEX_MASK		0x3FFFF
#define RES_GET_INDEX(dw)	((dw) & RES_INDEX_MASK)

#define RES_TYPE_SHIFT		25	// leave 6 bits = 64 for RES_TYPE
#define RES_TYPE_MASK		63
#define RES_GET_TYPE(dw)	((dw >> RES_TYPE_SHIFT) & RES_TYPE_MASK)

#define RES_PAGE_SHIFT		18	// leave 7 bits = 128 pages of space
#define RES_PAGE_MASK		127
#define RES_GET_PAGE(dw)	((dw >> RES_PAGE_SHIFT) & RES_PAGE_MASK)

public:
	int GetResIndex() const
	{
		return RES_GET_INDEX(m_dwInternalVal);
	}

	RES_TYPE GetResType() const
	{
		return static_cast<RES_TYPE>(RES_GET_TYPE(m_dwInternalVal));
	}

	int GetResPage() const
	{
		return RES_GET_PAGE(m_dwInternalVal);
	}

	bool operator==(const RESOURCE_ID_BASE &rid) const
	{
		return (rid.m_dwInternalVal == m_dwInternalVal);
	}
};

struct RESOURCE_ID : public RESOURCE_ID_BASE
{
	RESOURCE_ID()
	{
		InitUID();
	}
	RESOURCE_ID(RES_TYPE restype)
	{
		// Single instance type
		m_dwInternalVal = UID_F_RESOURCE|((restype) << RES_TYPE_SHIFT);
	}
	RESOURCE_ID(RES_TYPE restype, int index)
	{
		ASSERT(index < RES_INDEX_MASK);
		m_dwInternalVal = UID_F_RESOURCE|((restype) << RES_TYPE_SHIFT)|(index);
	}
	RESOURCE_ID(RES_TYPE restype, int index, int iPage)
	{
		ASSERT(index < RES_INDEX_MASK);
		ASSERT(iPage < RES_PAGE_MASK);
		m_dwInternalVal = UID_F_RESOURCE|((restype) << RES_TYPE_SHIFT)|((iPage) << RES_PAGE_SHIFT)|(index);
	}

	RESOURCE_ID_BASE &operator=(const RESOURCE_ID_BASE &rid)
	{
		ASSERT(rid.IsValidUID());
		ASSERT(rid.IsResource());
		m_dwInternalVal = rid.GetPrivateUID();
		return *this;
	}
};

// Desguise an id as a pointer
#ifndef MAKEINTRESOURCE
	#define MAKEINTRESOURCE(i) ((LPCTSTR)((ULONG_PTR)((WORD)(i))))
#endif
#define ISINTRESOURCE(i)	(!(((ULONG_PTR)(i)) & 0xFFFFF000))
#define GETINTRESOURCE(i)	(((ULONG_PTR)(i)) & 0xFFF)

//*********************************************************

struct CResourceQty
{
public:
	CResourceQty()
	{
		m_iQty = 0;
	}

private:
	RESOURCE_ID m_rid;		// RES_SKILL, RES_ITEMDEF, or RES_TYPEDEF
	int m_iQty;

public:
	RESOURCE_ID GetResourceID() const
	{
		return m_rid;
	}
	void SetResourceID(RESOURCE_ID rid, int iQty)
	{
		m_rid = rid;
		m_iQty = iQty;
	}

	int GetResQty() const
	{
		return m_iQty;
	}
	void SetResQty(int iQty)
	{
		m_iQty = iQty;
	}

	RES_TYPE GetResType() const
	{
		return m_rid.GetResType();
	}
	int GetResIndex() const
	{
		return m_rid.GetResIndex();
	}

	inline bool Load(LPTSTR &pszArgs)
	{
		return Load(const_cast<LPCTSTR &>(pszArgs));
	}

	size_t WriteKey(TCHAR *pszArgs, bool fQtyOnly = false, bool fKeyOnly = false) const;
	size_t WriteNameSingle(TCHAR *pszArgs, int iQty = 0) const;
	bool Load(LPCTSTR &pszCmds);
};

class CResourceQtyArray : public CGTypedArray<CResourceQty, CResourceQty &>
{
	// Define a list of index id's (not references) to resource objects (not owned by the list)
public:
	static const char *m_sClassName;

	CResourceQtyArray();
	explicit CResourceQtyArray(LPCTSTR pszCmds);

private:
	CResourceQtyArray(const CResourceQtyArray &copy);
	bool m_mergeOnLoad;

public:
	void setNoMergeOnLoad();

	size_t FindResourceType(RES_TYPE restype) const;
	size_t FindResourceID(RESOURCE_ID_BASE rid) const;
	size_t FindResourceMatch(CObjBase *pObj) const;
	bool IsResourceMatchAll(CChar *pChar) const;

	size_t Load(LPCTSTR pszCmds);
	void WriteKeys(TCHAR *pszArgs, size_t index = 0, bool fQtyOnly = false, bool fKeyOnly = false) const;
	void WriteNames(TCHAR *pszArgs, size_t index = 0) const;

	inline bool ContainsResourceID(RESOURCE_ID_BASE &rid) const
	{
		return (FindResourceID(rid) != BadIndex());
	}
	inline bool ContainsResourceMatch(CObjBase *pObj) const
	{
		return (FindResourceMatch(pObj) != BadIndex());
	}

	bool operator==(const CResourceQtyArray &copy) const;
	CResourceQtyArray &operator=(const CResourceQtyArray &other);
};

//*********************************************************

class CScriptFileContext
{
	// Track a temporary context into a script
	// NOTE: This should ONLY be stack based
public:
	static const char *m_sClassName;

	CScriptFileContext() : m_pPrvScriptContext(NULL)
	{
		Init();
	}
	explicit CScriptFileContext(const CScript *pScriptContext)
	{
		Init();
		OpenScript(pScriptContext);
	}
	~CScriptFileContext()
	{
		Close();
	}

	void OpenScript(const CScript *pScriptContext);
	void Close();

private:
	bool m_fOpenScript;		// NULL context may be legit
	const CScript *m_pPrvScriptContext;		// previous general context before this was opened

	void Init()
	{
		m_fOpenScript = false;
	}

	CScriptFileContext(const CScriptFileContext &copy);
	CScriptFileContext &operator=(const CScriptFileContext &other);
};

class CScriptObjectContext
{
	// Track a temporary context of an object
	// NOTE: This should ONLY be stack based
public:
	static const char *m_sClassName;

	CScriptObjectContext() : m_pPrvObjectContext(NULL)
	{
		Init();
	}
	explicit CScriptObjectContext(const CScriptObj *pObjectContext)
	{
		Init();
		OpenObject(pObjectContext);
	}
	~CScriptObjectContext()
	{
		Close();
	}

	void OpenObject(const CScriptObj *pObjectContext);
	void Close();

private:
	bool m_fOpenObject;		// NULL context may be legit
	const CScriptObj *m_pPrvObjectContext;	// previous general context before this was opened

	void Init()
	{
		m_fOpenObject = false;
	}

	CScriptObjectContext(const CScriptObjectContext &copy);
	CScriptObjectContext &operator=(const CScriptObjectContext &other);
};

//*********************************************************

class CResourceScript : public CScript
{
	// A script file containing resource, speech, motives or events handlers
public:
	static const char *m_sClassName;

	CResourceScript()
	{
		Init();
	}
	explicit CResourceScript(LPCTSTR pszFileName)
	{
		Init();
		SetFilePath(pszFileName);
	}

private:
	int m_iOpenCount;			// how many CResourceLock(s) have this open?
	CServTime m_timeLastAccess;	// CWorld time of last access

	// Last time it was closed. What did the file params look like?
	DWORD m_dwSize;				// compare to check if it was changed
	CGTime m_dateChange;		// real world time/date of last change

	void Init()
	{
		m_iOpenCount = 0;
		m_timeLastAccess.Init();
		m_dwSize = DWORD_MAX;
	}

	bool CheckForChange();

public:
	bool IsFirstCheck() const
	{
		return ((m_dwSize == DWORD_MAX) && !m_dateChange.IsTimeValid());
	}

	void ReSync();
	bool Open(LPCTSTR pszFileName = NULL, UINT uFlags = OF_READ);
	virtual void CloseForce();
	virtual void Close();

private:
	CResourceScript(const CResourceScript &copy);
	CResourceScript &operator=(const CResourceScript &other);
};

class CResourceLock : public CScript
{
	// Open a copy of a scipt that is already open
	// NOTE: This should ONLY be stack based. Preserve the previous openers offset in the script
public:
	static const char *m_sClassName;

	CResourceLock()
	{
		Init();
	}
	~CResourceLock()
	{
		Close();
	}

private:
	CResourceScript *m_pLock;
	CScriptLineContext m_PrvLockContext;		// I must return the locked file back here

	CScriptFileContext m_PrvScriptContext;		// where I was before (context wise) opening this (for error tracking)
	CScriptObjectContext m_PrvObjectContext;	// object context (for error tracking)

	void Init()
	{
		m_pLock = NULL;
		m_PrvLockContext.Init();	// means the script was NOT open when we started
	}

protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	virtual bool ReadTextLine(bool fRemoveBlanks);

public:
	int OpenLock(CResourceScript *pLock, CScriptLineContext context);

	void AttachObj(const CScriptObj *pObj)
	{
		m_PrvObjectContext.OpenObject(pObj);
	}

private:
	CResourceLock(const CResourceLock &copy);
	CResourceLock &operator=(const CResourceLock &other);
};

class CResourceDef : public CScriptObj
{
	// Define a generic resource block in the scripts
	// Now the scripts can be modular. Resources can be defined any place
	// NOTE: This may be loaded fully into memory or just an index to a file
public:
	static const char *m_sClassName;

	CResourceDef(RESOURCE_ID rid, LPCTSTR pszDefName) : m_rid(rid), m_pDefName(NULL)
	{
		SetResourceName(pszDefName);
	}
	CResourceDef(RESOURCE_ID rid, const CVarDefContNum *pDefName = NULL) : m_rid(rid), m_pDefName(pDefName) { };
	virtual ~CResourceDef() { };	// need a virtual to make dynamic_cast work

private:
	RESOURCE_ID m_rid;		// true resource id (must be unique for the RES_TYPE)

protected:
	const CVarDefContNum *m_pDefName;	// name of the resource (optional)

public:
	RESOURCE_ID GetResourceID() const
	{
		return m_rid;
	}
	RES_TYPE GetResType() const
	{
		return m_rid.GetResType();
	}
	int GetResPage() const
	{
		return m_rid.GetResPage();
	}

	void CopyDef(const CResourceDef *pLink)
	{
		m_pDefName = pLink->m_pDefName;
	}

	// Give it another DEFNAME= even if it already has one (it's ok to have multiple names)
	bool SetResourceName(LPCTSTR pszName);
	void SetResourceVar(const CVarDefContNum *pVarNum)
	{
		if ( pVarNum && !m_pDefName )
			m_pDefName = pVarNum;
	}

	// Get the name of the resource item. Used for saving (may be number or name)
	LPCTSTR GetResourceName() const;

	virtual LPCTSTR GetName() const		// default to same as the DEFNAME name
	{
		return GetResourceName();
	}

	inline bool HasResourceName() const
	{
		return (m_pDefName != NULL);
	}

	// Unlink all this data (don't delete the def as the pointer might still be used)
	virtual void UnLink()
	{
		// This does nothing in the CResourceDef case, only in the CResourceLink case
	}

private:
	CResourceDef(const CResourceDef &copy);
	CResourceDef &operator=(const CResourceDef &other);
};

class CResourceLink : public CResourceDef
{
	// A single resource object that also has part of itself remain in resource file
	// A pre-indexed link into a script file
	// This is a CResourceDef not fully read into memory at index time
	// We are able to lock it and read it as needed
#define MAX_TRIGGERS_ARRAY	5
#define XTRIG_UNKNOWN		0	// bit 0 is reserved to say there are triggers here that do not conform

public:
	static const char *m_sClassName;

	CResourceLink(RESOURCE_ID rid, const CVarDefContNum *pDef = NULL);
	virtual ~CResourceLink() { };

private:
	CResourceScript *m_pScript;	// we already found the script
	CScriptLineContext m_Context;
	DWORD m_dwRefInstances;		// how many CResourceRef objects refer to this

public:
	DWORD m_dwOnTriggers[MAX_TRIGGERS_ARRAY];

	void AddRefInstance()
	{
		++m_dwRefInstances;
	}
	void DelRefInstance()
	{
#ifdef _DEBUG
		ASSERT(m_dwRefInstances > 0);
#endif
		--m_dwRefInstances;
	}
	DWORD GetRefInstances() const
	{
		return m_dwRefInstances;
	}

	void ScanSection(RES_TYPE restype);
	bool IsLinked() const;
	void SetLink(CResourceScript *pScript);
	void CopyTransfer(CResourceLink *pLink);
	void ClearTriggers();
	void SetTrigger(int i);
	bool HasTrigger(int i) const;
	bool ResourceLock(CResourceLock &s);

private:
	CResourceLink(const CResourceLink &copy);
	CResourceLink &operator=(const CResourceLink &other);
};

class CResourceNamed : public CResourceLink
{
	// Private name pool (does not use DEFNAME) RES_FUNCTION
public:
	static const char *m_sClassName;
	const CGString m_sName;

	CResourceNamed(RESOURCE_ID rid, LPCTSTR pszName) : CResourceLink(rid), m_sName(pszName) { };
	virtual ~CResourceNamed() { };

public:
	LPCTSTR GetName() const
	{
		return m_sName;
	}

private:
	CResourceNamed(const CResourceNamed &copy);
	CResourceNamed &operator=(const CResourceNamed &other);
};

//*********************************************************

class CResourceRef
{
public:
	static const char *m_sClassName;

	CResourceRef()
	{
		m_pLink = NULL;
	}
	CResourceRef(CResourceLink *pLink) : m_pLink(pLink)
	{
		ASSERT(pLink);
		pLink->AddRefInstance();
	}
	CResourceRef(const CResourceRef &copy)
	{
		m_pLink = copy.m_pLink;
		if ( m_pLink )
			m_pLink->AddRefInstance();
	}
	~CResourceRef()
	{
		if ( m_pLink )
			m_pLink->DelRefInstance();
	}

private:
	CResourceLink *m_pLink;

public:
	CResourceLink *GetRef() const
	{
		return m_pLink;
	}
	void SetRef(CResourceLink *pLink)
	{
		if ( m_pLink )
			m_pLink->DelRefInstance();

		m_pLink = pLink;

		if ( pLink )
			pLink->AddRefInstance();
	}

	operator CResourceLink *() const
	{
		return GetRef();
	}
	CResourceRef &operator=(const CResourceRef &other)
	{
		if ( this != &other )
			SetRef(other.m_pLink);
		return *this;
	}
};

class CResourceRefArray : public CGPtrTypeArray<CResourceRef>
{
	// Define a list of pointer references to resource (not owned by the list)
	// An indexed list of CResourceLink
public:
	static const char *m_sClassName;

	CResourceRefArray() { };

private:
	LPCTSTR GetResourceName(size_t index) const
	{
		// Look up the name of the fragment given it's index
		CResourceLink *pResourceLink = GetAt(index);
		ASSERT(pResourceLink);
		return pResourceLink->GetResourceName();
	}

public:
	bool r_LoadVal(CScript &s, RES_TYPE restype);
	void r_Write(CScript &s, LPCTSTR pszKey) const;
	void WriteResourceRefList(CGString &sVal) const;
	size_t FindResourceType(RES_TYPE restype) const;
	size_t FindResourceID(RESOURCE_ID_BASE rid) const;
	size_t FindResourceName(RES_TYPE restype, LPCTSTR pszKey) const;

	inline bool ContainsResourceID(RESOURCE_ID_BASE &rid) const
	{
		return FindResourceID(rid) != BadIndex();
	}
	inline bool ContainsResourceName(RES_TYPE restype, LPCTSTR &pszKey) const
	{
		return FindResourceName(restype, pszKey) != BadIndex();
	}

private:
	CResourceRefArray(const CResourceRefArray &copy);
	CResourceRefArray &operator=(const CResourceRefArray &other);
};

//*********************************************************

class CResourceHashArray : public CGObSortArray<CResourceDef *, RESOURCE_ID_BASE>
{
	// This list owns the CResourceDef and CResourceLink objects
	// Sorted array of RESOURCE_ID
public:
	static const char *m_sClassName;

	CResourceHashArray() { };

public:
	int CompareKey(RESOURCE_ID_BASE rid, CResourceDef *pBase, bool fNoSpaces) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT(pBase);
		DWORD dwID1 = rid.GetPrivateUID();
		DWORD dwID2 = pBase->GetResourceID().GetPrivateUID();
		if ( dwID1 > dwID2 )
			return 1;
		if ( dwID1 == dwID2 )
			return 0;
		return -1;
	}

private:
	CResourceHashArray(const CResourceHashArray &copy);
	CResourceHashArray &operator=(const CResourceHashArray &other);
};

class CResourceHash
{
public:
	static const char *m_sClassName;

	CResourceHash() { };

private:
	int GetHashArray(RESOURCE_ID_BASE rid) const
	{
		return (rid.GetResIndex() & 0xF);
	}

public:
	CResourceHashArray m_Array[16];

	inline size_t BadIndex() const
	{
		return m_Array[0].BadIndex();
	}
	size_t FindKey(RESOURCE_ID_BASE rid) const
	{
		return m_Array[GetHashArray(rid)].FindKey(rid);
	}
	CResourceDef *GetAt(RESOURCE_ID_BASE rid, size_t index) const
	{
		return m_Array[GetHashArray(rid)].GetAt(index);
	}
	size_t AddSortKey(RESOURCE_ID_BASE rid, CResourceDef *pNew)
	{
		return m_Array[GetHashArray(rid)].AddSortKey(pNew, rid);
	}
	void SetAt(RESOURCE_ID_BASE rid, size_t index, CResourceDef *pNew)
	{
		m_Array[GetHashArray(rid)].SetAt(index, pNew);
	}

private:
	CResourceHash(const CResourceHash &copy);
	CResourceHash &operator=(const CResourceHash &other);
};

//*********************************************************

struct CStringSortArray : public CGObSortArray<TCHAR *, TCHAR *>
{
	// Sorted array of strings
public:
	CStringSortArray() { };

public:
	int CompareKey(TCHAR *pszID1, TCHAR *pszID2, bool fNoSpaces) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT(pszID2);
		return strcmpi(pszID1, pszID2);
	}

	void AddSortString(LPCTSTR pszText)
	{
		ASSERT(pszText);
		size_t len = strlen(pszText) + 1;
		TCHAR *pNew = new TCHAR[len];
		strncpy(pNew, pszText, len);
		pNew[len - 1] = '\0';
		AddSortKey(pNew, pNew);
	}

private:
	CStringSortArray(const CStringSortArray &copy);
	CStringSortArray &operator=(const CStringSortArray &other);
};

class CObNameSortArray : public CGObSortArray<CScriptObj *, LPCTSTR>
{
	// Sorted array of CScriptObj
public:
	static const char *m_sClassName;

	CObNameSortArray() { };

public:
	int CompareKey(LPCTSTR pszID, CScriptObj *pObj, bool fNoSpaces) const
	{
		ASSERT(pszID);
		ASSERT(pObj);

		LPCTSTR objStr = pObj->GetName();
		if ( fNoSpaces )
		{
			const char *p = strchr(pszID, ' ');
			if ( p )
			{
				size_t iLen = p - pszID;
				//return strnicmp(pszID, pObj->GetName(), iLen);

				size_t objStrLen = strlen(objStr);
				int iRetVal = strnicmp(pszID, objStr, iLen);
				if ( iRetVal == 0 )
				{
					if ( objStrLen == iLen )
						return 0;
					else if ( iLen < objStrLen )
						return -1;
					else
						return 1;
				}
				return iRetVal;
			}
		}
		return strcmpi(pszID, objStr);
	}

private:
	CObNameSortArray(const CObNameSortArray &copy);
	CObNameSortArray &operator=(const CObNameSortArray &other);
};

//*********************************************************

class CResourceBase : public CScriptObj
{
public:
	static const char *m_sClassName;

	CResourceBase() { };
	virtual ~CResourceBase() { };

protected:
	static const LPCTSTR sm_szResourceBlocks[RES_QTY];
	CGObArray<CResourceScript *> m_ResourceFiles;	// all resource files we need to get blocks later

public:
	CResourceHash m_ResHash;	// all script linked resources RES_QTY

	// INI file options
	CGString m_sSCPBaseDir;		// base dir of *.scp files

protected:
	CResourceScript *AddResourceFile(LPCTSTR pszName);
	void AddResourceDir(LPCTSTR pszDirName);

public:
	LPCTSTR GetName() const
	{
		return "CFG";
	}

	// Resource files
	CResourceScript *FindResourceFile(LPCTSTR pszPath);
	void LoadResourcesOpen(CScript *pScript);
	bool LoadResources(CResourceScript *pScript);
	CResourceScript *LoadResourcesAdd(LPCTSTR pszNewFileName);
	virtual bool OpenResourceFind(CScript &s, LPCTSTR pszFileName, bool fCritical = true);
	virtual bool LoadResourceSection(CScript *pScript) = 0;

	CResourceScript *GetResourceFile(size_t i)
	{
		if ( m_ResourceFiles.IsValidIndex(i) )
			return m_ResourceFiles[i];
		return NULL;	// all resource files we need to get blocks from later
	}

	// Resource block definitions
	LPCTSTR ResourceGetName(RESOURCE_ID_BASE rid) const;

	RESOURCE_ID ResourceGetID(RES_TYPE restype, LPCTSTR &pszName);
	RESOURCE_ID ResourceGetIDType(RES_TYPE restype, LPCTSTR pszName)
	{
		// Get a resource of just this index type
		RESOURCE_ID rid = ResourceGetID(restype, pszName);
		if ( rid.GetResType() != restype )
			rid.InitUID();
		return rid;
	}

	int ResourceGetIndexType(RES_TYPE restype, LPCTSTR pszName);

	virtual CResourceDef *ResourceGetDef(RESOURCE_ID_BASE rid) const;
	CScriptObj *ResourceGetDefByName(RES_TYPE restype, LPCTSTR pszName)
	{
		// Resolve a name to the actual resource def
		return ResourceGetDef(ResourceGetID(restype, pszName));
	}

	static LPCTSTR GetResourceBlockName(RES_TYPE restype)
	{
		if ( (restype < 0) || (restype >= RES_QTY) )
			restype = RES_UNKNOWN;
		return sm_szResourceBlocks[restype];
	}

	// Open resource blocks
	bool ResourceLock(CResourceLock &s, RESOURCE_ID_BASE rid);
	bool ResourceLock(CResourceLock &s, RES_TYPE restype, LPCTSTR pszName)
	{
		return ResourceLock(s, ResourceGetIDType(restype, pszName));
	}

private:
	CResourceBase(const CResourceBase &copy);
	CResourceBase &operator=(const CResourceBase &other);
};

#endif // _INC_CRESOURCEBASE_H
