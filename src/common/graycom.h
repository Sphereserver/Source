#ifndef _INC_GRAYCOM_H
#define _INC_GRAYCOM_H
#pragma once

// System definitions

#ifdef _WIN32
	// NOTE: If we want a max number of sockets we must compile for it
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024	// for max of n users ! default = 64

	#ifndef STRICT
		#define STRICT		// strict conversion of handlers and pointers
	#endif

	#include <WinSock2.h>
	extern const OSVERSIONINFO *GRAY_GetOSInfo();
#else
	#include "os_unix.h"
	#ifdef _BSD
		int getTimezone();
		#define _timezone		getTimezone()
	#else
		#define _timezone		timezone
	#endif
#endif

#ifndef ASSERT
	#ifdef _DEBUG
		extern void Assert_CheckFail(LPCTSTR pszExp, LPCTSTR pszFile, long lLine);
		#define ASSERT(exp)		(void)((exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0))
	#else
		#define ASSERT(exp)
	#endif
#endif

#ifdef _WIN32
	#define ATOI atoi
	#define ITOA _itoa
	#define LTOA _ltoa
	#define STRREV _strrev
#else
	int ATOI(const char *str);
	char *ITOA(int value, char *buffer, int radix);
	char *LTOA(long value, char *buffer, int radix);
	void STRREV(char *str);
#endif

#define FEATURE_T2A_UPDATE			0x01	// client feature flag 0x4
#define FEATURE_T2A_CHAT			0x02	// client feature flag 0x1

#define FEATURE_LBR_UPDATE			0x01	// client feature flag 0x8
#define FEATURE_LBR_SOUND			0x02	// client feature flag 0x2

#define FEATURE_AOS_UPDATE_A		0x01	// client feature flag 0x10
#define FEATURE_AOS_UPDATE_B		0x02	// character list flag 0x20
#define FEATURE_AOS_POPUP			0x04	// character list flag 0x8
#define FEATURE_AOS_DAMAGE			0x08	// 

#define FEATURE_SE_UPDATE			0x01	// client feature flag 0x40
#define FEATURE_SE_NINJASAM			0x02	// character list flag 0x80

#define FEATURE_ML_UPDATE			0x01	// client feature flag 0x80 / character list flag 0x100

#define FEATURE_KR_UPDATE			0x01	// character list flag 0x200

#define FEATURE_SA_UPDATE			0x01	// client feature flag 0x10000
#define FEATURE_SA_MOVEMENT			0x02	// character list flag 0x4000

#define FEATURE_TOL_UPDATE			0x01	// client feature flag 0x400000
#define FEATURE_TOL_VIRTUALGOLD		0x02	// not related to client/character flags (use virtual gold/platinum currency instead physical gold)

#define FEATURE_EXTRA_CRYSTAL		0x01	// client feature flag 0x200 (unlock ML crystal items on house design)
#define FEATURE_EXTRA_GOTHIC		0x02	// client feature flag 0x40000 (unlock SA gothic items on house design)
#define FEATURE_EXTRA_RUSTIC		0x04	// client feature flag 0x80000 (unlock SA rustic items on house design)
#define FEATURE_EXTRA_JUNGLE		0x08	// client feature flag 0x100000 (unlock TOL jungle items on house design)
#define FEATURE_EXTRA_SHADOWGUARD	0x10	// client feature flag 0x200000 (unlock TOL shadowguard items on house design)
#define FEATURE_EXTRA_ROLEPLAYFACES	0x20	// client feature flag 0x2000 (unlock extra roleplay face styles on character creation) - enhanced clients only

class CTextConsole;
class CObjBase;
class CChar;
class CItem;

#include "CException.h"
#include "CSocket.h"
#include "CEncrypt.h"
#include "CArray.h"
#include "CVarDefMap.h"
#include "CListDefMap.h"
#include "CExpression.h"
#include "CVarFloat.h"
#include "CScriptObj.h"

struct CGrayUIDBase		// A unique system serial id (4 bytes long)
{
	// This is a ref to a game object. It may or may not be valid
	// The top few bits are just flags
	#define UID_CLEAR			0
	#define UID_UNUSED			0xFFFFFFFF	// 0 = not used as well

	#define UID_F_ITEM			0x40000000	// CItem as apposed to CChar based
	#define UID_F_RESOURCE		0x80000000	// ALSO: pileable or special macro flag passed to client

	#define UID_O_CONTAINED		0x10000000	// This item is inside another container
	#define UID_O_EQUIPPED		0x20000000	// This item is equipped
	#define UID_O_DISCONNECT	0x30000000	// Not attached yet

	#define UID_O_INDEX_FREE	0x01000000	// Spellbook needs unused UID's?
	#define UID_O_INDEX_MASK	0x0FFFFFFF	// lose the upper bits

protected:
	DWORD m_dwInternalVal;

public:
	CObjBase *ObjFind() const;
	CItem *ItemFind() const;
	CChar *CharFind() const;

	void InitUID()
	{
		m_dwInternalVal = UID_UNUSED;
	}
	void ClearUID()
	{
		m_dwInternalVal = UID_CLEAR;
	}

	void SetPrivateUID(DWORD dwVal)
	{
		m_dwInternalVal = dwVal;
	}
	DWORD GetPrivateUID() const
	{
		return m_dwInternalVal;
	}

	DWORD GetObjUID() const
	{
		return (m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM));
	}
	void SetObjUID(DWORD dwVal)
	{
		// Can be set to -1 by client
		m_dwInternalVal = (dwVal & (UID_O_INDEX_MASK|UID_F_ITEM))|UID_O_DISCONNECT;
	}

	bool IsValidUID() const
	{
		return (m_dwInternalVal && ((m_dwInternalVal & UID_O_INDEX_MASK) != UID_O_INDEX_MASK));
	}
	bool IsResource() const
	{
		return (m_dwInternalVal & UID_F_RESOURCE) ? IsValidUID() : false;
	}
	bool IsItem() const
	{
		return ((m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == UID_F_ITEM);
	}
	bool IsChar() const
	{
		return ((m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == 0) ? IsValidUID() : false;
	}

	bool IsObjDisconnected() const	// not in the game world for some reason
	{
		return ((m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT);
	}
	bool IsObjTopLevel() const	// on the ground in the world
	{
		return ((m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0);
	}

	bool IsItemEquipped() const
	{
		return ((m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED)) ? IsValidUID() : false;
	}
	bool IsItemInContainer() const
	{
		return ((m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED)) ? IsValidUID() : false;
	}

	void SetObjContainerFlags(DWORD dwFlags = 0)
	{
		m_dwInternalVal = (m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM))|dwFlags;
	}

public:
	bool operator==(DWORD index) const
	{
		return (GetObjUID() == index);
	}
	bool operator!=(DWORD index) const
	{
		return (GetObjUID() != index);
	}
	operator DWORD() const
	{
		return GetObjUID();
	}
};

struct CGrayUID : public CGrayUIDBase
{
	CGrayUID()
	{
		InitUID();
	}
	CGrayUID(DWORD dwVal)
	{
		SetPrivateUID(dwVal);
	}
};

#ifdef _DEBUG
	#ifdef _NO_DEBUG_ASSERTS
		#undef ASSERT
		#define ASSERT(exp)
	#endif
#endif

#endif	// _INC_GRAYCOM_H
