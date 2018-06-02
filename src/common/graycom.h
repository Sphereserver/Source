#ifndef _INC_GRAYCOM_H
#define _INC_GRAYCOM_H
#pragma once

//---------------------------SYSTEM DEFINITIONS---------------------------

#include <time.h>

#ifdef _WIN32
	// NOTE: If we want a max number of sockets we must compile for it !
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024 // for max of n users ! default = 64

	#ifndef STRICT
		#define STRICT			// strict conversion of handles and pointers.
	#endif	// STRICT

	#include <WinSock2.h>

	#define strcmpi		_strcmpi	// Non ANSI equiv functions ?
	#define strnicmp	_strnicmp

	extern const OSVERSIONINFO * GRAY_GetOSInfo();

#else	// _WIN32 else assume LINUX

	#include <stdio.h>
	#include <limits.h>
	#include <limits>
	#include <string.h>

	#define HANDLE			DWORD
	#define _cdecl
	#define __cdecl

	#define WCHAR			unsigned short
	#define FAR
	#define E_FAIL			0x80004005
	#define BOOL			unsigned short

	#ifdef _BSD
		int getTimezone();
		#define _timezone		getTimezone()
	#else
		#define _timezone		timezone
	#endif

	#define strcmpi		strcasecmp
	#define strnicmp	strncasecmp
	#define _vsnprintf	vsnprintf
#endif // !_WIN32

#define INT32		int
#define UINT32		unsigned int

#define INT64		long long
#define UINT64		unsigned long long

#ifdef _DEBUG
	#ifndef ASSERT
		extern void Assert_CheckFail(LPCTSTR pszExp, LPCTSTR pszFile, long lLine);
		#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
	#endif	// ASSERT

#else	// _DEBUG

	#ifndef ASSERT
		/*#ifndef _WIN32
			// In linux, if we get an access violation, an exception isn't thrown.  Instead, we get
			// a SIG_SEGV, and the process cores. The following code takes care of this for us.
			extern void Assert_CheckFail(LPCTSTR pszExp, LPCTSTR pszFile, long lLine);
			//matex3: is this still necessary? We have a SIG_SEGV handler nowaways.
			#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
		#else*/
			#define ASSERT(exp)
		/*#endif*/
	#endif	// ASSERT

#endif	// ! _DEBUG

#ifdef _WIN32
	#define ATOI atoi
	#define ITOA _itoa
	#define LTOA _ltoa
	#define STRREV _strrev
#else
	int ATOI( const char * str );
	char * ITOA(int value, char *string, int radix);
	char * LTOA(long value, char *string, int radix);
	void STRREV( char* string );
#endif

#define FEATURE_T2A_UPDATE 			0x01
#define FEATURE_T2A_CHAT 			0x02

#define FEATURE_LBR_UPDATE			0x01
#define FEATURE_LBR_SOUND			0x02

#define FEATURE_AOS_UPDATE_A		0x01	// AOS Monsters, Map, Skills
#define FEATURE_AOS_UPDATE_B		0x02	// Tooltip, Fightbook, Necro/paladin on creation, Single/Six char selection screen
#define FEATURE_AOS_POPUP			0x04	// PopUp Menus
#define FEATURE_AOS_DAMAGE			0x08

#define FEATURE_SE_UPDATE			0x01	// 0x00008 in 0xA9
#define FEATURE_SE_NINJASAM			0x02	// 0x00040 in feature

#define FEATURE_ML_UPDATE			0x01 	// 0x00100 on charlist and 0x0080 for feature to activate

#define FEATURE_KR_UPDATE			0x01	// 0x00200 in 0xA9 (KR crapness)

#define FEATURE_SA_UPDATE			0x01	// 0x10000 feature (unlock gargoyle character, housing items)
#define FEATURE_SA_MOVEMENT			0x02	// 0x04000 on charlist (new movement packets)

#define FEATURE_TOL_UPDATE			0x01	// 0x400000 feature
#define FEATURE_TOL_VIRTUALGOLD		0x02	// Use virtual gold/platinum instead physical gold. Not related to login flags

#define FEATURE_EXTRA_CRYSTAL		0x01	// 0x200 feature (unlock ML crystal items on house design)
#define FEATURE_EXTRA_GOTHIC		0x02	// 0x40000 feature (unlock SA gothic items on house design)
#define FEATURE_EXTRA_RUSTIC		0x04	// 0x80000 feature (unlock SA rustic items on house design)
#define FEATURE_EXTRA_JUNGLE		0x08	// 0x100000 feature (unlock TOL jungle items on house design)
#define FEATURE_EXTRA_SHADOWGUARD	0x10	// 0x200000 feature (unlock TOL shadowguard items on house design)
#define FEATURE_EXTRA_ROLEPLAYFACES	0x20	// 0x2000 feature (unlock extra roleplay face styles on character creation) - enhanced clients only

