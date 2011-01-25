//
// CResourceBase.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CResourceBase_H
#define _INC_CResourceBase_H
#pragma once

#include "CTime.h"

enum RES_TYPE	// all the script resource blocks we know how to deal with !
{
	// NOTE: SPHERE.INI, SPHERETABLE.SCP are read at start.
	// All other files are indexed from the SCPFILES directories.
	// (SI) = Single instance types.
	// (SL) = single line multiple definitions.
	// Alphabetical order.
	RES_UNKNOWN = 0,		// Not to be used.
	RES_ACCOUNT,		// Define an account instance.
	RES_ADVANCE,		// Define the advance rates for stats.
	RES_AREA,			// Complex region. (w/extra tags)
	RES_BLOCKIP,		// (SL) A list of IP's to block.
	RES_BOOK,			// A book or a page from a book.
	RES_CHARDEF,		// Define a char type. (overlap with RES_SPAWN)
	RES_COMMENT,		// A commented out block type.
	RES_DEFNAME,		// (SL) Just add a bunch of new defs and equivs str/values.
	RES_DIALOG,			// A scriptable gump dialog, text or handler block.
	RES_EVENTS,			// An Event handler block with the trigger type in it. ON=@Death etc.
	RES_FAME,
	RES_FUNCTION,		// Define a new command verb script that applies to a char.
	RES_GMPAGE,			// A GM page. (SAVED in World)
	RES_ITEMDEF,		// Define an item type. (overlap with RES_TEMPLATE)
	RES_KARMA,
	RES_KRDIALOGLIST,	// Mapping of dialog<->kr ids
	RES_MENU,			// General scriptable menus.
	RES_MOONGATES,		// (SL) Define where the moongates are.
	RES_NAMES,			// A block of possible names for a NPC type. (read as needed)
	RES_NEWBIE,			// Triggers to execute on Player creation (based on skills selected)
	RES_NOTOTITLES,		// (SI) Define the noto titles used.
	RES_OBSCENE,		// (SL) A list of obscene words.
	RES_PLEVEL,			// Define the list of commands that a PLEVEL can access. (or not access)
	RES_REGIONRESOURCE,	// Define an Ore type.
	RES_REGIONTYPE,		// Triggers etc. that can be assinged to a RES_AREA
	RES_RESOURCES,		// (SL) list of all the resource files we should index !
	RES_ROOM,			// Non-complex region. (no extra tags)
	RES_RUNES,			// (SI) Define list of the magic runes.
	RES_SCROLL,			// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD, SCROLL_NEWBIE
	RES_SECTOR,			// Make changes to a sector. (SAVED in World)
	RES_SERVERS,		// List a number of servers in 3 line format. (Phase this out)
	RES_SKILL,			// Define attributes for a skill (how fast it raises etc)
	RES_SKILLCLASS,		// Define specifics for a char with this skill class. (ex. skill caps)
	RES_SKILLMENU,		// A menu that is attached to a skill. special arguments over other menus.
	RES_SPAWN,			// Define a list of NPC's and how often they may spawn.
	RES_SPEECH,			// A speech block with ON=*blah* in it.
	RES_SPELL,			// Define a magic spell. (0-64 are reserved)
	RES_SPHERE,			// Main Server INI block
	RES_SPHERECRYPT, // Encryption keys
	RES_STARTS,			// (SI) List of starting locations for newbies.
	RES_STAT,			// Stats elements like KARMA,STR,DEX,FOOD,FAME,CRIMINAL etc. Used for resource and desire scripts.
	RES_TELEPORTERS,	// (SL) Where are the teleporters in the world ? dungeon transports etc.
	RES_TEMPLATE,		// Define lists of items. (for filling loot etc)
	RES_TIMERF,
	RES_TIP,			// Tips (similar to RES_SCROLL) that can come up at startup.
	RES_TYPEDEF,		// Define a trigger block for a RES_WORLDITEM m_type.
	RES_TYPEDEFS,
	RES_WC,				// =WORLDCHAR
	RES_WEBPAGE,		// Define a web page template.
	RES_WI,				// =WORLDITEM
	RES_WORLDCHAR,		// Define instance of char in the world. (SAVED in World)
	RES_WORLDITEM,		// Define instance of item in the world. (SAVED in World)
	RES_WORLDLISTS,
	RES_WORLDSCRIPT,	// Something to load into a script.
	RES_WORLDVARS,
	RES_WS,				// =WORLDSCRIPT
	RES_QTY,			// Don't care
};

