//
// CObjBase.cpp
// Copyright Menace Software (www.menasoft.com).
// base classes.
//
#include "graysvr.h"	// predef header.
#include "../common/grayver.h"

bool CObjBaseTemplate::IsDeleted() const
{
	ADDTOCALLSTACK("CObjBaseTemplate::IsDeleted");
	return (!m_UID.IsValidUID() || ( GetParent() == &g_World.m_ObjDelete ));
}

int CObjBaseTemplate::IsWeird() const
{
	ADDTOCALLSTACK("CObjBaseTemplate::IsWeird");
	if ( !GetParent() )
		return 0x3101;

	if ( !IsValidUID() )
		return 0x3102;

	return 0;
}

bool GetDeltaStr( CPointMap & pt, TCHAR * pszDir )
{
	TCHAR * ppCmd[3];
	int iQty = Str_ParseCmds( pszDir, ppCmd, COUNTOF(ppCmd));

	if (iQty == 0)
		return( false );

	TCHAR chDir = toupper( ppCmd[0][0] );
	int iTmp = Exp_GetVal( ppCmd[1] );

	if ( isdigit( chDir ) || chDir == '-' )
	{
		pt.m_x += Exp_GetVal( ppCmd[0] );
		pt.m_y += iTmp;
		pt.m_z += Exp_GetVal( ppCmd[2] );
	}
	else	// a direction by name.
	{
		if ( iTmp == 0 )
			iTmp = 1;
		DIR_TYPE eDir = GetDirStr( ppCmd[0] );
		if ( eDir >= DIR_QTY )
			return( false );
		pt.MoveN( eDir, iTmp );
	}

	return( true );
}

/////////////////////////////////////////////////////////////////
// -CObjBase stuff
// Either a player, npc or item.

CObjBase::CObjBase( bool fItem )
{
	sm_iCount ++;
	m_wHue=HUE_DEFAULT;
	m_timeout.Init();

	//	Init some global variables
	m_ModAr = 0;

	if ( g_Serv.IsLoading())
	{
		// Don't do this yet if we are loading. UID will be set later.
		// Just say if this is an item or not.
		CObjBaseTemplate::SetUID(UID_O_DISCONNECT|UID_O_INDEX_MASK|(fItem ? UID_F_ITEM : 0));
	}
	else
	{
		// Find a free UID slot for this.
		SetUID(UID_CLEAR, fItem);
		ASSERT(IsValidUID());
		SetContainerFlags(UID_O_DISCONNECT);	// it is no place for now
	}

	// Put in the idle list by default. (til placed in the world)
	g_World.m_ObjNew.InsertHead( this );
}

CObjBase::~CObjBase()
{
	sm_iCount --;
	ASSERT( IsDisconnected());

	// free up the UID slot.
	SetUID( UID_UNUSED, false );
}

bool CObjBase::IsContainer() const
{
	ADDTOCALLSTACK("CObjBase::IsContainer");
	// Simple test if object is a container.
	return( dynamic_cast <const CContainer*>(this) != NULL );
}

int CObjBase::IsWeird() const
{
	ADDTOCALLSTACK("CObjBase::IsWeird");
	int iResultCode = CObjBaseTemplate::IsWeird();
	if ( iResultCode )
	{
		return( iResultCode );
	}
	if ( ! g_Serv.IsLoading())
	{
		if ( GetUID().ObjFind() != this )	// make sure it's linked both ways correctly.
		{
			return( 0x3201 );
		}
	}
	return( 0 );
}

void CObjBase::SetUID( DWORD dwIndex, bool fItem )
{
	ADDTOCALLSTACK("CObjBase::SetUID");
	// Move the serial number,
	// This is possibly dangerous if conflict arrises.

	dwIndex &= UID_O_INDEX_MASK;	// Make sure no flags in here.
	if ( IsValidUID())	// i already have a uid ?
	{
		if ( ! dwIndex )
			return;	// The point was just to make sure it was located.
		// remove the old UID.
		g_World.FreeUID( ((DWORD)GetUID()) & UID_O_INDEX_MASK );
	}

	if ( dwIndex != UID_O_INDEX_MASK )	// just wanted to remove it
	{
		dwIndex = g_World.AllocUID( dwIndex, this );
	}

	if ( fItem ) dwIndex |= UID_F_ITEM;

	CObjBaseTemplate::SetUID( dwIndex );
}

void inline CObjBase::SetNamePool_Fail( TCHAR * ppTitles )
{
	ADDTOCALLSTACK("CObjBase::SetNamePool_Fail");
	DEBUG_ERR(( "Name pool '%s' could not be found\n", ppTitles ));
	CObjBase::SetName( ppTitles );
}

bool CObjBase::SetNamePool( LPCTSTR pszName )
{
	ADDTOCALLSTACK("CObjBase::SetNamePool");
	ASSERT(pszName);

	// Parse out the name from the name pool ?
	if ( pszName[0] == '#' )
	{
		++pszName;
		TCHAR *pszTmp = Str_GetTemp();
		strcpy( pszTmp, pszName );

		TCHAR * ppTitles[2];
		Str_ParseCmds( pszTmp, ppTitles, COUNTOF(ppTitles));

		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, RES_NAMES, ppTitles[0] ))
		{
			SetNamePool_Fail( ppTitles[0] );
			return false;
		}

		// Pick a random name.
		if ( ! s.ReadKey())
		{
			SetNamePool_Fail( ppTitles[0] );
			return false;
		}
		int iCount = Calc_GetRandVal( ATOI( s.GetKey())) + 1;
		while ( iCount-- )
		{
			if ( ! s.ReadKey())
			{
				SetNamePool_Fail( ppTitles[0] );
				return false;
			}
		}

		return CObjBaseTemplate::SetName( s.GetKey());
	}

	// NOTE: Name must be <= MAX_NAME_SIZE
	TCHAR szTmp[ MAX_ITEM_NAME_SIZE + 1 ];
	int len = strlen( pszName );
	if ( len >= MAX_ITEM_NAME_SIZE )
	{
		strcpylen( szTmp, pszName, MAX_ITEM_NAME_SIZE );
		pszName = szTmp;
	}

	// Can't be a dupe name with type ?
	LPCTSTR pszTypeName = Base_GetDef()->GetTypeName();
	if ( ! strcmpi( pszTypeName, pszName ))
		pszName = "";

	return CObjBaseTemplate::SetName( pszName );
}

bool CObjBase::MoveNearObj( const CObjBaseTemplate * pObj, int iSteps, WORD wCan )
{
	ADDTOCALLSTACK("CObjBase::MoveNearObj");
	ASSERT( pObj );
	if ( pObj->IsDisconnected())	// nothing is "near" a disconnected item.
		return false;

	pObj = pObj->GetTopLevelObj();
	return( MoveNear( pObj->GetTopPoint(), iSteps, wCan ) );
}

void CObjBase::r_WriteSafe( CScript & s )
{
	ADDTOCALLSTACK("CObjBase::r_WriteSafe");
	// Write an object with some fault protection.
	DWORD uid;
	try
	{
		uid = GetUID();

		//	objects with TAG.NOSAVE set are not saved
		if ( m_TagDefs.GetKeyNum("NOSAVE", true) )
			return;

		if ( !g_Cfg.m_fSaveGarbageCollect )
		{
			if ( g_World.FixObj(this) )
				return;
		}
		r_Write(s);
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Write Object 0%x", uid);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(NULL, "Write Object 0%x", uid);
	}
}