#include "CException.h"

#include "CSocket.h"
#include "CEncrypt.h"

#include "CArray.h"

class CTextConsole; // swapped these two includes, so need to declare this here
#include "CVarDefMap.h"
#include "CListDefMap.h"
#include "CExpression.h"
#include "CVarFloat.h"
#include "CScriptObj.h"

class CObjBase;
class CChar;
class CItem;

struct CGrayUIDBase		// A unique system serial id. 4 bytes long
{
	// This is a ref to a game object. It may or may not be valid.
	// The top few bits are just flags.
#define UID_CLEAR			0
#define UID_UNUSED			0xFFFFFFFF	// 0 = not used as well.

#define UID_F_RESOURCE		0x80000000	// ALSO: pileable or special macro flag passed to client.
#define UID_F_ITEM			0x40000000	// CItem as apposed to CChar based

#define UID_O_EQUIPPED		0x20000000	// This item is equipped.
#define UID_O_CONTAINED		0x10000000	// This item is inside another container
#define UID_O_DISCONNECT	0x30000000	// Not attached yet.
#define UID_O_INDEX_MASK	0x0FFFFFFF	// lose the upper bits.
#define UID_O_INDEX_FREE	0x01000000	// Spellbook needs unused UID's ?

protected:
	DWORD m_dwInternalVal;
public:

	bool IsValidUID() const
	{
		return( m_dwInternalVal && ( m_dwInternalVal & UID_O_INDEX_MASK ) != UID_O_INDEX_MASK );
	}
	void InitUID()
	{
		m_dwInternalVal = UID_UNUSED;
	}
	void ClearUID()
	{
		m_dwInternalVal = UID_CLEAR;
	}

	bool IsResource() const
	{
		if ( m_dwInternalVal & UID_F_RESOURCE )
			return( IsValidUID() );
		return( false );
	}
	bool IsItem() const	// Item vs. Char
	{
		if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == UID_F_ITEM )
			return( true );	// might be static in client ?
		return( false );
	}
	bool IsChar() const	// Item vs. Char
	{
		if (( m_dwInternalVal & (UID_F_ITEM|UID_F_RESOURCE)) == 0 )
			return( IsValidUID());
		return( false );
	}

	bool IsObjDisconnected() const	// Not in the game world for some reason.
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == UID_O_DISCONNECT )
			return( true );
		return( false );
	}
	bool IsObjTopLevel() const	// on the ground in the world.
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_O_DISCONNECT)) == 0 )
			return( true );	// might be static in client ?
		return( false );
	}

	bool IsItemEquipped() const
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_EQUIPPED))
			return( IsValidUID() );
		return( false );
	}
	bool IsItemInContainer() const
	{
		if (( m_dwInternalVal & (UID_F_RESOURCE|UID_F_ITEM|UID_O_DISCONNECT)) == (UID_F_ITEM|UID_O_CONTAINED))
			return( IsValidUID() );
		return( false );
	}

	void SetObjContainerFlags( DWORD dwFlags = 0 )
	{
		m_dwInternalVal = ( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM )) | dwFlags;
	}

	void SetPrivateUID( DWORD dwVal )
	{
		m_dwInternalVal = dwVal;
	}
	DWORD GetPrivateUID() const
	{
		return m_dwInternalVal;
	}

	DWORD GetObjUID() const
	{
		return( m_dwInternalVal & (UID_O_INDEX_MASK|UID_F_ITEM) );
	}
	void SetObjUID( DWORD dwVal )
	{
		// can be set to -1 by the client.
		m_dwInternalVal = ( dwVal & (UID_O_INDEX_MASK|UID_F_ITEM)) | UID_O_DISCONNECT;
	}

	bool operator == ( DWORD index ) const
	{
		return( GetObjUID() == index );
	}
	bool operator != ( DWORD index ) const
	{
		return( GetObjUID() != index );
	}
    operator DWORD () const
    {
		return( GetObjUID());
    }

	CObjBase * ObjFind() const;
	CItem * ItemFind() const; // Does item still exist or has it been deleted
	CChar * CharFind() const; // Does character still exist
};

struct CGrayUID : public CGrayUIDBase
{
	CGrayUID()
	{
		InitUID();
	}
	CGrayUID( DWORD dwVal )
	{
		SetPrivateUID( dwVal );
	}
};

#ifdef _DEBUG
	#ifdef _NO_DEBUG_ASSERTS
		#undef ASSERT
		#define ASSERT(exp)
	#endif
#endif

#endif	// _INC_GRAYCOM_H