#define RES_DIALOG_TEXT				1	// sub page for the section.
#define RES_DIALOG_BUTTON			2
#define RES_NEWBIE_MALE_DEFAULT		(10000+1)	// just an unused number for the range.
#define RES_NEWBIE_FEMALE_DEFAULT	(10000+2)

struct RESOURCE_ID_BASE : public CGrayUIDBase
{
#define RES_TYPE_SHIFT	25	// leave 6 bits = 64 for RES_TYPE
#define RES_TYPE_MASK	63
#define RES_PAGE_SHIFT	18	// leave 7 bits = 128 pages of space.
#define RES_PAGE_MASK	127
#define RES_INDEX_SHIFT	0	// leave 18 bits = 262144 entries.
#define RES_INDEX_MASK	0x3FFFF

public:
#define RES_GET_TYPE(dw)	( ( dw >> RES_TYPE_SHIFT) & RES_TYPE_MASK )
	RES_TYPE GetResType() const
	{
		DWORD dwVal = RES_GET_TYPE(m_dwInternalVal);
		return( (RES_TYPE) dwVal );
	}
#define RES_GET_INDEX(dw)	((dw)&RES_INDEX_MASK)
	int GetResIndex() const
	{
		return ( RES_GET_INDEX(m_dwInternalVal) );
	}
	int GetResPage() const
	{
		DWORD dwVal = m_dwInternalVal >> RES_PAGE_SHIFT;
		dwVal &= RES_PAGE_MASK;
		return(dwVal);
	}
	bool operator == ( const RESOURCE_ID_BASE & rid ) const
	{
		return( rid.m_dwInternalVal == m_dwInternalVal );
	}
};

struct RESOURCE_ID : public RESOURCE_ID_BASE
{
	RESOURCE_ID()
	{
		InitUID();
	}
	RESOURCE_ID( RES_TYPE restype )
	{
		// single instance type.
		m_dwInternalVal = UID_F_RESOURCE|((restype)<<RES_TYPE_SHIFT);
	}
	RESOURCE_ID( RES_TYPE restype, int index )
	{
		ASSERT( index < RES_INDEX_MASK );
		m_dwInternalVal = UID_F_RESOURCE|((restype)<<RES_TYPE_SHIFT)|(index);
	}
	RESOURCE_ID( RES_TYPE restype, int index, int iPage )
	{
		ASSERT( index < RES_INDEX_MASK );
		ASSERT( iPage < RES_PAGE_MASK );
		m_dwInternalVal = UID_F_RESOURCE|((restype)<<RES_TYPE_SHIFT)|((iPage)<<RES_PAGE_SHIFT)|(index);
	}
	RESOURCE_ID_BASE & operator = ( const RESOURCE_ID_BASE & rid )
	{
		ASSERT( rid.IsValidUID());
		ASSERT( rid.IsResource());
		m_dwInternalVal = rid.GetPrivateUID();
		return( *this );
	}
};

// Desguise an id as a pointer.
#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(id) ((LPCTSTR)((DWORD)((WORD)(id))))
#endif
#define ISINTRESOURCE(p)	(!(((DWORD)p)&0xFFFFF000))
#define GETINTRESOURCE(p)	(((DWORD)p)&0x0FFF)

//*********************************************************

struct CResourceQty
{
private:
	RESOURCE_ID m_rid;	// A RES_SKILL, RES_ITEMDEF, or RES_TYPEDEF
	int m_iQty;			// How much of this ?
public:
	RESOURCE_ID GetResourceID() const
	{
		return( m_rid );
	}
	void SetResourceID( RESOURCE_ID rid, int iQty )
	{
		m_rid = rid;
		m_iQty = iQty;
	}
	RES_TYPE GetResType() const
	{
		return( m_rid.GetResType());
	}
	int GetResIndex() const
	{
		return( m_rid.GetResIndex());
	}
	int GetResQty() const
	{
		return( m_iQty );
	}
	void SetResQty( int wQty )
	{
		m_iQty = wQty;
	}