void CObjBase::SetTimeout( int iDelayInTicks )
{
	ADDTOCALLSTACK("CObjBase::SetTimeout");
	// Set delay in TICK_PER_SEC of a sec. -1 = never.
	if ( iDelayInTicks < 0 )
		m_timeout.Init();
	else
		m_timeout = CServTime::GetCurrentTime() + iDelayInTicks;
}

void CObjBase::Sound( SOUND_TYPE id, int iOnce ) const // Play sound effect for player
{
	ADDTOCALLSTACK("CObjBase::Sound");
	// play for everyone near by.

	if (( id <= 0 ) || !g_Cfg.m_fGenericSounds )
		return;

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanHear( this, TALKMODE_OBJ ))
			continue;
		pClient->addSound( id, this, iOnce );
	}
}

void CObjBase::Effect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource, BYTE bSpeedSeconds, BYTE bLoop, bool fExplode, DWORD color, DWORD render ) const
{
	ADDTOCALLSTACK("CObjBase::Effect");
	// show for everyone near by.
	//
	// bSpeedSeconds
	// bLoop
	// fExplode

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanSee( this ))
			continue;
		pClient->addEffect( motion, id, this, pSource, bSpeedSeconds, bLoop, fExplode, color, render );
	}
}


void CObjBase::Emote( LPCTSTR pText, CClient * pClientExclude, bool fForcePossessive )
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase * pObjTop = STATIC_CAST <CObjBase*>( GetTopLevelObj());
	ASSERT(pObjTop);

	TCHAR *pszThem = Str_GetTemp();
	TCHAR *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar())
	{
		// Someone has this equipped.

		if ( pObjTop != this )
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_1 ), (LPCTSTR) pObjTop->GetName(), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_2 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_3 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_4 ), pText);
		}
		else
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_5 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_6 ), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_7 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_RED, TALKMODE_EMOTE);
}

void CObjBase::Speak( LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK("CObjBase::Speak");
	g_World.Speak( this, pText, wHue, mode, font );
}

void CObjBase::SpeakUTF8( LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8");
	// convert UTF8 to UNICODE.
	NCHAR szBuffer[ MAX_TALK_BUFFER ];
	int iLen = CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pText, -1 );
	g_World.SpeakUNICODE( this, szBuffer, wHue, mode, font, lang );
}

void CObjBase::SpeakUTF8Ex( const NWORD * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8Ex");
	g_World.SpeakUNICODE( this, pText, wHue, mode, font, lang );
}

bool CObjBase::MoveNear( CPointMap pt, int iSteps, WORD wCan )
{
	ADDTOCALLSTACK("CObjBase::MoveNear");
	// Move to nearby this other object.
	// Actually move it within +/- iSteps

	DIR_TYPE dir = (DIR_TYPE) Calc_GetRandVal( DIR_QTY );
	for (; iSteps > 0 ; --iSteps )
	{
		// Move to the right or left?
		CPointBase pTest = pt;	// Save this so we can go back to it if we hit a blocking object.
		pt.Move( dir );
		dir = GetDirTurn( dir, Calc_GetRandVal(3)-1 );	// stagger ?
		// Put the item at the correct Z point
		WORD wBlockRet = wCan;
		if (( IsSetEF( EF_WalkCheck ) ) && ( ! IsSetEF( EF_NewPositionChecks ) ))
			pt.m_z = g_World.GetHeightPoint_New( pt, wBlockRet, true );
		else if ( ! IsSetEF( EF_NewPositionChecks ) )
			pt.m_z = g_World.GetHeightPoint( pt, wBlockRet, true );
		if ( wBlockRet &~ wCan )
		{
			// Hit a block, so go back to the previous valid position
			pt = pTest;
			break;	// stopped
		}
	}

	//	deny spawning outside the house, etc
	if ( IsChar() )
	{
		WORD		wBlockFlags	= 0;
		((CChar*)this)->m_zClimbHeight = 0;
		if ( ((CChar*)this)->CanMoveWalkTo(pt, false, true, dir) == NULL ) {
			return( false );
		}
	}

	return MoveTo(pt);
}

void CObjBase::UpdateObjMessage( LPCTSTR pTextThem, LPCTSTR pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode ) const
{
	ADDTOCALLSTACK("CObjBase::UpdateObjMessage");
	// Show everyone a msg coming form this object.

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;

		pClient->addBarkParse(( pClient->GetChar() == this )? pTextYou : pTextThem, this, wHue, mode, FONT_NORMAL );
	}
}

void CObjBase::UpdateCanSee( const CCommand * pCmd, int iLen, CClient * pClientExclude ) const
{
	ADDTOCALLSTACK("CObjBase::UpdateCanSee");
	// Send this update message to everyone who can see this.
	// NOTE: Need not be a top level object. CanSee() will calc that.
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;
		pClient->xSendPkt( pCmd, iLen );
	}
}

TRIGRET_TYPE CObjBase::OnHearTrigger( CResourceLock & s, LPCTSTR pszCmd, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CObjBase::OnHearTrigger");
	// Check all the keys in this script section.
	// look for pattern or partial trigger matches.
	// RETURN:
	//  TRIGRET_ENDIF = no match.
	//  TRIGRET_DEFAULT = found match but it had no RETURN
	CScriptTriggerArgs Args( pszCmd );
	Args.m_iN1 = mode;
	Args.m_iN2 = wHue;

	bool fMatch = false;

	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead("ON",2))
		{
			// Look for some key word.
			_strupr( s.GetArgStr());
			if ( Str_Match( s.GetArgStr(), pszCmd ) == MATCH_VALID )
				fMatch = true;
			continue;
		}

		if ( ! fMatch )
			continue;	// look for the next "ON" section.

		TRIGRET_TYPE iRet = CObjBase::OnTriggerRunVal( s, TRIGRUN_SECTION_EXEC, pSrc, &Args );
		if ( iRet != TRIGRET_RET_FALSE )
			return( iRet );

		fMatch = false;
	}

	mode	= (TALKMODE_TYPE) Args.m_iN1;
	return( TRIGRET_ENDIF );	// continue looking.
}

enum OBR_TYPE
{
	OBR_ROOM,
	OBR_SECTOR,
	OBR_TOPOBJ,
	OBR_TYPEDEF,
	OBR_QTY,
};

LPCTSTR const CObjBase::sm_szRefKeys[OBR_QTY+1] =
{
	"ROOM",
	"SECTOR",
	"TOPOBJ",
	"TYPEDEF",
	NULL,
};

bool CObjBase::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CObjBase::r_GetRef");
	int i = FindTableHeadSorted( pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		pszKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(pszKey);
		switch (i)
		{
			case OBR_ROOM:
				pRef = GetTopLevelObj()->GetTopPoint().GetRegion( REGION_TYPE_ROOM );
				return( true );
			case OBR_SECTOR:
				pRef = GetTopLevelObj()->GetTopSector();
				return( true );
			case OBR_TOPOBJ:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = dynamic_cast <CObjBase*>(GetTopLevelObj());
				return( true );
			case OBR_TYPEDEF:
				pRef = Base_GetDef();
				return( true );
		}

	}

	return( CScriptObj::r_GetRef(pszKey, pRef) );
}

enum OBC_TYPE
{
	#define ADD(a,b) OC_##a,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	OC_QTY,
};

