//
// graycom.h
// Copyright Menace Software (www.menasoft.com).
// common header file.
//

#ifndef _INC_GRAYCOM_H
#define _INC_GRAYCOM_H
#pragma once

//---------------------------SYSTEM DEFINITIONS---------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#ifdef _WIN32
// NOTE: If we want a max number of sockets we must compile for it !
	#undef FD_SETSIZE
	#define FD_SETSIZE 1024 // for max of n users ! default = 64

	#ifndef STRICT
		#define STRICT			// strict conversion of handles and pointers.
	#endif	// STRICT

	#include <io.h>
	#include <winsock2.h>
	#include <windows.h>
	#include <dos.h>
	#include <limits.h>	// INT_MAX, etc
	#include <limits>	// std::numeric_limits
	#include <conio.h>
	#include <sys/timeb.h>

	#define strcmpi		_strcmpi	// Non ANSI equiv functions ?
	#define strnicmp	_strnicmp

	extern const OSVERSIONINFO * GRAY_GetOSInfo();
	#define INT64			__int64
	#define INT32			__int32

#else	// _WIN32 else assume LINUX

	#include <sys/types.h>
	#include <sys/timeb.h>
	#include <limits.h>	// INT_MAX, etc
	#include <limits>	// std::numeric_limits

	#define HANDLE			DWORD
	#define _cdecl
	#define __cdecl
	#ifndef LONG
		#define LONG			DWORD
	#endif
	#ifndef LONGLONG
		#define LONGLONG		DWORD	// This should be 64 bit ???
	#endif
	#define WCHAR			unsigned short
	#define FAR
	#define E_FAIL			0x80004005
	#define BOOL			unsigned short
	#define INT64			int64_t
	#define INT32			int32_t

	#ifdef _BSD
		int getTimezone();
		#define _timezone		getTimezone()
	#else
		#define _timezone		timezone
	#endif

	#define PUINT			unsigned int *

	#define IsBadReadPtr( p, len )		((p) == NULL)
	#define IsBadStringPtr( p, len )	((p) == NULL)
	#define Sleep(mSec)					usleep(mSec*1000)	// arg is microseconds = 1/1000000
	#define SleepEx(mSec, unused)		usleep(mSec*1000)	// arg is microseconds = 1/1000000

	#define strcmpi		strcasecmp
	#define strnicmp	strncasecmp
	#define _vsnprintf	vsnprintf
#endif // !_WIN32

#ifdef _DEBUG
	#ifndef ASSERT
		extern void Assert_CheckFail( const char * pExp, const char *pFile, long lLine );
		#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
	#endif	// ASSERT

	#ifndef STATIC_CAST
		#define STATIC_CAST dynamic_cast
	#endif

#else	// _DEBUG

	#ifndef ASSERT
		#ifndef _WIN32
			// In linux, if we get an access violation, an exception isn't thrown.  Instead, we get
			// a SIG_SEGV, and the process cores. The following code takes care of this for us.
			extern void Assert_CheckFail( const char * pExp, const char *pFile, long lLine );
			#define ASSERT(exp)			(void)( (exp) || (Assert_CheckFail(#exp, __FILE__, __LINE__), 0) )
		#else
			#define ASSERT(exp)
		#endif
	#endif	// ASSERT

	#ifndef STATIC_CAST
		#define STATIC_CAST static_cast
	#endif

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

// Macro for fast NoCrypt Client version check
#define IsAosFlagEnabled( value )	( g_Cfg.m_iFeatureAOS & (value) )
#define IsResClient( value )		( GetAccount()->GetResDisp() >= (value) )

#define FEATURE_T2A_UPDATE 		0x01
#define FEATURE_T2A_CHAT 		0x02
#define FEATURE_LBR_UPDATE		0x01
#define FEATURE_LBR_SOUND		0x02

#define FEATURE_AOS_UPDATE_A	0x01	// AOS Monsters, Map, Skills
#define FEATURE_AOS_UPDATE_B	0x02	// Tooltip, Fightbook, Necro/paladin on creation, Single/Six char selection screen
#define FEATURE_AOS_POPUP		0x04	// PopUp Menus
#define FEATURE_AOS_DAMAGE		0x08

#define FEATURE_SE_UPDATE		0x01	// 0x00008 in 0xA9
#define FEATURE_SE_NINJASAM		0x02	// 0x00040 in feature

#define FEATURE_ML_UPDATE		0x01 	// 0x00100 on charlist and 0x0080 for feature to activate
#define FEATURE_ML_NINTHAGE		0x02	// 0x00200 feature (unlock house designer items)

#define FEATURE_KR_UPDATE		0x01	// 0x00200 in 0xA9 (KR crapness)
#define FEATURE_KR_CLIENTTYPE	0x02	// 0x00400 in 0xA9 (enables 0xE1 packet)

#define FEATURE_SA_UPDATE		0x01	// 0x10000 feature (unlock gargoyle character, housing items)
	//changed from 0x02 to 0x08 to disable current flag but still able to turn on for testing
#define FEATURE_SA_MOVEMENT		0x04	// 0x04000 on charlist (new movement packets)

#define FEATURE_EXTRA_GOTHIC	0x01	// 0x40000 feature (unlock gothic house designer items)
#define FEATURE_EXTRA_RUSTIC	0x02	// 0x80000 feature (unlock rustic house designer items)

#include "common.h"
#include "CException.h"

#include "CSocket.h"
#include "CEncrypt.h"

#include "CArray.h"
#include "CString.h"
#include "CFile.h"
#include "CScript.h"

class CTextConsole; // swapped these two includes, so need to declare this here
#include "CVarDefMap.h"
#include "ListDefContMap.h"
#include "CExpression.h"
#include "CVarFloat.h"
#include "CScriptObj.h"

class CObjBase;
class CChar;
class CItem;
class CResourceDef;

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
	CGrayUID( DWORD dw )
	{
		SetPrivateUID( dw );
	}
};

#ifdef _DEBUG
	#ifdef _NO_DEBUG_ASSERTS
		#undef ASSERT
		#define ASSERT(exp)
	#endif
#endif

#endif	// _INC_GRAYCOM_H