	inline bool Load( LPSTR & arg )
	{
		return Load( (LPCTSTR&)arg );
	}

	bool Load( LPCTSTR & pszCmds );
	size_t WriteKey( TCHAR * pszArgs, bool fQtyOnly = false, bool fKeyOnly = false ) const;
	size_t WriteNameSingle( TCHAR * pszArgs, int iQty = 0 ) const;
public:
	CResourceQty() : m_iQty(0) { };
};

class CResourceQtyArray : public CGTypedArray<CResourceQty, CResourceQty&>
{
	// Define a list of index id's (not references) to resource objects. (Not owned by the list)
public:
	static const char *m_sClassName;
	CResourceQtyArray();
	explicit CResourceQtyArray(LPCTSTR pszCmds);
	bool operator == ( const CResourceQtyArray & array ) const;
	CResourceQtyArray& operator=(const CResourceQtyArray& other);

private:
	CResourceQtyArray(const CResourceQtyArray& copy);

public:
	int Load( LPCTSTR pszCmds );
	void WriteKeys( TCHAR * pszArgs, int index = 0, bool fQtyOnly = false, bool fKeyOnly = false ) const;
	void WriteNames( TCHAR * pszArgs, int index = 0 ) const;

	int FindResourceID( RESOURCE_ID_BASE rid ) const;
	int FindResourceType( RES_TYPE type ) const;
	int FindResourceMatch( CObjBase * pObj ) const;
	bool IsResourceMatchAll( CChar * pChar ) const;

	void setNoMergeOnLoad();

private:
	bool m_mergeOnLoad;
};

//*********************************************************

class CScriptFileContext
{
	// Track a temporary context into a script.
	// NOTE: This should ONLY be stack based !
private:
	bool m_fOpenScript;	// NULL context may be legit.
	const CScript * m_pPrvScriptContext;	// previous general context before this was opened.
private:
	void Init()
	{
		m_fOpenScript = false;
	}
public:
	static const char *m_sClassName;
	void OpenScript( const CScript * pScriptContext );
	void Close();
	CScriptFileContext() : m_pPrvScriptContext(NULL)
	{
		Init();
	}
	explicit CScriptFileContext( const CScript * pScriptContext )
	{
		Init();
		OpenScript( pScriptContext );
	}
	~CScriptFileContext()
	{
		Close();
	}

private:
	CScriptFileContext(const CScriptFileContext& copy);
	CScriptFileContext& operator=(const CScriptFileContext& other);
};

class CScriptObjectContext
{
	// Track a temporary context of an object.
	// NOTE: This should ONLY be stack based !
private:
	bool m_fOpenObject;	// NULL context may be legit.
	const CScriptObj * m_pPrvObjectContext;	// previous general context before this was opened.
private:
	void Init()
	{
		m_fOpenObject = false;
	}
public:
	static const char *m_sClassName;
	void OpenObject( const CScriptObj * pObjectContext );
	void Close();
	CScriptObjectContext() : m_pPrvObjectContext(NULL)
	{
		Init();
	}
	explicit CScriptObjectContext( const CScriptObj * pObjectContext )
	{
		Init();
		OpenObject( pObjectContext );
	}
	~CScriptObjectContext()
	{
		Close();
	}

private:
	CScriptObjectContext(const CScriptObjectContext& copy);
	CScriptObjectContext& operator=(const CScriptObjectContext& other);
};

//*********************************************************

class CResourceScript : public CScript
{
	// A script file containing resource, speech, motives or events handlers.
	// NOTE: we should check periodically if this file has been altered externally ?
private:
	int		m_iOpenCount;		// How many CResourceLock(s) have this open ?
	CServTime m_timeLastAccess;	// CWorld time of last access/Open.