LPCTSTR const CObjBase::sm_szLoadKeys[OC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	NULL,
};

bool CObjBase::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CObjBase::r_WriteVal");
	EXC_TRY("WriteVal");
	LPCSTR	pszArgs	= NULL;

	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		// RES_FUNCTION call
		// Is it a function returning a value ? Parse args ?
		if ( ( pszArgs = strchr( pszKey, ' ' ) ) )
		{
			pszArgs++;
			SKIP_SEPARATORS(pszArgs);
		}

		CScriptTriggerArgs Args( pszArgs ? pszArgs : "" );
		if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
			return true;

		// Just try to default to something reasonable ?
		// Even though we have not really specified it correctly !

		// WORLD. ?
		if ( g_World.r_WriteVal( pszKey, sVal, pSrc ))
			return( true );


		// TYPEDEF. ?
		if ( Base_GetDef()->r_WriteVal( pszKey, sVal, pSrc ))
			return( true );

		return(	CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}

	bool	fZero	= false;
	switch (index)
	{
		case OC_CANSEE:
		case OC_CANSEELOS:
		case OC_CANSEELOSFLAG:
			{
				bool bCanSee = ( index == OC_CANSEE );
				bool bFlags = ( index == OC_CANSEELOSFLAG );
				CChar *pChar = pSrc->GetChar();

				pszKey += (( bCanSee ) ? ( 6 ) : (( bFlags  ) ? ( 13 ) : ( 9 )));
				SKIP_SEPARATORS(pszKey);
				GETNONWHITESPACE(pszKey);

				int flags = 0;
				char * pszFlags;
				if ( bFlags )
				{
					short int length = strlen( pszKey )+1;
					char * pszTmp = new char[length];
					strcpylen( pszTmp, pszKey, length );
					Str_ParseCmds( pszTmp, &pszFlags, 1, "," );
					pszKey += strlen( pszFlags )+1;
					flags = Exp_GetVal( pszFlags );
					delete[] pszTmp;
				}
				if ( *pszKey )		// has an argument - UID to see(los) or POS to los only
				{
					CPointMap pt;
					CGrayUID uid;
					CObjBase *pObj;

					if ( !bCanSee )
						pt = g_Cfg.GetRegionPoint(pszKey);

					if ( bCanSee || !pt.IsValidPoint() )
					{
						uid = Exp_GetVal( pszKey );
						pObj = uid.ObjFind();
						if ( !bCanSee && pObj )
							pt = pObj->GetTopPoint();
					}

					pChar = GetUID().CharFind();

					if ( pChar )
						sVal.FormatVal( bCanSee ? pChar->CanSee(pObj) : pChar->CanSeeLOS(pt, NULL, pChar->GetVisualRange(), flags) );
					else
						sVal.FormatVal(0);

				}
				else if ( !pChar )		// no char -> no see
					sVal.FormatVal(0);
				else					// standart way src TO current object
					sVal.FormatVal( bCanSee ? pChar->CanSee(this) : pChar->CanSeeLOS(this, flags) );
			}
			break;
		case OC_COLOR:
			sVal.FormatHex( GetHue());
			break;
		case OC_COMPLEXITY:
			{
			if ( IsDisconnected() || !GetTopLevelObj()->GetTopPoint().IsValidPoint() )
				return false;
			return GetTopLevelObj()->GetTopSector()->r_WriteVal( pszKey, sVal, pSrc );
			}
		case OC_DISTANCE:
			{
				pszKey	+= 8;
				SKIP_SEPARATORS( pszKey );
				GETNONWHITESPACE( pszKey );
				CObjBase *	pObj	= pSrc->GetChar();

				CObjBase	* pThis	= this;
				if ( !IsTopLevel() )
					pThis	= dynamic_cast <CObjBase*>( GetTopLevelObj() );
				if ( !pThis )
					return false;

				if ( *pszKey )
				{
					CPointMap	pt	= g_Cfg.GetRegionPoint( pszKey );

					if ( pt.IsValidPoint() )
					{
						if ( !pThis->GetTopPoint().IsValidPoint() )
							return false;
						else
							sVal.FormatVal( pThis->GetTopDist( pt ) );
						return true;
					}

					CGrayUID	uid			= Exp_GetVal( pszKey );
					SKIP_SEPARATORS( pszKey ); GETNONWHITESPACE( pszKey );
					pObj	= uid.ObjFind();
				}

				if ( pObj && !pObj->IsTopLevel() )
						pObj	= dynamic_cast <CObjBase*>( pObj->GetTopLevelObj() );
				if ( !pObj )
					return false;
				sVal.FormatVal( pThis->GetDist( pObj ) );
			}
			break;
		case OC_EVENTS:
			m_OEvents.WriteResourceRefList( sVal );
			break;
		case OC_ISCHAR:
			sVal.FormatVal( IsChar());
			break;
		case OC_ISEVENT:
			if ( pszKey[7] != '.' )
				return( false );
			pszKey += 8;
			sVal = ( m_OEvents.FindResourceName(RES_EVENTS, pszKey) >= 0 ) ? "1" : "0";
			return true;
		case OC_ISITEM:
			sVal.FormatVal( IsItem());
			break;
		case OC_ISCONT:
			sVal.FormatVal( IsContainer());
			break;
		case OC_ISNEARTYPETOP:
		case OC_ISNEARTYPE:
			{
				bool fP = false;
				pszKey	+= ( index == OC_ISNEARTYPETOP ) ? 13 : 10;
				if ( !strnicmp( pszKey, ".P", 2 ) )
				{
					fP	= true;
					pszKey	+= 2;
				}
				SKIP_SEPARATORS( pszKey );
				SKIP_ARGSEP( pszKey );

				if ( !GetTopPoint().IsValidPoint() )
					sVal.FormatVal( 0 );
				else
				{
					int iType = g_Cfg.ResourceGetIndexType( RES_TYPEDEF, pszKey );
					int iDistance;

					SKIP_IDENTIFIERSTRING( pszKey );
					SKIP_SEPARATORS( pszKey );
					SKIP_ARGSEP( pszKey );

					if ( !*pszKey )
						iDistance	= 0;
					else
						iDistance	= Exp_GetVal( pszKey );

					if ( fP )
					{
						CPointMap pt = ( index == OC_ISNEARTYPETOP ) ? ( g_World.FindTypeNear_Top(GetTopPoint(), (IT_TYPE) iType, iDistance ) ) : ( g_World.FindItemTypeNearby(GetTopPoint(), (IT_TYPE) iType, iDistance ) );

						if ( !pt.IsValidPoint() )
							sVal.FormatVal( 0 );
						else
							sVal = pt.WriteUsed();
					}
					else
						sVal.FormatVal( ( index == OC_ISNEARTYPETOP ) ? ( g_World.IsTypeNear_Top(GetTopPoint(), (IT_TYPE) iType, iDistance ) ) : ( g_World.IsItemTypeNear(GetTopPoint(), (IT_TYPE) iType, iDistance ) ) );
				}
				return true;
			}
			break;
		case OC_ISPLAYER:
			{
				CChar * pChar = dynamic_cast<CChar*>(this);
				if ( pChar )
					sVal.FormatVal( (pChar->m_pPlayer == NULL) ? 0 : 1 );
				else
					sVal.FormatVal( 0 );
				return( true );
			}
		case OC_ISDIALOGOPEN:
			{
				pszKey += 12;
				SKIP_SEPARATORS( pszKey );
				GETNONWHITESPACE( pszKey );
				CChar * pCharToCheck = dynamic_cast<CChar*>(this);
				CClient * pClientToCheck = (pCharToCheck && pCharToCheck->IsClient()) ? (pCharToCheck->GetClient()) : NULL ;

				if ( pClientToCheck )
				{
					int context = GETINTRESOURCE( (DWORD) g_Cfg.ResourceGetIDType( RES_DIALOG, pszKey ) );
					CClient::OpenedGumpsMap_t::iterator itGumpFound = pClientToCheck->m_mapOpenedGumps.find( context );

					if ( itGumpFound != pClientToCheck->m_mapOpenedGumps.end() )
					{
						sVal.FormatVal( (*itGumpFound).second );
					}
					else
					{
						sVal.FormatVal( 0 );
					}
				}
				else
				{
					sVal.FormatVal( 0 );
				}

				return( true );
			}
		case OC_ISARMOR:
			{
			pszKey += 7;
			SKIP_SEPARATORS( pszKey );
			GETNONWHITESPACE( pszKey );
			CItem * pItem = NULL;
			if ( *pszKey )
			{
				char * pszArgs = Str_GetTemp();
				strcpylen( pszArgs, pszKey, strlen( pszKey ) + 1 );

				CGrayUID uid = Exp_GetVal( pszKey );
				pItem = dynamic_cast<CItem*> (uid.ObjFind());
				if ( pItem == NULL )
				{
					ITEMID_TYPE id = (ITEMID_TYPE) g_Cfg.ResourceGetID( RES_ITEMDEF, (LPCTSTR&)pszArgs ).GetResIndex();
					CItemBase * pItemDef = CItemBase::FindItemBase( id );
					if ( pItemDef )
					{
						sVal.FormatVal( CItemBase::IsTypeArmor( pItemDef->GetType() ) );
						break;
					}
				}
				sVal.FormatVal( (( pItem ) ? ( pItem->IsTypeArmor() ) : ( 0 )) );
				break;
			}
			pItem = dynamic_cast<CItem*> (this);
			sVal.FormatVal( (( pItem ) ? ( pItem->IsTypeArmor() ) : ( 0 )) );
			break;
			}
		case OC_ISWEAPON:
			{
			pszKey += 8;
			SKIP_SEPARATORS( pszKey );
			GETNONWHITESPACE( pszKey );
			CItem * pItem = NULL;
			if ( *pszKey )
			{
				char * pszArgs = Str_GetTemp();
				strcpylen( pszArgs, pszKey, strlen( pszKey ) + 1 );

				CGrayUID uid = Exp_GetVal( pszKey );
				pItem = dynamic_cast<CItem*> (uid.ObjFind());
				if ( pItem == NULL )
				{
					ITEMID_TYPE id = (ITEMID_TYPE) g_Cfg.ResourceGetID( RES_ITEMDEF, (LPCTSTR&)pszArgs ).GetResIndex();
					CItemBase * pItemDef = CItemBase::FindItemBase( id );
					if ( pItemDef )
					{
						sVal.FormatVal( CItemBase::IsTypeWeapon( pItemDef->GetType() ) );
						break;
					}
				}
				sVal.FormatVal( (( pItem ) ? ( pItem->IsTypeWeapon() ) : ( 0 )) );
				break;
			}
			pItem = dynamic_cast<CItem*> (this);
			sVal.FormatVal( (( pItem ) ? ( pItem->IsTypeWeapon() ) : ( 0 )) );
			break;
			}
		case OC_MAP:
			sVal.FormatVal( GetUnkPoint().m_map);
			break;
		case OC_MODAR:
		case OC_MODAC:
			sVal.FormatVal( m_ModAr );
			break;
		case OC_NAME:
			sVal = GetName();
			break;
		case OC_P:
			if ( pszKey[1] == '.' )
			{
				return( GetUnkPoint().r_WriteVal( pszKey+2, sVal ));
			}
			sVal = GetUnkPoint().WriteUsed();
			break;
		case OC_TAG0:
			fZero	= true;
			pszKey++;
		case OC_TAG:			// "TAG" = get/set a local tag.
			{
				if ( pszKey[3] != '.' )
					return( false );
				pszKey += 4;

				CVarDefCont *	pVarKey	= m_TagDefs.GetKey( pszKey );
				if ( !pVarKey )
					sVal	= Base_GetDef()->m_TagDefs.GetKeyStr( pszKey, fZero );
				else
					sVal = pVarKey->GetValStr();
			}
			return( true );
		case OC_TIMER:
			sVal.FormatVal( GetTimerAdjusted());
			break;
		case OC_TIMERD:
			sVal.FormatVal( GetTimerDAdjusted() );
			break;
		case OC_TOPOBJ:
			if ( pszKey[6] == '.' )
			{
				return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
			}
			sVal.FormatHex(GetTopLevelObj()->GetUID());
			break;
		case OC_UID:
			if ( pszKey[3] == '.' )
				return(	CScriptObj::r_WriteVal( pszKey, sVal, pSrc ) );
		case OC_SERIAL:
			sVal.FormatHex( GetUID());
			break;
		case OC_SEXTANTP:
			{
				pszKey += 8;
				SKIP_SEPARATORS( pszKey ); GETNONWHITESPACE( pszKey );

				CPointMap pt;
				if ( *pszKey )
					pt = g_Cfg.GetRegionPoint( pszKey );
				else
					pt = this->GetUnkPoint();

				if ( !pt.IsValidPoint() )
					return( false );

				sVal = g_Cfg.Calc_MaptoSextant(pt);
			} break;
		case OC_VERSION:
			sVal = GRAY_VERSION;
			break;
		case OC_WEIGHT:
			sVal.FormatVal( GetWeight());
			break;
		case OC_Z:
			sVal.FormatVal( GetUnkZ());
			break;
		case OC_TAGAT:
			{
 				pszKey += 5;	// eat the 'TAGAT'
 				if ( *pszKey == '.' )	// do we have an argument?
 				{
 					SKIP_SEPARATORS( pszKey );
 					int iQty = Exp_GetVal( pszKey );
					if ( iQty < 0 || iQty >= m_TagDefs.GetCount() )
 						return( false );	// tryig to get non-existant tag

 					CVarDefCont * pTagAt = m_TagDefs.GetAt( iQty );
 					if ( !pTagAt )
 						return( false );	// tryig to get non-existant tag

 					SKIP_SEPARATORS( pszKey );
 					if ( ! *pszKey )
 					{
 						sVal.Format( "%s=%s", (LPCTSTR) pTagAt->GetKey(), (LPCTSTR) pTagAt->GetValStr() );
 						return( true );
 					}
 					if ( strnicmp( pszKey, "KEY", 3 ))	// key?
 					{
 						sVal = (LPCTSTR) pTagAt->GetKey();
 						return( true );
 					}
 					if ( strnicmp( pszKey, "VAL", 3 ))	// val?
 					{
 						sVal = pTagAt->GetValStr();
 						return( true );
 					}
 				}

			return( false );
			}
			break;
		case OC_TAGCOUNT:
			sVal.FormatVal( m_TagDefs.GetCount() );
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CObjBase::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CObjBase::r_LoadVal");
	// load the basic stuff.
	EXC_TRY("LoadVal");
	// we're using FindTableSorted so we must do this here.
	// Using FindTableHeadSorted instead would result in keywords
	// starting with "P" not working, for instance :)

	if ( s.IsKeyHead("TAG.", 4) )
	{
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey()+4, fQuoted, s.GetArgStr(&fQuoted), false);
		return( true );
	}
	else if ( s.IsKeyHead("TAG0.", 5) )
	{
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey()+5, fQuoted, s.GetArgStr(&fQuoted), true);
		return true;
	}

	int index = FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		return( CScriptObj::r_LoadVal(s));
	}

	switch ( index )
	{
		case OC_COLOR:
			if ( ! strcmpi( s.GetArgStr(), "match_shirt" ) ||
				! strcmpi( s.GetArgStr(), "match_hair" ))
			{
				CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
				if ( pChar )
				{
					CItem * pHair = pChar->LayerFind( ! strcmpi( s.GetArgStr()+6, "shirt" ) ? LAYER_SHIRT : LAYER_HAIR );
					if ( pHair )
					{
						m_wHue = pHair->GetHue();
						break;
					}
				}
				m_wHue = HUE_GRAY;
				break;
			}
			RemoveFromView();
			m_wHue = (HUE_TYPE) s.GetArgVal();
			Update();
			break;
		case OC_EVENTS:
			return( m_OEvents.r_LoadVal( s, RES_EVENTS ));
		case OC_MAP:
			// Move to another map
			if ( ! IsTopLevel())
				return( false );
			{
				CPointMap pt = GetTopPoint();
				pt.m_map = s.GetArgVal();

				//	is the desired mapplane allowed?
				if ( !g_MapList.m_maps[pt.m_map] )
					return false;

				MoveTo(pt);
			}
			break;
		case OC_MODAR:
		case OC_MODAC:
			{
				m_ModAr = s.GetArgVal();
				CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
				if ( pChar && pChar->IsChar() )
				{
					pChar->m_defense = pChar->CalcArmorDefense();
					pChar->UpdateStatsFlag();
				}
			}
			break;
		case OC_NAME:
			SetName( s.GetArgStr());
			break;
		case OC_P:	// Must set the point via the CItem or CChar methods.
			return( false );
		case OC_TIMER:
			SetTimeout( s.GetArgVal() * TICK_PER_SEC );
			break;
		case OC_TIMERD:
			SetTimeout( s.GetArgVal());
			break;
		case OC_UID:
		case OC_SERIAL:
			// Don't set container flags through this.
			SetUID( s.GetArgVal(), (dynamic_cast <CItem*>(this)) ? true : false );
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CObjBase::r_Write");
	s.WriteKeyHex( "SERIAL", GetUID());
	if ( IsIndividualName() )
		s.WriteKey( "NAME", GetIndividualName());
	if ( m_wHue != HUE_DEFAULT )
		s.WriteKeyHex( "COLOR", GetHue());
	if ( m_timeout.IsTimeValid() )
		s.WriteKeyVal( "TIMER", GetTimerAdjusted());
	if ( m_ModAr )
		s.WriteKeyVal("MODAR", m_ModAr);

	m_TagDefs.r_WritePrefix(s, "TAG");
	m_OEvents.r_Write(s, "EVENTS");
}