	// Last time it was closed. What did the file params look like ?
	DWORD m_dwSize;			// Compare to see if this has changed.
	CGTime m_dateChange;	// real world time/date of last change.

private:
	void Init()
	{
		m_iOpenCount = 0;
		m_timeLastAccess.Init();
		m_dwSize = (DWORD) -1;			// Compare to see if this has changed.
	}
	bool CheckForChange();

public:
	static const char *m_sClassName;
	explicit CResourceScript( LPCTSTR pszFileName )
	{
		Init();
		SetFilePath( pszFileName );
	}
	CResourceScript()
	{
		Init();
	}

private:
	CResourceScript(const CResourceScript& copy);
	CResourceScript& operator=(const CResourceScript& other);

public:
	bool IsFirstCheck() const
	{
		return( m_dwSize == -1 && ! m_dateChange.IsTimeValid());
	}

	void ReSync();
	bool Open( LPCTSTR pszFilename = NULL, UINT wFlags = OF_READ );
	virtual void Close();
	virtual void CloseForce();
};

class CResourceLock : public CScript
{
	// Open a copy of a scipt that is already open
	// NOTE: This should ONLY be stack based !
	// preserve the previous openers offset in the script.
private:
	CResourceScript * m_pLock;
	CScriptLineContext m_PrvLockContext;	// i must return the locked file back here.	

	CScriptFileContext m_PrvScriptContext;	// where was i before (context wise) opening this. (for error tracking)
	CScriptObjectContext m_PrvObjectContext; // object context (for error tracking)
private:
	void Init()
	{
		m_pLock = NULL;
		m_PrvLockContext.Init();	// means the script was NOT open when we started.
	}

protected:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();
	virtual bool ReadTextLine( bool fRemoveBlanks );

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
	CResourceLock(const CResourceLock& copy);
	CResourceLock& operator=(const CResourceLock& other);

public:
	int OpenLock( CResourceScript * pLock, CScriptLineContext context );
	void AttachObj( const CScriptObj * pObj )
	{
		m_PrvObjectContext.OpenObject(pObj);
	}
};

class CResourceDef : public CScriptObj
{
	// Define a generic  resource block in the scripts.
	// Now the scripts can be modular. resources can be defined any place.
	// NOTE: This may be loaded fully into memory or just an Index to a file.
private:
	RESOURCE_ID m_rid;		// the true resource id. (must be unique for the RES_TYPE)
protected:
	const CVarDefContNum * m_pDefName;	// The name of the resource. (optional)
public:
	static const char *m_sClassName;
	CResourceDef( RESOURCE_ID rid, LPCTSTR pszDefName ) :
		m_rid( rid ),
		m_pDefName( NULL )
	{
		SetResourceName( pszDefName );
	}
	CResourceDef( RESOURCE_ID rid, const CVarDefContNum * pDefName = NULL ) :
		m_rid( rid ),
		m_pDefName( pDefName )
	{
	}
	virtual ~CResourceDef()	// need a virtual for the dynamic_cast to work.
	{
		// ?? Attempt to remove m_pDefName ?
	}

private:
	CResourceDef(const CResourceDef& copy);
	CResourceDef& operator=(const CResourceDef& other);

public:
	RESOURCE_ID GetResourceID() const
	{
		return( m_rid );
	}
	RES_TYPE GetResType() const
	{
		return( m_rid.GetResType() );
	}
	int GetResPage() const
	{
		return( m_rid.GetResPage());
	}

	void CopyDef( const CResourceDef * pLink )
	{
		m_pDefName = pLink->m_pDefName;
	}

	// Get the name of the resource item. (Used for saving) may be number or name
	LPCTSTR GetResourceName() const;
	virtual LPCTSTR GetName() const	// default to same as the DEFNAME name.
	{
		return( GetResourceName());
	}

	// Give it another DEFNAME= even if it already has one. it's ok to have multiple names.
	bool SetResourceName( LPCTSTR pszName );
	void SetResourceVar( const CVarDefContNum* pVarNum )
	{
		if ( pVarNum != NULL && m_pDefName == NULL )
		{
			m_pDefName = pVarNum;
		}
	}

	// unlink all this data. (tho don't delete the def as the pointer might still be used !)
	virtual void UnLink()
	{
		// This does nothing in the CResourceDef case, Only in the CResourceLink case.
	}

	bool	HasResourceName();
	bool	MakeResourceName();
};

#define MAX_TRIGGERS_ARRAY	5

class CResourceLink : public CResourceDef
{
	// A single resource object that also has part of itself remain in resource file.
	// A pre-indexed link into a script file.
	// This is a CResourceDef not fully read into memory at index time.
	// We are able to lock it and read it as needed
private:
	CResourceScript * m_pScript;	// we already found the script.
	CScriptLineContext m_Context;

	DWORD m_lRefInstances;	// How many CResourceRef objects refer to this ?
public:
	static const char *m_sClassName;
	DWORD	m_dwOnTriggers[MAX_TRIGGERS_ARRAY];

#define XTRIG_UNKNOWN 0	// bit 0 is reserved to say there are triggers here that do not conform.

public:

	void AddRefInstance()
	{
		m_lRefInstances ++;
	}
	void DelRefInstance()
	{
#ifdef _DEBUG
		ASSERT(m_lRefInstances > 0);
#endif
		m_lRefInstances --;
	}
	DWORD GetRefInstances() const
	{
		return( m_lRefInstances );
	}

	bool IsLinked() const;	// been loaded from the scripts ?
	CResourceScript * GetLinkFile() const;
	long GetLinkOffset() const;
	void SetLink( CResourceScript * pScript );
    void CopyTransfer( CResourceLink * pLink );
	void ScanSection( RES_TYPE restype );
	void ClearTriggers();
	void SetTrigger( int i );
	bool HasTrigger( int i ) const;
	bool ResourceLock( CResourceLock & s );

public:
	CResourceLink( RESOURCE_ID rid, const CVarDefContNum * pDef = NULL );
	virtual ~CResourceLink();

private:
	CResourceLink(const CResourceLink& copy);
	CResourceLink& operator=(const CResourceLink& other);
};

class CResourceNamed : public CResourceLink
{
	// Private name pool. (does not use DEFNAME) RES_FUNCTION
public:
	static const char *m_sClassName;
	const CGString m_sName;
public:
	CResourceNamed( RESOURCE_ID rid, LPCTSTR pszName ) :
		CResourceLink( rid ),
		m_sName( pszName )
	{
	}
	virtual ~CResourceNamed()
	{
	}

private:
	CResourceNamed(const CResourceNamed& copy);
	CResourceNamed& operator=(const CResourceNamed& other);

public:
	LPCTSTR GetName() const
	{
		return( m_sName );
	}
};

//***********************************************************

class CResourceRef
{
private:
	CResourceLink* m_pLink;
public:
	static const char *m_sClassName;
	CResourceRef()
	{
		m_pLink = NULL;
	}
	CResourceRef( CResourceLink* pLink ) : m_pLink(pLink)
	{
		ASSERT(pLink);
		pLink->AddRefInstance();
	}
	CResourceRef(const CResourceRef& copy)
	{
		m_pLink = copy.m_pLink;
		if (m_pLink != NULL)
			m_pLink->AddRefInstance();
	}
	~CResourceRef()
	{
		if (m_pLink != NULL)
			m_pLink->DelRefInstance();
	}
	CResourceRef& operator=(const CResourceRef& other)
	{
		if (this != &other)
		{
			SetRef(other.m_pLink);
		}
		return *this;
	}

public:
	CResourceLink* GetRef() const
	{
		return(m_pLink);
	}
	void SetRef( CResourceLink* pLink )
	{
		if ( m_pLink != NULL )
			m_pLink->DelRefInstance();

		m_pLink = pLink;

		if ( pLink != NULL )
			pLink->AddRefInstance();
	}
	operator CResourceLink*() const
    {
		return( GetRef());
    }
};

class CResourceRefArray : public CGPtrTypeArray<CResourceRef>
{
	// Define a list of pointer references to resource. (Not owned by the list)
	// An indexed list of CResourceLink s.
private:
	LPCTSTR GetResourceName( int iIndex ) const
	{
		// look up the name of the fragment given it's index.
		CResourceLink * pResourceLink = GetAt( iIndex );
		ASSERT(pResourceLink);
		return( pResourceLink->GetResourceName());
	}
public:
	static const char *m_sClassName;
	CResourceRefArray() { };
private:
	CResourceRefArray(const CResourceRefArray& copy);
	CResourceRefArray& operator=(const CResourceRefArray& other);

public:
	int FindResourceType( RES_TYPE type ) const;
	int FindResourceID( RESOURCE_ID_BASE rid ) const;
	int FindResourceName( RES_TYPE restype, LPCTSTR pszKey ) const;