enum OV_TYPE
{
	#define ADD(a,b) OV_##a,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	OV_QTY,
};

LPCTSTR const CObjBase::sm_szVerbKeys[OV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	NULL,
};

bool CObjBase::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CObjBase::r_Verb");
	EXC_TRY("Verb");
	LPCTSTR	pszKey = s.GetKey();
	ASSERT(pSrc);
	int	index;

	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}
	
	CGString sVal;
	CScriptTriggerArgs Args( s.GetArgRaw() );
	if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
		return true;

	if ( !strnicmp( pszKey, "TARGET", 6 ) )
		index = OV_TARGET;
	else
		index = FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );

	if ( index < 0 )
		return CScriptObj::r_Verb(s, pSrc);

	CChar * pCharSrc = pSrc->GetChar();
	CClient * pClientSrc = (pCharSrc && pCharSrc->IsClient()) ? (pCharSrc->GetClient()) : NULL ;

	switch (index)
	{
		case OV_DAMAGE:	//	"Amount,SourceFlags,SourceCharUid" = do me some damage.
			{
				EXC_SET("DAMAGE");
				int piCmd[3];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty < 1 )
					return( false );
				if ( iArgQty > 2 )	// Give it a new source char UID
				{
					CObjBaseTemplate * pObj = CGrayUID( piCmd[2] ).ObjFind();
					if ( pObj )
					{
						pObj = pObj->GetTopLevelObj();
					}
					pCharSrc = dynamic_cast <CChar*>(pObj);
				}
				OnTakeDamage( piCmd[0],
					pCharSrc,
					( iArgQty > 1 ) ? piCmd[1] : ( DAMAGE_HIT_BLUNT | DAMAGE_GENERAL ));
			}
			break;

		case OV_EDIT:
			{
				EXC_SET("EDIT");
				// Put up a list of items in the container. (if it is a container)
				if ( pClientSrc == NULL )
					return( false );
				pClientSrc->m_Targ_Text = s.GetArgStr();
				pClientSrc->Cmd_EditItem( this, -1 );
			}
			break;
		case OV_EFFECT: // some visual effect.
			{
				EXC_SET("EFFECT");
				int piCmd[7];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty < 2 )
					return( false );
				CObjBase *	pThis	= this;
				//DEBUG_ERR(("this->GetUID() 0%x \n", (DWORD)this->GetUID()));
				if ( piCmd[0] == -1 )
				{
					if ( pCharSrc )
					{
						piCmd[0]	= EFFECT_BOLT;
						pThis		= pCharSrc;
						pCharSrc	= dynamic_cast <CChar*>(this);
					}

				}
				//DEBUG_ERR(("this->GetUID() 0%x pThis->GetUID() 0%x pCharSrc->GetUID() 0%x\n",(DWORD)this->GetUID(),(DWORD)pThis->GetUID(),(DWORD)pCharSrc->GetUID()));
				pThis->Effect( (EFFECT_TYPE) piCmd[0], (ITEMID_TYPE) RES_GET_INDEX(piCmd[1]),
					pCharSrc,
					(iArgQty>=3)? piCmd[2] : 5,		// BYTE bSpeedSeconds = 5,
					(iArgQty>=4)? piCmd[3] : 1,		// BYTE bLoop = 1,
					(iArgQty>=5)? piCmd[4] : false,	// bool fExplode = false
					(iArgQty>=6)? piCmd[5] : 0,		// hue
					(iArgQty>=7)? piCmd[6] : 0		// render mode
					);
			}
			break;
		case OV_EMOTE:
			EXC_SET("EMOTE");
			Emote( s.GetArgStr());
			break;
		case OV_FLIP:
			EXC_SET("FLIP");
			Flip();
			break;
		case OV_INPDLG:
			// "INPDLG" verb maxchars
			// else assume it was a property button.
			{
				EXC_SET("INPDLG");
				if ( pClientSrc == NULL )
					return( false );

				TCHAR *Arg_ppCmd[2];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				CGString sOrgValue;
				if ( ! r_WriteVal( Arg_ppCmd[0], sOrgValue, pSrc ))
					sOrgValue = ".";

				pClientSrc->m_Targ_Text = Arg_ppCmd[0];	// The attribute we want to edit.

				int iMaxLength = iQty > 1 ? ATOI(Arg_ppCmd[1]) : 1;

				CGString sPrompt;
				sPrompt.Format( "%s (# = default)", (LPCTSTR) Arg_ppCmd[0] );
				pClientSrc->addGumpInpVal( true, INPVAL_STYLE_TEXTEDIT,
					iMaxLength,	sPrompt, sOrgValue, this );
			}
			break;
		case OV_MENU:
			{
				EXC_SET("MENU");
				if ( pClientSrc == NULL )
					return( false );
				pClientSrc->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()), this );
			}
			break;
		case OV_MESSAGE:	//put info message (for pSrc client only) over item.
		case OV_MSG:
			{
				EXC_SET("MESSAGE or MSG");
				if ( pCharSrc == NULL )
					UpdateObjMessage(s.GetArgStr(), s.GetArgStr(), NULL, HUE_TEXT_DEF, IsChar() ? TALKMODE_EMOTE : TALKMODE_ITEM);
				else
					pCharSrc->ObjMessage(s.GetArgStr(), this);
			}
			break;
		case OV_MESSAGEUA:
			{
				EXC_SET("MESSAGEUA");
				if ( pClientSrc == NULL )
					return( false );

				TCHAR * pszArgs[5];
				NCHAR ncBuffer[ MAX_TALK_BUFFER ];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
				if ( iArgQty < 5 )
					break;

				CvtSystemToNUNICODE( ncBuffer, COUNTOF( ncBuffer ), pszArgs[4], -1 );
				pClientSrc->addBarkUNICODE( ncBuffer, this,
					(HUE_TYPE) ( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE) ( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE) ( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
				break;
			}
		case OV_MOVE:
			// move without restriction. east,west,etc. (?up,down,)
			EXC_SET("MOVE");
			if ( IsTopLevel())
			{
				CPointMap pt = GetTopPoint();
				if ( ! GetDeltaStr( pt, s.GetArgStr()))
					return( false );
				MoveTo( pt );
				Update();
			}
			break;
		case OV_MOVENEAR:
			{
				EXC_SET("MOVENEAR");
				CObjBase *	pObjNear;
				int piCmd[4];

				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd) );
				if ( iArgQty == 0 )
					return false;
				if ( iArgQty < 3 )
					piCmd[2]	= 1;
				if ( iArgQty < 2 )
					piCmd[1]	= 1;

				CGrayUID	uid	= piCmd[0];
				pObjNear	= uid.ObjFind();
				if ( !pObjNear )
					return false;
				MoveNearObj( pObjNear, piCmd[1] );
				if ( piCmd[2] )
					Update();
			}
			break;
		case OV_NUDGEDOWN:
			EXC_SET("NUDGEDOWN");
			if ( IsTopLevel())
			{
				int zdiff = s.GetArgVal();
				SetTopZ( GetTopZ() - ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_NUDGEUP:
			EXC_SET("NUDGEUP");
			if ( IsTopLevel())
			{
				int zdiff = s.GetArgVal();
				SetTopZ( GetTopZ() + ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_MOVETO:
		case OV_P:
			EXC_SET("P or MOVETO");
			MoveTo( g_Cfg.GetRegionPoint( s.GetArgStr()));
			break;
		case OV_PROMPTCONSOLE:
			{
				EXC_SET("PROMPTCONSOLE");
				if ( pClientSrc == NULL )
					return( false );

				TCHAR * pszArgs[2];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 2 );
				if ( iArgQty == 0 )
					break;
					
				pClientSrc->addPromptConsoleFunction( pszArgs[0], pszArgs[1] );
			}
			break;
		case OV_INFO:
			EXC_SET("INFO");
			if ( ! pClientSrc )
				return( false );
			return pClientSrc->addGumpDialogProps( GetUID());

		case OV_REMOVE:	//remove this object now.
			EXC_SET("REMOVE");
			Delete();
			return( true );
		case OV_REMOVEFROMVIEW:
			EXC_SET("REMOVEFROMVIEW");
			RemoveFromView( NULL, false );	// remove this item from all clients.
			return( true );
		case OV_RESENDTOOLTIP:
			EXC_SET("RESENDTOOLTIP");
			ResendTooltip(s.GetArgVal());
			return( true );
		case OV_SAY: //speak so everyone can here
			EXC_SET("SAY");
			Speak( s.GetArgStr());
			break;

		case OV_SAYU:
			// Speak in unicode from the UTF8 system format.
			EXC_SET("SAYU");
			SpeakUTF8( s.GetArgStr(), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL );
			break;

		case OV_SAYUA:
			// This can have full args. SAYUA Color, Mode, Font, Lang, Text Text
			{
				EXC_SET("SAYUA");
				TCHAR * pszArgs[5];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
				if ( iArgQty < 5 )
					break;

				SpeakUTF8( pszArgs[4],
					(HUE_TYPE) ( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE) ( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE) ( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
			}
			break;

		case OV_SOUND:
			{
				EXC_SET("SOUND");
				int piCmd[2];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				Sound( piCmd[0], ( iArgQty > 1 ) ? piCmd[1] : 1 );
			}
			break;
		case OV_SPELLEFFECT:	// spell, strength, noresist
			{
				EXC_SET("SPELLEFFECT");
				int piCmd[4];
				int iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				CItem		* pItemSrc	= NULL;
				switch( iArgs )
				{
				case 4:
					{
						CGrayUID	uid	= (DWORD) piCmd[3];
						pItemSrc	= uid.ItemFind();
					}
				case 3:
					if ( piCmd[2] == 1 )
					{
						pCharSrc = dynamic_cast <CChar*> (this);
					}
					else
					{
						CGrayUID	uid	= (DWORD) piCmd[2];
						pCharSrc	= uid.CharFind();
					}
					break;
				default:
					break;
				}
				OnSpellEffect( (SPELL_TYPE) RES_GET_INDEX( piCmd[0] ), pCharSrc, piCmd[1], pItemSrc );
			}
			break;
		case OV_TAGLIST:
			EXC_SET("TAGLIST");
			if ( ! strcmpi( s.GetArgStr(), "log" ))
				pSrc = (CTextConsole *)&g_Serv;
			m_TagDefs.DumpKeys(pSrc, "TAG.");
			break;
		case OV_TARGET:
			{
				EXC_SET("TARGET");
				if ( pClientSrc == NULL )
					return( false );
				pszKey	+= 6;
				bool		fGround		= false;
				bool		fCrim		= false;
				bool		fFunction	= false;
				bool		fMulti		= false;
				char		low = tolower(*pszKey);

				while (( low >= 'a' ) && ( low <= 'z' ))
				{
					if ( low == 'f' )
						fFunction = true;
					else if ( low == 'g' )
						fGround = true;
					else if ( low == 'w' )
						fCrim = true;
					else if ( low == 'm' )
						fMulti = true;

					low = tolower(*(++pszKey));
				}

				pClientSrc->m_Targ_UID = GetUID();
				pClientSrc->m_tmUseItem.m_pParent = GetParent();	// Cheat Verify

				if ( fFunction )
				{
					if ( fMulti )
					{
						if ( IsStrEmpty(s.GetArgStr()) )
							break;
						char * ppArg[2];
						Str_ParseCmds( s.GetArgStr(), ppArg, 2, "," );
						if ( !IsStrNumeric( ppArg[1] ))
							DEBUG_ERR(("Invalid argument in Target Multi\n"));
						ITEMID_TYPE itemid = (ITEMID_TYPE) Exp_GetVal(ppArg[1]);
						pClientSrc->addTargetFunctionMulti( ppArg[0], itemid, fGround );
					}
					else
						pClientSrc->addTargetFunction( s.GetArgStr(), fGround, fCrim );
				}
				else
				{
					if ( fMulti )
					{
						if ( !IsStrNumeric( s.GetArgStr() ))
							DEBUG_ERR(("Invalid argument in Target Multi\n"));
						LPCTSTR arg = s.GetArgStr();
						ITEMID_TYPE itemid = (ITEMID_TYPE) Exp_GetVal(arg);
						pClientSrc->addTargetItems( CLIMODE_TARG_USE_ITEM, itemid, fGround );
					}
					else
						pClientSrc->addTarget( CLIMODE_TARG_USE_ITEM, s.GetArgStr(), fGround, fCrim );
				}
			}
			break;

	case OV_TIMERF:
			{
				EXC_SET("TIMERF");
				if ( !strnicmp( s.GetArgStr(), "CLEAR", 5 ) )
				{
					g_World.m_TimedFunctions.Erase(GetUID());
				}
				else
				{
					char *p = s.GetArgRaw();
					int el = Exp_GetVal(p);
					if ( el < 0 )
					{
						g_Log.EventError("TimerF function invalid parameter '%i'.\n", el);
						return false;
					}
					else
					{
						SKIP_ARGSEP(p);
						if ( !*p || ( strlen(p) >= 1024 ))
						{
							g_Log.EventError("TimerF function name empty or args too long - total length must be less than 1024 characters\n");
							return false;
						}
						else
						{
							g_World.m_TimedFunctions.Add(GetUID(), el, p);
						}
					}
				}
			}
			break;

		case OV_DIALOG:
		case OV_SDIALOG:
			{
				EXC_SET("DIALOG or SDIALOG");
				if ( pClientSrc == NULL )
					return( false );

				TCHAR *	Arg_ppCmd[3];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				if ( iQty < 1 )
					return( false );

				if ( index == OV_SDIALOG )
				{
					int context = GETINTRESOURCE( (DWORD) g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ) );
					CClient::OpenedGumpsMap_t::iterator itGumpFound = pClientSrc->m_mapOpenedGumps.find( context );

					if ( pCharSrc && (( itGumpFound != pClientSrc->m_mapOpenedGumps.end() ) && ( (*itGumpFound).second > 0 )) )
						break;
				}
				pClientSrc->Dialog_Setup( CLIMODE_DIALOG, g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ),
					iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0, this, Arg_ppCmd[2] );
			}
			break;
		case OV_DIALOGCLOSE:
			{
				EXC_SET("DIALOGCLOSE");
				if ( pClientSrc == NULL )
					return( false );

				TCHAR *	Arg_ppCmd[2];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				if ( iQty < 1 )
					return false;

				pClientSrc->Dialog_Close( this, g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ), iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0 );
			}
			break;
		case OV_TRYP:
			{
				EXC_SET("TRYP");
				int iMinPriv = s.GetArgVal();

				if ( iMinPriv >= PLEVEL_QTY )
				{
					pSrc->SysMessagef("The %s property can't be changed.", (LPCTSTR) s.GetArgStr());
					return false;
				}

				if ( pSrc->GetPrivLevel() < iMinPriv )
				{
					pSrc->SysMessagef( "You lack the privilege to change the %s property.", (LPCTSTR) s.GetArgStr());
					return false;
				}

				// do this verb only if we can touch it.
				if ( pSrc->GetPrivLevel() <= PLEVEL_Counsel )
				{
					if ( pCharSrc == NULL || !pCharSrc->CanTouch(this) )
					{
						pSrc->SysMessagef("Can't touch %s object %s", (LPCTSTR)s.GetArgStr(), (LPCTSTR)GetName());
						return false;
					}
				}
			}
			// no break here, TRYP only does extra checks
		case OV_TRY:
			{
				EXC_SET("TRY or TRYP");
				LPCTSTR pszVerb = s.GetArgStr();
				CScript script( pszVerb );
				//DEBUG_WARN(("pszVerb %s",pszVerb));
				if ( !r_Verb(script, pSrc) )
				{
					DEBUG_ERR(( "Can't try %s object %s (0%x)\n", (LPCTSTR) pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID()));
					return( false );
				}
			}
			return( true );
		case OV_TRYSRC:
		case OV_TRYSRV:
			{
				EXC_SET("TRYSRC or TRYSRV");
				CGrayUID NewSrc;
				CTextConsole * pNewSrc = NULL;

				if ( index == OV_TRYSRC )
				{
					NewSrc = s.GetArgVal();
				}

				LPCTSTR pszVerb = s.GetArgStr();

				if ( index == OV_TRYSRC )
				{
					if ( NewSrc.IsValidUID() )
					{
						pNewSrc = NewSrc.CharFind();
					}
				}
				else
				{
					pNewSrc = &g_Serv;
				}

				if ( pNewSrc == NULL )
				{
					if ( index == OV_TRYSRC )
						DEBUG_ERR(( "Can't trysrc %s object %s (0%x): invalid src uid 0%x\n", (LPCTSTR)pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID(), NewSrc.GetObjUID()));
					else
						DEBUG_ERR(( "Can't trysrv %s object %s (0%x)\n", (LPCTSTR)pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID()));

					return false;
				}
				CScript script( pszVerb );
				if (!r_Verb(script, pNewSrc))
				{
					if ( index == OV_TRYSRC )
						DEBUG_ERR(( "Can't trysrc %s object %s (0%x) with src %s (0%x)\n", (LPCTSTR) pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID(), (LPCTSTR)pNewSrc->GetName(), NewSrc.GetObjUID()));
					else
						DEBUG_ERR(( "Can't trysrv %s object %s (0%x)\n", (LPCTSTR) pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID()));

					return false;
				}
			}
			return true;

		case OV_UID:
			EXC_SET("UID");
			// block anyone from ever doing this.
			if ( pSrc )
			{
				pSrc->SysMessage( "Setting the UID this way is not allowed" );
			}
			return( false );

		case OV_UPDATE:
			EXC_SET("UPDATE");
			Update();
			break;
		case OV_UPDATEX:
			EXC_SET("UPDATEX");
			// Some things like equipped items need to be removed before they can be updated !
			RemoveFromView();
			Update();
			break;
		case OV_DCLICK:
			EXC_SET("DCLICK");
			if ( ! pCharSrc )
				return( false );
			if ( s.HasArgs() )
			{
				CGrayUID uid = s.GetArgVal();

				if (( ! uid.ObjFind()) || ( ! this->IsChar() ))
					return( false );

				CChar *pChar = dynamic_cast <CChar *> (this);

				return pChar->Use_Obj( uid.ObjFind(), true, true );
			}
			else
				return pCharSrc->Use_Obj( this, true, true );

		case OV_USEITEM:
			EXC_SET("USEITEM");
			if ( ! pCharSrc )
				return( false );
			if ( s.HasArgs() )
			{
				CGrayUID uid = s.GetArgVal();

				if (( ! uid.ObjFind()) || ( ! this->IsChar() ))
					return( false );

				CChar *pChar = dynamic_cast <CChar *> (this);

				return pChar->Use_Obj( uid.ObjFind(), false, true );
			}
			else
				return pCharSrc->Use_Obj( this, false, true );

		case OV_FIX:
			s.GetArgStr()[0] = '\0';
		case OV_Z:	//	ussually in "SETZ" form
			EXC_SET("FIX or Z");
			if ( IsItemEquipped())
				return( false );
			if ( s.HasArgs())
			{
				SetUnkZ( s.GetArgVal());
			}
			else if ( IsTopLevel())
			{
				WORD wBlockFlags = 0;
				if (IsSetEF( EF_NewPositionChecks ))
				{
					CChar *pChar = dynamic_cast <CChar *>(this);
					CItem *pItem = dynamic_cast <CItem *>(this);
					if ( pChar )
					{
						WORD wCan = pChar->GetMoveBlockFlags();
						wBlockFlags = wCan;
						if ( wCan & CAN_C_WALK )
							wBlockFlags |= CAN_I_CLIMB; // If we can walk than we can climb. Ignore CAN_C_FLY at all here

						CGrayMapBlockState block( wBlockFlags, GetTopPoint().m_z, GetTopPoint().m_z + pChar->m_zClimbHeight + pChar->GetHeightMount( false ), GetTopPoint().m_z + pChar->m_zClimbHeight + 2, pChar->GetHeightMount( false ) );
						g_World.GetHeightPoint_New( GetTopPoint(), block, true );

						wBlockFlags = block.m_Bottom.m_dwBlockFlags;
						if ( block.m_Top.m_dwBlockFlags )
						{
							wBlockFlags |= CAN_I_ROOF;	// we are covered by something.
							if ( block.m_Top.m_z < GetTopPoint().m_z - (pChar->m_zClimbHeight + (block.m_Top.m_wTile > TERRAIN_QTY ? pChar->GetHeightMount( false ) : pChar->GetHeightMount( false )/2 )) )
								wBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
						}
						if (( wCan != 0xFFFF ) && ( wBlockFlags != 0x0 ))
						{
							CCharBase* pCharDef = pChar->Char_GetDef();

							if ( ( wBlockFlags & CAN_I_DOOR ) && pCharDef->Can( CAN_C_GHOST ))
								wBlockFlags &= ~CAN_I_BLOCK;

							if ( ( wBlockFlags & CAN_I_WATER ) && pCharDef->Can( CAN_C_SWIM ))
								wBlockFlags &= ~CAN_I_BLOCK;

							if ( ! pCharDef->Can( CAN_C_FLY ))
							{
								if ( ! ( wBlockFlags & CAN_I_CLIMB ) ) // we can climb anywhere
								{
									if ( block.m_Bottom.m_wTile > TERRAIN_QTY )
									{
										if ( block.m_Bottom.m_z > GetTopPoint().m_z + pChar->m_zClimbHeight + 2) // Too high to climb.
											break;
									}
									else if ( block.m_Bottom.m_z > GetTopPoint().m_z + pChar->m_zClimbHeight + pChar->GetHeightMount( false ) + 3)
										break;
								}
							}
							if (( wBlockFlags & CAN_I_BLOCK ) && ( ! pCharDef->Can( CAN_C_PASSWALLS )) )
								break;

							if ( block.m_Bottom.m_z >= UO_SIZE_Z )
								break;
						}
						if (( pChar->GetHeightMount( false ) + GetTopPoint().m_z >= block.m_Top.m_z ) && ( g_Cfg.m_iMountHeight ) && ( !pChar->IsPriv( PRIV_GM ) ) && ( !pChar->IsPriv( PRIV_ALLMOVE ) ))
							break;
						SetTopZ(block.m_Bottom.m_z);
					}
					else if ( pItem )
					{
						signed char zHeight = CItemBase::GetItemHeight( pItem->GetDispID(), wBlockFlags );
						CGrayMapBlockState block( wBlockFlags, GetTopPoint().m_z, GetTopPoint().m_z + zHeight, GetTopPoint().m_z + 2, zHeight );
						g_World.GetHeightPoint_New( GetTopPoint(), block, true );
						SetTopZ(block.m_Bottom.m_z);
					}
					else
					{
						g_Log.EventDebug("Failed to get reference in FIX or Z\n");
						break;
					}
				}
				else
				{
					if ( IsSetEF( EF_WalkCheck ) )
					{
						SetTopZ( g_World.GetHeightPoint_New( GetTopPoint(), wBlockFlags, true));
					}
					else
					{
						SetTopZ( g_World.GetHeightPoint( GetTopPoint(), wBlockFlags, true ));
					}
				}
			}
			Update();
			break;

		default:
			return( false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::RemoveFromView( CClient * pClientExclude, bool fHardcoded )
{
	ADDTOCALLSTACK("CObjBase::RemoveFromView");
	// Remove this item from all clients.
	// In a destructor this can do funny things.

	if ( IsDisconnected())
		return;	// not in the world.

	CObjBaseTemplate * pObjTop = GetTopLevelObj();
	CItem * pItem = fHardcoded ? (dynamic_cast<CItem*>(this)) : (NULL);
	CChar * pChar = NULL;

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClientExclude == pClient )
			continue;
		pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;
		if ( pChar->GetTopDist( pObjTop ) > UO_MAP_VIEW_SIZE ) //Client does not support removing of items which are farther (will be removed from the radar on the next step, cause the server won't resend it)
			continue;
		if (( pItem ) && ( pItem->IsItemEquipped() ) && ( !pChar->IsPriv(PRIV_GM) ))
		{
			if (( pItem->GetEquipLayer() > LAYER_HORSE ) && ( pItem->GetEquipLayer() != LAYER_BANKBOX ) && ( pItem->GetEquipLayer() != LAYER_DRAGGING ))
				continue;
		}
		pClient->addObjectRemove( this );
	}
}

void CObjBase::ResendTooltip( bool bForce )
{
	ADDTOCALLSTACK("CObjBase::ResendTooltip");
	// Remove this item from all clients.
	// In a destructor this can do funny things.

	if ( IsDisconnected())
		return;	// not in the world.

	CChar * pChar = NULL;

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext(), pChar = NULL)
	{
		pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;
		if ( !pChar->CanSee( this ) )
			continue;
		pClient->addAOSTooltip(this, bForce);
	}
}

void CObjBase::DeletePrepare()
{
	ADDTOCALLSTACK("CObjBase::DeletePrepare");
	// Prepare to delete.
	RemoveFromView();
	RemoveSelf();	// Must remove early or else virtuals will fail.
}

void CObjBase::Delete()
{
	ADDTOCALLSTACK("CObjBase::Delete");
	DeletePrepare();
	g_World.m_TimedFunctions.Erase( GetUID() );
	g_World.m_ObjDelete.InsertHead(this);
}

TRIGRET_TYPE CObjBase::Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CObjBase::Spell_OnTrigger");
	CSpellDef	* pSpellDef = g_Cfg.GetSpellDef( spell );
	if ( !pSpellDef )
		return TRIGRET_RET_TRUE;

	if ( pSpellDef->HasTrigger( stage ) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSpellDef->ResourceLock( s ))
		{
			return CScriptObj::OnTriggerScript( s, CSpellDef::sm_szTrigName[stage], pSrc, pArgs );
		}
	}
	return TRIGRET_RET_DEFAULT;
}