	void WriteResourceRefList( CGString & sVal ) const;
	bool r_LoadVal( CScript & s, RES_TYPE restype );
	void r_Write( CScript & s, LPCTSTR pszKey ) const;
};

//*********************************************************

class CResourceHashArray : public CGObSortArray< CResourceDef*, RESOURCE_ID_BASE >
{
	// This list OWNS the CResourceDef and CResourceLink objects.
	// Sorted array of RESOURCE_ID
public:
	static const char *m_sClassName;
	CResourceHashArray() { };
private:
	CResourceHashArray(const CResourceHashArray& copy);
	CResourceHashArray& operator=(const CResourceHashArray& other);
public:
	int CompareKey( RESOURCE_ID_BASE rid, CResourceDef * pBase, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		DWORD dwID1 = rid.GetPrivateUID();
		ASSERT( pBase );
		DWORD dwID2 = pBase->GetResourceID().GetPrivateUID();
		if (dwID1 > dwID2 )
			return(1);
		if (dwID1 == dwID2 )
			return(0);
		return(-1);
	}
};

class CResourceHash
{
public:
	static const char *m_sClassName;
	CResourceHashArray m_Array[16];
public:
	CResourceHash() { };
private:
	CResourceHash(const CResourceHash& copy);
	CResourceHash& operator=(const CResourceHash& other);
private:
	int GetHashArray( RESOURCE_ID_BASE rid ) const
	{
		return( rid.GetResIndex() & 0x0F );
	}
public:
	int FindKey( RESOURCE_ID_BASE rid ) const
	{
		return( m_Array[ GetHashArray( rid ) ].FindKey(rid));
	}
	CResourceDef* GetAt( RESOURCE_ID_BASE rid, int index ) const
	{
		return( m_Array[ GetHashArray( rid ) ].GetAt(index));
	}
	int AddSortKey( RESOURCE_ID_BASE rid, CResourceDef* pNew )
	{
		return( m_Array[ GetHashArray( rid ) ].AddSortKey( pNew, rid ));
	}
	void SetAt( RESOURCE_ID_BASE rid, int index, CResourceDef* pNew )
	{
		m_Array[ GetHashArray( rid ) ].SetAt( index, pNew );
	}
};

//*************************************************

struct CStringSortArray : public CGObSortArray< TCHAR*, TCHAR* >
{
public:
	CStringSortArray() { };
private:
	CStringSortArray(const CStringSortArray& copy);
	CStringSortArray& operator=(const CStringSortArray& other);
public:
	virtual void DestructElements( TCHAR** pElements, int nCount )
	{
		// delete the objects that we own.
		for ( int i = 0; i < nCount; i++ )
		{
			if ( pElements[i] != NULL )
			{
				delete[] pElements[i];
				pElements[i] = NULL;
			}
		}

		CGObSortArray<TCHAR*, TCHAR*>::DestructElements(pElements, nCount);
	}

	// Sorted array of strings
	int CompareKey( TCHAR* pszID1, TCHAR* pszID2, bool fNoSpaces ) const
	{
		UNREFERENCED_PARAMETER(fNoSpaces);
		ASSERT( pszID2 );
		return( strcmpi( pszID1, pszID2));
	}

	void AddSortString( LPCTSTR pszText )
	{
		ASSERT(pszText);
		int len = strlen( pszText );
		TCHAR * pNew = new TCHAR [ len + 1 ];
		strcpy( pNew, pszText );
		AddSortKey( pNew, pNew );
	}
};

class CObNameSortArray : public CGObSortArray< CScriptObj*, LPCTSTR >
{
public:
	static const char *m_sClassName;
	CObNameSortArray() { };

private:
	CObNameSortArray(const CObNameSortArray& copy);
	CObNameSortArray& operator=(const CObNameSortArray& other);

public:
	// Array of CScriptObj. name sorted.
	int CompareKey( LPCTSTR pszID, CScriptObj* pObj, bool fNoSpaces ) const
	{
		ASSERT( pszID );
		ASSERT( pObj );

		LPCSTR objStr = pObj->GetName();
		if ( fNoSpaces )
		{
			const char * p = strchr( pszID, ' ' );
			if (p != NULL)
			{
				int		iLen	= p - pszID;
				// return( strnicmp( pszID, pObj->GetName(), iLen ) );

				int objStrLen = strlen( objStr );
				int retval = strnicmp( pszID, objStr, iLen );
				if ( retval == 0 )
				{
					if (objStrLen == iLen )
					{
						return 0;
					} 
					else if ( iLen < objStrLen ) 
					{
						return -1;
					} 
					else 
					{
						return 1;
					}
				} 
				else 
				{
					return(retval);
				}
			}
		}
		return( strcmpi( pszID, objStr) );
	}
};

//***************************************************************8

class CResourceBase : public CScriptObj
{
protected:
	static LPCTSTR const sm_szResourceBlocks[RES_QTY];

	CGObArray< CResourceScript* > m_ResourceFiles;	// All resource files we need to get blocks from later.

public:
	static const char *m_sClassName;
	CResourceHash m_ResHash;		// All script linked resources RES_QTY

	// INI File options.
	CGString m_sSCPBaseDir;		// if we want to get *.SCP files from elsewhere.

protected:
	CResourceScript * AddResourceFile( LPCTSTR pszName );
	void AddResourceDir( LPCTSTR pszDirName );

public:
	void LoadResourcesOpen( CScript * pScript );
	bool LoadResources( CResourceScript * pScript );

	static LPCTSTR GetResourceBlockName( RES_TYPE restype );

	LPCTSTR GetName() const
	{
		return "CFG";
	}

	CResourceScript* GetResourceFile( int i )
	{
		if ( ! m_ResourceFiles.IsValidIndex(i))
		{
			return( NULL );	// All resource files we need to get blocks from later.
		}
		return( m_ResourceFiles[i] );
	}

	RESOURCE_ID ResourceGetID( RES_TYPE restype, LPCTSTR & pszName );
	RESOURCE_ID ResourceGetIDType( RES_TYPE restype, LPCTSTR pszName )
	{
		// Get a resource of just this index type.
		RESOURCE_ID rid = ResourceGetID( restype, pszName );
		if ( rid.GetResType() != restype )
		{
			rid.InitUID();
			return( rid );
		}
		return( rid );
	}
	int ResourceGetIndexType( RES_TYPE restype, LPCTSTR pszName );

	LPCTSTR ResourceGetName( RESOURCE_ID_BASE rid ) const;

	CScriptObj * ResourceGetDefByName( RES_TYPE restype, LPCTSTR pszName )
	{
		// resolve a name to the actual resource def.
		return( ResourceGetDef( ResourceGetID( restype, pszName )));
	}

	bool ResourceLock( CResourceLock & s, RESOURCE_ID_BASE rid );
	bool ResourceLock( CResourceLock & s, RES_TYPE restype, LPCTSTR pszName )
	{
		return ResourceLock( s, ResourceGetIDType( restype, pszName ));
	}

	CResourceScript * FindResourceFile( LPCTSTR pszTitle );
	CResourceScript * LoadResourcesAdd( LPCTSTR pszNewName );
	
	virtual CResourceDef * ResourceGetDef( RESOURCE_ID_BASE rid ) const;
	virtual bool OpenResourceFind( CScript &s, LPCTSTR pszFilename, bool bCritical = true );
	virtual bool LoadResourceSection( CScript * pScript ) = 0;

public:
	CResourceBase() { };
	virtual ~CResourceBase() { };

private:
	CResourceBase(const CResourceBase& copy);
	CResourceBase& operator=(const CResourceBase& other);
};

inline LPCTSTR CResourceBase::GetResourceBlockName( RES_TYPE restype )	// static
{
	if ( restype < 0 || restype >= RES_QTY )
		restype = RES_UNKNOWN;
	return( sm_szResourceBlocks[restype] );
}

#endif // _INC_CResourceBase_H
