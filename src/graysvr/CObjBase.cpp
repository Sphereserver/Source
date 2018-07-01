#include "graysvr.h"	// predef header
#include "../network/send.h"

bool CObjBaseTemplate::IsDeleted() const
{
	ADDTOCALLSTACK("CObjBaseTemplate::IsDeleted");
	return (!m_UID.IsValidUID() || (GetParent() == &g_World.m_ObjDelete));
}

int CObjBaseTemplate::IsWeird() const
{
	ADDTOCALLSTACK_INTENSIVE("CObjBaseTemplate::IsWeird");
	if ( !GetParent() )
		return 0x3101;
	if ( !IsValidUID() )
		return 0x3102;
	return 0;
}

bool GetDeltaStr(CPointMap &pt, TCHAR *pszDir)
{
	TCHAR *ppCmd[3];
	size_t iQty = Str_ParseCmds(pszDir, ppCmd, COUNTOF(ppCmd));
	if ( iQty <= 0 )
		return false;

	TCHAR chDir = static_cast<TCHAR>(toupper(ppCmd[0][0]));
	int iTmp = Exp_GetVal(ppCmd[1]);

	if ( IsDigit(chDir) || (chDir == '-') )
	{
		pt.m_x += static_cast<signed short>(Exp_GetVal(ppCmd[0]));
		pt.m_y += static_cast<signed short>(iTmp);
		pt.m_z += static_cast<signed char>(Exp_GetVal(ppCmd[2]));
	}
	else	// a direction by name
	{
		if ( iTmp == 0 )
			iTmp = 1;
		DIR_TYPE eDir = GetDirStr(ppCmd[0]);
		if ( eDir >= DIR_QTY )
			return false;
		pt.MoveN(eDir, iTmp);
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// -CObjBase stuff
// Either a player, npc or item.

CObjBase::CObjBase(bool fItem)
{
	sm_iCount++;
	m_wHue = HUE_DEFAULT;
	m_timeout.Init();
	m_timestamp.Init();

	//	Init some global variables
	m_fStatusUpdate = 0;
	m_ModAr = 0;
	m_PropertyList = NULL;
	m_PropertyHash = 0;
	m_PropertyRevision = 0;
	m_uidSpawnItem = UID_UNUSED;
	m_ModMaxWeight = 0;

	if ( g_Serv.IsLoading() )
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
	g_World.m_ObjNew.InsertHead(this);
}

CObjBase::~CObjBase()
{
	FreePropertyList();
	g_World.m_ObjStatusUpdates.RemovePtr(this);

	sm_iCount--;
	ASSERT(IsDisconnected());

	// free up the UID slot.
	SetUID(UID_UNUSED, false);
}

bool CObjBase::IsContainer() const
{
	ADDTOCALLSTACK("CObjBase::IsContainer");
	// Simple test if object is a container.
	return (dynamic_cast<const CContainer *>(this) != NULL);
}

void CObjBase::SetHue(HUE_TYPE wHue, bool bAvoidTrigger, CTextConsole *pSrc, CObjBase *pObj, SOUND_TYPE wSound)
{
	ADDTOCALLSTACK("CObjBase::SetHue");
	if ( g_Serv.IsLoading() )	// we do not want tons of @Dye being called during world load, just set the hue then continue...
	{
		m_wHue = wHue;
		return;
	}

	CScriptTriggerArgs args;
	args.m_iN1 = wHue;
	args.m_iN2 = wSound;

	/*	@Dye is now more universal, it is called on EVERY CObjBase color change.
		Sanity checks are recommended and if possible, avoid using it on universal events.

		Trigger info to be added to intenal
		LPCTSTR const CItem::sm_szTrigName	//CItem.cpp
		LPCTSTR const CChar::sm_szTrigName	//CChar.cpp
		enum ITRIG_TYPE						//CObjBase.h
		enum CTRIG_TYPE						//CObjBase.h
		ADD(DYE, "@Dye")					//triggers.tbl
	*/

	if ( IsTrigUsed("@Dye") && !bAvoidTrigger )
	{
		if ( pObj )
			args.m_pO1 = pObj;
		if ( OnTrigger("@Dye", pSrc, &args) == TRIGRET_RET_TRUE )
			return;
	}

	m_wHue = static_cast<HUE_TYPE>(args.m_iN1);

	if ( args.m_iN2 > 0 )	// no sound? no checks for who can hear, packets....
		Sound(static_cast<SOUND_TYPE>(args.m_iN2));

	if ( IsChar() )
	{
		// When changing body hue, check if face hue must be changed too
		CItem *pFace = static_cast<CChar *>(this)->LayerFind(LAYER_FACE);
		if ( pFace )
			pFace->SetHue(m_wHue, bAvoidTrigger, pSrc, pObj);
	}
}

int CObjBase::IsWeird() const
{
	ADDTOCALLSTACK_INTENSIVE("CObjBase::IsWeird");
	int iResultCode = CObjBaseTemplate::IsWeird();
	if ( iResultCode )
		return iResultCode;

	if ( !g_Serv.IsLoading() )
	{
		if ( GetUID().ObjFind() != this )	// make sure it's linked both ways correctly.
			return 0x3201;
	}
	return 0;
}

void CObjBase::SetUID(DWORD dwIndex, bool fItem)
{
	ADDTOCALLSTACK("CObjBase::SetUID");
	// Move the serial number.
	// This is possibly dangerous if conflict arrises.

	dwIndex &= UID_O_INDEX_MASK;	// make sure no flags in here
	if ( IsValidUID() )		// I already have a uid?
	{
		if ( !dwIndex )
			return;		// the point was just to make sure it was located
		g_World.FreeUID(static_cast<DWORD>(GetUID()) & UID_O_INDEX_MASK);	// remove the old UID
	}

	if ( dwIndex != UID_O_INDEX_MASK )	// just wanted to remove it
		dwIndex = g_World.AllocUID(dwIndex, this);
	if ( fItem )
		dwIndex |= UID_F_ITEM;

	CObjBaseTemplate::SetUID(dwIndex);
}

void inline CObjBase::SetNamePool_Fail(TCHAR *pszTitles)
{
	ADDTOCALLSTACK("CObjBase::SetNamePool_Fail");
	DEBUG_ERR(("Name pool '%s' could not be found\n", pszTitles));
	CObjBase::SetName(pszTitles);
}

bool CObjBase::SetNamePool(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CObjBase::SetNamePool");
	ASSERT(pszName);

	// Parse out the name from the name pool ?
	if ( pszName[0] == '#' )
	{
		++pszName;
		TCHAR *pszTmp = Str_GetTemp();
		strcpy(pszTmp, pszName);

		TCHAR *ppTitles[2];
		Str_ParseCmds(pszTmp, ppTitles, COUNTOF(ppTitles));

		CResourceLock s;
		if ( !g_Cfg.ResourceLock(s, RES_NAMES, ppTitles[0]) )
		{
			SetNamePool_Fail(ppTitles[0]);
			return false;
		}

		// Pick a random name
		if ( !s.ReadKey() )
		{
			SetNamePool_Fail(ppTitles[0]);
			return false;
		}
		int iCount = Calc_GetRandVal(ATOI(s.GetKey())) + 1;
		while ( iCount-- )
		{
			if ( !s.ReadKey() )
			{
				SetNamePool_Fail(ppTitles[0]);
				return false;
			}
		}

		if ( !CObjBaseTemplate::SetName(s.GetKey()) )
			return false;
	}
	else
	{
		LPCTSTR pszTmp = pszName;

		// NOTE: Name must be <= MAX_NAME_SIZE
		TCHAR szTmp[MAX_ITEM_NAME_SIZE + 1];
		if ( strlen(pszName) >= MAX_ITEM_NAME_SIZE )
		{
			strcpylen(szTmp, pszName, MAX_ITEM_NAME_SIZE);
			pszTmp = szTmp;
		}

		// Can't be a dupe name with type ?
		LPCTSTR pszTypeName = Base_GetDef()->GetTypeName();
		if ( !strcmpi(pszTypeName, pszTmp) )
			pszTmp = "";

		if ( !CObjBaseTemplate::SetName(pszTmp) )
			return false;
	}

	UpdatePropertyFlag(AUTOTOOLTIP_FLAG_NAME);
	return true;
}

bool CObjBase::MoveNearObj(const CObjBaseTemplate *pObj, WORD wSteps)
{
	ADDTOCALLSTACK("CObjBase::MoveNearObj");
	ASSERT(pObj);
	if ( pObj->IsDisconnected() )	// nothing is "near" a disconnected item.
		return false;

	pObj = pObj->GetTopLevelObj();
	return MoveNear(pObj->GetTopPoint(), wSteps);
}

void CObjBase::r_WriteSafe(CScript &s)
{
	ADDTOCALLSTACK("CObjBase::r_WriteSafe");
	// Write an object with some fault protection.
	DWORD uid = 0;
	try
	{
		uid = GetUID();

		if ( m_TagDefs.GetKeyNum("NOSAVE") )	// objects with TAG.NOSAVE set are not saved
			return;

		if ( !g_Cfg.m_fSaveGarbageCollect )
		{
			if ( g_World.FixObj(this) )
				return;
		}
		r_Write(s);
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Write Object 0%lx", uid);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )	// catch all
	{
		g_Log.CatchEvent(NULL, "Write Object 0%lx", uid);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
}

void CObjBase::SetTimeout(INT64 iDelayInTicks)
{
	ADDTOCALLSTACK("CObjBase::SetTimeout");
	// Set delay in TICK_PER_SEC of a sec. -1 = never.
	if ( iDelayInTicks < 0 )
		m_timeout.Init();
	else
		m_timeout = CServTime::GetCurrentTime() + iDelayInTicks;
}

void CObjBase::Sound(SOUND_TYPE id, BYTE iRepeat) const
{
	ADDTOCALLSTACK("CObjBase::Sound");
	// play for everyone near by.

	if ( !g_Cfg.m_fGenericSounds || (id <= SOUND_NONE) )
		return;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->CanHear(this, TALKMODE_OBJ) )
			continue;
		pClient->addSound(id, this, iRepeat);
	}
}

void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase *pSource, BYTE bSpeedSeconds, BYTE bLoop, bool fExplode, DWORD color, DWORD render, WORD effectid, WORD explodeid, WORD explodesound, DWORD effectuid, BYTE type) const
{
	ADDTOCALLSTACK("CObjBase::Effect");
	// show for everyone near by.
	//
	// bSpeedSeconds
	// bLoop
	// fExplode

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->CanSee(this) )
			continue;
		pClient->addEffect(motion, id, this, pSource, bSpeedSeconds, bLoop, fExplode, color, render, effectid, explodeid, explodesound, effectuid, type);
	}
}


void CObjBase::Emote(LPCTSTR pText, CClient *pClientExclude, bool fForcePossessive)
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase *pObjTop = static_cast<CObjBase *>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	TCHAR *pszThem = Str_GetTemp();
	TCHAR *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		// Someone has this equipped.

		if ( pObjTop != this )
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_1), pObjTop->GetName(), GetName(), pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_3), GetName(), pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_4), pText);
		}
		else
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_5), GetName(), pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_7), GetName(), pText);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_TEXT_DEF, TALKMODE_EMOTE);
}

void CObjBase::Emote2(LPCTSTR pText, LPCTSTR pText1, CClient *pClientExclude, bool fForcePossessive)
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase *pObjTop = static_cast<CObjBase *>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	TCHAR *pszThem = Str_GetTemp();
	TCHAR *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		// Someone has this equipped.
		if ( pObjTop != this )
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_1), pObjTop->GetName(), GetName(), pText1);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_3), GetName(), pText1);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_4), pText);
		}
		else
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_5), GetName(), pText1);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		sprintf(pszThem, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_7), GetName(), pText1);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_TEXT_DEF, TALKMODE_EMOTE);
}

void CObjBase::Speak(LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font)
{
	ADDTOCALLSTACK("CObjBase::Speak");
	g_World.Speak(this, pText, wHue, mode, font);
}

void CObjBase::SpeakUTF8(LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8");
	// convert UTF8 to UNICODE.
	NCHAR szBuffer[MAX_TALK_BUFFER];
	CvtSystemToNUNICODE(szBuffer, COUNTOF(szBuffer), pText, -1);
	g_World.SpeakUNICODE(this, szBuffer, wHue, mode, font, lang);
}

void CObjBase::SpeakUTF8Ex(const NWORD *pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8Ex");
	g_World.SpeakUNICODE(this, pText, wHue, mode, font, lang);
}

bool CObjBase::MoveNear(CPointMap pt, WORD wSteps)
{
	ADDTOCALLSTACK("CObjBase::MoveNear");
	// Move to nearby this other object.
	// Actually move it within +/- iSteps

	CChar *pChar = dynamic_cast<CChar *>(this);
	CPointMap ptOld = pt;

	for ( WORD i = 0; i < 20; i++ )
	{
		pt = ptOld;
		pt.m_x += static_cast<signed short>(Calc_GetRandVal2(-wSteps, wSteps));
		pt.m_y += static_cast<signed short>(Calc_GetRandVal2(-wSteps, wSteps));

		if ( !MoveTo(pt) )
			continue;

		if ( pChar )
		{
			pChar->m_zClimbHeight = 0;
			if ( !pChar->CanMoveWalkTo(pt, false) )
				continue;
		}
		if ( IsItem() )
			Update();
		return true;
	}
	return false;
}

void CObjBase::UpdateObjMessage(LPCTSTR pTextThem, LPCTSTR pTextYou, CClient *pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool bUnicode) const
{
	ADDTOCALLSTACK("CObjBase::UpdateObjMessage");
	// Show everyone a msg coming from this object.

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == pClientExclude )
			continue;
		if ( !pClient->CanSee(this) )
			continue;

		pClient->addBarkParse((pClient->GetChar() == this) ? pTextYou : pTextThem, this, wHue, mode, font, bUnicode);
	}
}

void CObjBase::UpdateCanSee(PacketSend *packet, CClient *exclude) const
{
	ADDTOCALLSTACK("CObjBase::UpdateCanSee");
	// Send this update message to everyone who can see this.
	// NOTE: Need not be a top level object. CanSee() will calc that.

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClient == exclude )
			continue;
		if ( !pClient->CanSee(this) )
			continue;

		packet->send(pClient);
	}
	delete packet;
}

TRIGRET_TYPE CObjBase::OnHearTrigger(CResourceLock &s, LPCTSTR pszCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CObjBase::OnHearTrigger");
	// Check all the keys in this script section.
	// look for pattern or partial trigger matches.
	// RETURN:
	//  TRIGRET_ENDIF = no match.
	//  TRIGRET_DEFAULT = found match but it had no RETURN
	CScriptTriggerArgs Args(pszCmd);
	Args.m_iN1 = mode;
	Args.m_iN2 = wHue;

	bool fMatch = false;

	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )
		{
			// Look for some key word.
			_strupr(s.GetArgStr());
			if ( Str_Match(s.GetArgStr(), pszCmd) == MATCH_VALID )
				fMatch = true;
			continue;
		}

		if ( !fMatch )
			continue;	// look for the next "ON" section.

		TRIGRET_TYPE iRet = CObjBase::OnTriggerRunVal(s, TRIGRUN_SECTION_EXEC, pSrc, &Args);
		if ( iRet != TRIGRET_RET_FALSE )
			return iRet;

		fMatch = false;
	}

	mode = static_cast<TALKMODE_TYPE>(Args.m_iN1);
	return TRIGRET_ENDIF;	// continue looking.
}

enum OBR_TYPE
{
	OBR_ROOM,
	OBR_SECTOR,
	OBR_SPAWNITEM,
	OBR_TOPOBJ,
	OBR_TYPEDEF,
	OBR_QTY
};

LPCTSTR const CObjBase::sm_szRefKeys[OBR_QTY+1] =
{
	"ROOM",
	"SECTOR",
	"SPAWNITEM",
	"TOPOBJ",
	"TYPEDEF",
	NULL
};

bool CObjBase::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CObjBase::r_GetRef");
	int i = FindTableHeadSorted(pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys) - 1);
	if ( i >= 0 )
	{
		pszKey += strlen(sm_szRefKeys[i]);
		SKIP_SEPARATORS(pszKey);
		switch ( i )
		{
			case OBR_ROOM:
				pRef = GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_ROOM);
				return true;
			case OBR_SECTOR:
				pRef = GetTopLevelObj()->GetTopSector();
				return true;
			case OBR_SPAWNITEM:
				if ( (m_uidSpawnItem != static_cast<CGrayUID>(UID_UNUSED)) && (pszKey[-1] != '.') )
					break;
				pRef = m_uidSpawnItem.ItemFind();
				return true;
			case OBR_TOPOBJ:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = dynamic_cast<CObjBase *>(GetTopLevelObj());
				return true;
			case OBR_TYPEDEF:
				pRef = Base_GetDef();
				return true;
		}

	}

	return CScriptObj::r_GetRef(pszKey, pRef);
}

enum OBC_TYPE
{
	#define ADD(a,b) OC_##a,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	OC_QTY
};

LPCTSTR const CObjBase::sm_szLoadKeys[OC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	NULL
};

bool CObjBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CObjBase::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		// RES_FUNCTION call
		// Is it a function returning a value ? Parse args ?
		LPCTSTR pszArgs = strchr(pszKey, ' ');
		if ( pszArgs != NULL )
		{
			pszArgs++;
			SKIP_SEPARATORS(pszArgs);
		}

		CScriptTriggerArgs Args(pszArgs != NULL ? pszArgs : "");
		if ( r_Call(pszKey, pSrc, &Args, &sVal) )
			return true;

		// Just try to default to something reasonable ?
		// Even though we have not really specified it correctly !

		// WORLD. ?
		if ( g_World.r_WriteVal(pszKey, sVal, pSrc) )
			return true;


		// TYPEDEF. ?
		if ( Base_GetDef()->r_WriteVal(pszKey, sVal, pSrc) )
			return true;

		return	CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
	}

	bool fZero = false;
	switch ( index )
	{
		//return as string or hex number or NULL if not set
		//On these ones, check BaseDef if not found on dynamic
		case OC_NAMELOC:
		{
			CVarDefCont *pVar = GetDefKey(pszKey, true);
			sVal = pVar ? pVar->GetValStr() : "";
			break;
		}

		//return as decimal number or 0 if not set
		//On these ones, check BaseDef if not found on dynamic
		case OC_DAMCHAOS:
		case OC_DAMDIRECT:
		case OC_EXPANSION:
		case OC_REGENFOOD:
		case OC_REGENHITS:
		case OC_REGENMANA:
		case OC_REGENSTAM:
		case OC_REGENVALFOOD:
		case OC_REGENVALHITS:
		case OC_REGENVALMANA:
		case OC_REGENVALSTAM:
		case OC_COMBATBONUSSTAT:
		case OC_COMBATBONUSPERCENT:
		{
			CVarDefCont *pVar = GetDefKey(pszKey, true);
			sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
			break;
		}
			
		case OC_ARMOR:
		{
			if ( IsChar() )
			{
				CChar *pChar = static_cast<CChar *>(this);
				sVal.FormatVal(pChar->m_defense);
				break;
			}

			pszKey += strlen(sm_szLoadKeys[index]);	// 9;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				if ( !strnicmp(pszKey, "LO", 2) )
					sVal.Format("%d", m_defenseBase);
				else if ( !strnicmp(pszKey, "HI", 2) )
					sVal.Format("%d", m_defenseBase + m_defenseRange);
			}
			else
				sVal.Format("%d,%d", m_defenseBase, m_defenseBase + m_defenseRange);
			break;
		}
		case OC_DAM:
		{
			pszKey += strlen(sm_szLoadKeys[index]);	// 9;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				if ( !strnicmp(pszKey, "LO", 2) )
					sVal.Format("%d", m_attackBase);
				else if ( !strnicmp(pszKey, "HI", 2) )
					sVal.Format("%d", m_attackBase + m_attackRange);
			}
			else
				sVal.Format("%d,%d", m_attackBase, m_attackBase + m_attackRange);
			break;
		}
		case OC_DAMCOLD:
			sVal.FormatVal(m_DamCold);
			break;
		case OC_DAMENERGY:
			sVal.FormatVal(m_DamEnergy);
			break;
		case OC_DAMFIRE:
			sVal.FormatVal(m_DamFire);
			break;
		case OC_DAMPHYSICAL:
			sVal.FormatVal(m_DamPhysical);
			break;
		case OC_DAMPOISON:
			sVal.FormatVal(m_DamPoison);
			break;
		case OC_INCREASEDAM:
			sVal.FormatVal(m_DamIncrease);
			break;
		case OC_INCREASEDEFCHANCE:
			sVal.FormatVal(m_DefChanceIncrease);
			break;
		case OC_INCREASEDEFCHANCEMAX:
			sVal.FormatVal(m_DefChanceIncreaseMax);
			break;
		case OC_INCREASEHITCHANCE:
			sVal.FormatVal(m_HitChanceIncrease);
			break;
		case OC_INCREASESPELLDAM:
			sVal.FormatVal(m_SpellDamIncrease);
			break;
		case OC_INCREASESWINGSPEED:
			sVal.FormatVal(m_SwingSpeedIncrease);
			break;
		case OC_FASTERCASTING:
			sVal.FormatVal(m_FasterCasting);
			break;
		case OC_FASTERCASTRECOVERY:
			sVal.FormatVal(m_FasterCastRecovery);
			break;
		case OC_HITLEECHLIFE:
			sVal.FormatVal(m_HitLifeLeech);
			break;
		case OC_HITLEECHMANA:
			sVal.FormatVal(m_HitManaLeech);
			break;
		case OC_HITLEECHSTAM:
			sVal.FormatVal(m_HitStaminaLeech);
			break;
		case OC_HITMANADRAIN:
			sVal.FormatVal(m_HitManaDrain);
			break;
		case OC_LOWERMANACOST:
			sVal.FormatVal(m_LowerManaCost);
			break;
		case OC_LOWERREAGENTCOST:
			sVal.FormatVal(m_LowerReagentCost);
			break;
		case OC_ENHANCEPOTIONS:
			sVal.FormatVal(m_EnhancePotions);
			break;
		case OC_NIGHTSIGHT:
			sVal.FormatVal(m_NightSight);
			break;
		case OC_REFLECTPHYSICALDAM:
			sVal.FormatVal(m_ReflectPhysicalDamage);
			break;
		case OC_RANGE:
		{
			if ( RangeH() == 0 )
				sVal.Format("%d", RangeL());
			else
				sVal.Format("%d,%d", RangeH(), RangeL());
			break;
		}
		case OC_RANGEL:
			sVal.FormatVal(RangeH());
			break;
		case OC_RANGEH:
			sVal.FormatVal(RangeL());
			break;
		case OC_CAN:
			sVal.FormatHex(m_Can);
			break;
		case OC_MODMAXWEIGHT:
			sVal.FormatVal(m_ModMaxWeight);
			return true;

		case OC_CANSEE:
		case OC_CANSEELOS:
		case OC_CANSEELOSFLAG:
		{
			CChar *pChar = pSrc->GetChar();
			bool bCanSee = (index == OC_CANSEE);
			bool bUseFlags = (index == OC_CANSEELOSFLAG);
			WORD flags = 0;

			pszKey += bCanSee ? 6 : (bUseFlags ? 13 : 9);
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			if ( bUseFlags && *pszKey )
			{
				flags = static_cast<WORD>(Exp_GetVal(pszKey));
				SKIP_ARGSEP(pszKey);
			}

			if ( *pszKey )		// has an argument - UID to see(los) or POS to los only
			{
				CPointMap pt;
				CObjBase *pObj = NULL;

				if ( !bCanSee )
					pt = g_Cfg.GetRegionPoint(pszKey);

				if ( bCanSee || !pt.IsValidPoint() )
				{
					CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
					pObj = uid.ObjFind();
					if ( !bCanSee && pObj )
						pt = pObj->GetTopPoint();
				}

				pChar = GetUID().CharFind();
				if ( pChar )
					sVal.FormatVal(bCanSee ? pChar->CanSee(pObj) : pChar->CanSeeLOS(pt, NULL, pChar->GetSight(), flags));
				else
					sVal.FormatVal(0);
			}
			else
			{
				if ( pChar )	// standard way src TO current object
					sVal.FormatVal(bCanSee ? pChar->CanSee(this) : pChar->CanSeeLOS(this, flags));
				else
					sVal.FormatVal(0);
			}
			break;
		}
		case OC_COLOR:
			sVal.FormatHex(GetHue());
			break;
		case OC_COMPLEXITY:
		{
			if ( IsDisconnected() || !GetTopLevelObj()->GetTopPoint().IsValidPoint() )
				return false;
			return GetTopLevelObj()->GetTopSector()->r_WriteVal(pszKey, sVal, pSrc);
		}
		case OC_CTAGCOUNT:
		{
			CChar *pChar = dynamic_cast<CChar *>(this);
			if ( pChar )
				sVal.FormatVal(pChar->m_pClient ? pChar->m_pClient->m_TagDefs.GetCount() : 0);
			else
				sVal.FormatVal(0);
			break;
		}
		case OC_TEXTF:
		{
			TCHAR *key = const_cast<TCHAR *>(pszKey);
			key += 5;
			TCHAR *pszArg[4];
			size_t iArgQty = Str_ParseCmds(key, pszArg, COUNTOF(pszArg));
			if ( iArgQty < 2 )
			{
				g_Log.EventError("Textf with less than 1 args for the given text\n");
				return false;
			}
			if ( iArgQty > 4 )
			{
				g_Log.EventError("Too many arguments given to Textf (max = text + 3\n");
				return false;
			}
			//strip quotes if any
			if ( *pszArg[0] == '"' )
				pszArg[0]++;
			BYTE count = 0;
			for ( TCHAR *pEnd = pszArg[0] + strlen(pszArg[0]) - 1; pEnd >= pszArg[0]; pEnd-- )
			{
				if ( *pEnd == '"' )
				{
					*pEnd = '\0';
					break;
				}
				count++;
			}
			sVal.Format(pszArg[0], pszArg[1], pszArg[2] ? pszArg[2] : 0, pszArg[3] ? pszArg[3] : 0);
			return true;
		}
		case OC_DIALOGLIST:
		{
			pszKey += 10;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				GETNONWHITESPACE(pszKey);

				CClient *pClient = (pSrc->GetChar() && pSrc->GetChar()->m_pClient) ? pSrc->GetChar()->m_pClient : NULL;
				sVal.FormatVal(0);

				if ( pClient )
				{
					if ( !strnicmp(pszKey, "COUNT", 5) )
						sVal.FormatVal(pClient->m_mapOpenedGumps.size());
					else
					{
						CClient::OpenedGumpsMap_t *ourMap = &pClient->m_mapOpenedGumps;
						size_t iDialogIndex = static_cast<size_t>(Exp_GetVal(pszKey));
						SKIP_SEPARATORS(pszKey);

						if ( iDialogIndex <= ourMap->size() )
						{
							CClient::OpenedGumpsMap_t::iterator itGumpFound = ourMap->begin();
							while ( iDialogIndex-- )
								++itGumpFound;

							if ( !strnicmp(pszKey, "ID", 2) )
								sVal.Format("%s", g_Cfg.ResourceGetName(RESOURCE_ID(RES_DIALOG, (*itGumpFound).first)));
							else if ( !strnicmp(pszKey, "COUNT", 5) )
								sVal.FormatVal((*itGumpFound).second);
						}
					}
				}
				else
					DEBUG_ERR(("DIALOGLIST called on non-client object.\n"));
				return true;
			}
			return false;
		}
		case OC_DISTANCE:
		{
			pszKey += 8;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			CObjBase *pThis = IsTopLevel() ? this : static_cast<CObjBase *>(GetTopLevelObj());
			CObjBase *pObj = pSrc->GetChar();

			if ( *pszKey )
			{
				CPointMap	pt = g_Cfg.GetRegionPoint(pszKey);

				if ( pt.IsValidPoint() )
				{
					if ( !pThis->GetTopPoint().IsValidPoint() )
						return false;
					else
						sVal.FormatVal(pThis->GetTopDist(pt));
					return true;
				}

				CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
				SKIP_SEPARATORS(pszKey);
				GETNONWHITESPACE(pszKey);
				pObj = uid.ObjFind();
			}

			if ( pObj && !pObj->IsTopLevel() )
				pObj = dynamic_cast<CObjBase *>(pObj->GetTopLevelObj());
			if ( !pObj )
				return false;
			sVal.FormatVal(pThis->GetDist(pObj));
			break;
		}
		case OC_EVENTS:
			m_OEvents.WriteResourceRefList(sVal);
			break;
		case OC_FACING:
		{
			pszKey += 6;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			CObjBase *pThis = IsTopLevel() ? this : static_cast<CObjBase *>(GetTopLevelObj());
			CObjBase *pObj = pSrc->GetChar();

			if ( *pszKey )
			{
				CPointMap pt = g_Cfg.GetRegionPoint(pszKey);
				if ( pt.IsValidPoint() )
				{
					if ( !pThis->GetTopPoint().IsValidPoint() )
						return false;
					else
						sVal.FormatVal(pThis->GetTopPoint().GetDir(pt));
					return true;
				}

				CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
				SKIP_SEPARATORS(pszKey);
				GETNONWHITESPACE(pszKey);
				pObj = uid.ObjFind();
			}

			if ( !pObj )
				return false;
			if ( !pObj->IsTopLevel() )
				pObj = static_cast<CObjBase *>(pObj->GetTopLevelObj());

			sVal.FormatVal(pThis->GetDir(pObj));
			break;
		}
		case OC_ISCHAR:
			sVal.FormatVal(IsChar());
			break;
		case OC_ISEVENT:
		{
			if ( pszKey[7] != '.' )
				return false;
			pszKey += 8;
			sVal.FormatVal(m_OEvents.ContainsResourceName(RES_EVENTS, pszKey));
			return true;
		}
		case OC_ISTEVENT:
		{
			if ( pszKey[8] != '.' )
				return false;
			pszKey += 9;
			sVal.FormatVal(Base_GetDef()->m_TEvents.ContainsResourceName(RES_EVENTS, pszKey));
			return true;
		}
		case OC_ISITEM:
			sVal.FormatVal(IsItem());
			break;
		case OC_ISCONT:
			sVal.FormatVal(IsContainer());
			break;
		case OC_ISNEARTYPETOP:
		case OC_ISNEARTYPE:
		{
			bool bReturnP = false;
			pszKey += (index == OC_ISNEARTYPETOP) ? 13 : 10;
			if ( !strnicmp(pszKey, ".P", 2) )
			{
				bReturnP = true;
				pszKey += 2;
			}
			SKIP_SEPARATORS(pszKey);
			SKIP_ARGSEP(pszKey);

			if ( !GetTopPoint().IsValidPoint() )
				sVal.FormatVal(0);
			else
			{
				IT_TYPE iType = static_cast<IT_TYPE>(g_Cfg.ResourceGetIndexType(RES_TYPEDEF, pszKey));

				SKIP_IDENTIFIERSTRING(pszKey);
				SKIP_SEPARATORS(pszKey);
				SKIP_ARGSEP(pszKey);

				int iDistance = *pszKey ? Exp_GetVal(pszKey) : 0;
				bool bCheckMulti = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
				bool bLimitZ = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;

				if ( bReturnP )
				{
					CPointMap pt = (index == OC_ISNEARTYPETOP) ? g_World.FindTypeNear_Top(GetTopPoint(), iType, iDistance) : g_World.FindItemTypeNearby(GetTopPoint(), iType, iDistance, bCheckMulti, bLimitZ);
					if ( !pt.IsValidPoint() )
						sVal.FormatVal(0);
					else
						sVal = pt.WriteUsed();
				}
				else
					sVal.FormatVal((index == OC_ISNEARTYPETOP) ? g_World.IsTypeNear_Top(GetTopPoint(), iType, iDistance) : g_World.IsItemTypeNear(GetTopPoint(), iType, iDistance, bCheckMulti, bLimitZ));
			}
			return true;
		}
		case OC_ISPLAYER:
		{
			CChar *pChar = dynamic_cast<CChar *>(this);
			sVal.FormatVal((pChar && pChar->m_pPlayer) ? 1 : 0);
			return true;
		}
		case OC_ISDIALOGOPEN:
		{
			pszKey += 12;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);
			CChar *pChar = dynamic_cast<CChar *>(this);
			CClient *pClient = (pChar && pChar->m_pClient) ? pChar->m_pClient : NULL;

			if ( pClient )
			{
				RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pszKey);
				int context;
				if ( pClient->m_NetState->isClientKR() )
					context = g_Cfg.GetKRDialog(static_cast<DWORD>(rid)) & 0x00FFFFFF;
				else
					context = static_cast<DWORD>(rid) & 0x00FFFFFF;

				CClient::OpenedGumpsMap_t::iterator itGumpFound = pClient->m_mapOpenedGumps.find(context);
				if ( itGumpFound != pClient->m_mapOpenedGumps.end() )
					sVal.FormatVal((*itGumpFound).second);
				else
					sVal.FormatVal(0);
			}
			else
				sVal.FormatVal(0);
			return true;
		}
		case OC_ISARMOR:
		{
			pszKey += 7;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);
			CItem *pItem = NULL;
			if ( *pszKey )
			{
				TCHAR *pszArg = Str_GetTemp();
				strcpylen(pszArg, pszKey, strlen(pszKey) + 1);

				CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
				pItem = dynamic_cast<CItem *>(uid.ObjFind());
				if ( !pItem )
				{
					ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetID(RES_ITEMDEF, const_cast<LPCTSTR &>(reinterpret_cast<LPTSTR &>(pszArg))).GetResIndex());
					const CItemBase *pItemDef = CItemBase::FindItemBase(id);
					if ( pItemDef )
					{
						sVal.FormatVal(CItemBase::IsTypeArmor(pItemDef->GetType()));
						break;
					}
				}
				sVal.FormatVal(pItem ? pItem->IsTypeArmor() : 0);
				break;
			}
			pItem = dynamic_cast<CItem *>(this);
			sVal.FormatVal(pItem ? pItem->IsTypeArmor() : 0);
			break;
		}
		case OC_ISTIMERF:
		{
			if ( pszKey[8] != '.' )
				return false;
			pszKey += 9;
			sVal.FormatVal(g_World.m_TimedFunctions.IsTimer(GetUID(), pszKey));
			return true;
		}
		case OC_ISWEAPON:
		{
			pszKey += 8;
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);
			CItem *pItem = NULL;
			if ( *pszKey )
			{
				TCHAR *pszArg = Str_GetTemp();
				strcpylen(pszArg, pszKey, strlen(pszKey) + 1);

				CGrayUID uid = static_cast<CGrayUID>(Exp_GetVal(pszKey));
				pItem = dynamic_cast<CItem *>(uid.ObjFind());
				if ( !pItem )
				{
					ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetID(RES_ITEMDEF, const_cast<LPCTSTR &>(reinterpret_cast<LPTSTR &>(pszArg))).GetResIndex());
					const CItemBase *pItemDef = CItemBase::FindItemBase(id);
					if ( pItemDef )
					{
						sVal.FormatVal(CItemBase::IsTypeWeapon(pItemDef->GetType()));
						break;
					}
				}
				sVal.FormatVal(pItem ? pItem->IsTypeWeapon() : 0);
				break;
			}
			pItem = dynamic_cast<CItem *>(this);
			sVal.FormatVal(pItem ? pItem->IsTypeWeapon() : 0);
			break;
		}
		case OC_LUCK:
			sVal.FormatVal(m_Luck);
			break;
		case OC_MAP:
			sVal.FormatVal(GetTopPoint().m_map);
			break;
		case OC_MODAR:
		case OC_MODAC:
			sVal.FormatVal(m_ModAr);
			break;
		case OC_NAME:
			sVal = GetName();
			break;
		case OC_P:
		{
			if ( pszKey[1] == '.' )
				return GetTopPoint().r_WriteVal(pszKey + 2, sVal);
			sVal = GetTopPoint().WriteUsed();
			break;
		}
		case OC_RESCOLD:
			sVal.FormatVal(m_ResCold);
			break;
		case OC_RESCOLDMAX:
			sVal.FormatVal(m_ResColdMax);
			break;
		case OC_RESENERGY:
			sVal.FormatVal(m_ResEnergy);
			break;
		case OC_RESENERGYMAX:
			sVal.FormatVal(m_ResEnergyMax);
			break;
		case OC_RESFIRE:
			sVal.FormatVal(m_ResFire);
			break;
		case OC_RESFIREMAX:
			sVal.FormatVal(m_ResFireMax);
			break;
		case OC_RESPHYSICAL:
			sVal.FormatVal(m_ResPhysical);
			break;
		case OC_RESPHYSICALMAX:
			sVal.FormatVal(m_ResPhysicalMax);
			break;
		case OC_RESPOISON:
			sVal.FormatVal(m_ResPoison);
			break;
		case OC_RESPOISONMAX:
			sVal.FormatVal(m_ResPoisonMax);
			break;
		case OC_TAG0:
			fZero = true;
			pszKey++;
		case OC_TAG:			// "TAG" = get/set a local tag.
		{
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			CVarDefCont *pVarKey = m_TagDefs.GetKey(pszKey);
			if ( !pVarKey )
				sVal = Base_GetDef()->m_TagDefs.GetKeyStr(pszKey, fZero);
			else
				sVal = pVarKey->GetValStr();
			return true;
		}
		case OC_TIMER:
			sVal.FormatLLVal(GetTimerAdjusted());
			break;
		case OC_TIMERD:
			sVal.FormatLLVal(GetTimerDAdjusted());
			break;
		case OC_TRIGGER:
		{
			pszKey += 7;
			GETNONWHITESPACE(pszKey);

			if ( *pszKey )
			{
				TRIGRET_TYPE trReturn;
				bool bTrigReturn = CallPersonalTrigger(const_cast<TCHAR *>(pszKey), pSrc, trReturn, false);
				if ( bTrigReturn )
					sVal.FormatVal(trReturn);

				return bTrigReturn;
			}
			return false;
		}
		case OC_TOPOBJ:
		{
			if ( pszKey[6] == '.' )
				return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
			sVal.FormatHex(GetTopLevelObj()->GetUID());
			break;
		}
		case OC_UID:
			if ( pszKey[3] == '.' )
				return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
		case OC_SERIAL:
			sVal.FormatHex(GetUID());
			break;
		case OC_SPAWNITEM:
		{
			if ( pszKey[9] == '.' )
				return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
			sVal.FormatHex(m_uidSpawnItem);
			break;
		}
		case OC_SEXTANTP:
		{
			pszKey += 8;
			SKIP_SEPARATORS(pszKey); GETNONWHITESPACE(pszKey);

			CPointMap pt;
			if ( *pszKey )
				pt = g_Cfg.GetRegionPoint(pszKey);
			else
				pt = GetTopPoint();

			if ( !pt.IsValidPoint() )
				return false;

			sVal = g_Cfg.Calc_MaptoSextant(pt);
			break;
		}
		case OC_SPEED:
		{
			if ( !IsItem() )
				return false;
			CItem *pItem = static_cast<CItem *>(this);
			sVal.FormatVal(pItem->GetSpeed());
			break;
		}
		case OC_TIMESTAMP:
			sVal.FormatLLVal(GetTimeStamp().GetTimeRaw());
			break;
		case OC_VERSION:
			sVal = SPHERE_VERSION;
			break;
		case OC_WEIGHT:
			sVal.FormatVal(GetWeight());
			break;
		case OC_Z:
			sVal.FormatVal(GetTopZ());
			break;
		case OC_TAGAT:
		{
			pszKey += 5;
			if ( *pszKey == '.' )	// do we have an argument?
			{
				SKIP_SEPARATORS(pszKey);
				size_t iQty = static_cast<size_t>(Exp_GetVal(pszKey));
				if ( iQty >= m_TagDefs.GetCount() )
					return false;	// trying to get non-existant tag

				const CVarDefCont *pTagAt = m_TagDefs.GetAt(iQty);
				if ( !pTagAt )
					return false;	// trying to get non-existant tag

				SKIP_SEPARATORS(pszKey);
				if ( !*pszKey )
				{
					sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
					return true;
				}
				else if ( !strnicmp(pszKey, "KEY", 3) ) // key?
				{
					sVal = pTagAt->GetKey();
					return true;
				}
				else if ( !strnicmp(pszKey, "VAL", 3) ) // val?
				{
					sVal = pTagAt->GetValStr();
					return true;
				}
			}
			return false;
		}
		case OC_TAGCOUNT:
			sVal.FormatVal(m_TagDefs.GetCount());
			break;
		case OC_PROPSAT:
		{
			pszKey += 7;	// eat the 'TAGAT'
			if ( *pszKey == '.' )	// do we have an argument?
			{
				SKIP_SEPARATORS(pszKey);
				size_t iQty = static_cast<size_t>(Exp_GetVal(pszKey));
				if ( iQty >= m_BaseDefs.GetCount() )
					return false;	// trying to get non-existant tag

				const CVarDefCont *pTagAt = m_BaseDefs.GetAt(iQty);
				if ( !pTagAt )
					return false;	// trying to get non-existant tag

				SKIP_SEPARATORS(pszKey);
				if ( !*pszKey )
				{
					sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
					return true;
				}
				else if ( !strnicmp(pszKey, "KEY", 3) ) // key?
				{
					sVal = pTagAt->GetKey();
					return true;
				}
				else if ( !strnicmp(pszKey, "VAL", 3) ) // val?
				{
					sVal = pTagAt->GetValStr();
					return true;
				}
			}

			return false;
		}
		case OC_PROPSCOUNT:
			sVal.FormatVal(m_BaseDefs.GetCount());
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

bool CObjBase::r_LoadVal(CScript &s)
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
		m_TagDefs.SetStr(s.GetKey() + 4, fQuoted, s.GetArgStr(&fQuoted), false);
		return true;
	}
	else if ( s.IsKeyHead("TAG0.", 5) )
	{
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey() + 5, fQuoted, s.GetArgStr(&fQuoted), true);
		return true;
	}

	int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
		return CScriptObj::r_LoadVal(s);

	bool bUpdateClientStats = false;

	switch ( index )
	{
		// Set as numeric
		case OC_DAMCHAOS:
		case OC_DAMDIRECT:
		case OC_EXPANSION:
		case OC_NAMELOC:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			break;

		case OC_REGENFOOD:
		case OC_REGENHITS:
		case OC_REGENSTAM:
		case OC_REGENMANA:
		case OC_REGENVALFOOD:
		case OC_REGENVALHITS:
		case OC_REGENVALSTAM:
		case OC_REGENVALMANA:
		case OC_COMBATBONUSSTAT:
		case OC_COMBATBONUSPERCENT:
			SetDefNum(s.GetKey(), s.GetArgVal());
			bUpdateClientStats = true;
			break;

		case OC_ARMOR:
		{
			if ( IsChar() )
				return false;

			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_defenseBase = static_cast<WORD>(piVal[0]);
			m_defenseRange = (iQty > 1) ? static_cast<WORD>(piVal[1]) - m_defenseBase : 0;
			bUpdateClientStats = true;
			break;
		}
		case OC_DAM:
		{
			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_attackBase = static_cast<WORD>(piVal[0]);
			m_attackRange = (iQty > 1) ? static_cast<WORD>(piVal[1]) - m_attackBase : 0;
			bUpdateClientStats = true;
			break;
		}
		case OC_DAMCOLD:
			m_DamCold = static_cast<int>(s.GetArgVal());
			break;
		case OC_DAMENERGY:
			m_DamEnergy = static_cast<int>(s.GetArgVal());
			break;
		case OC_DAMFIRE:
			m_DamFire = static_cast<int>(s.GetArgVal());
			break;
		case OC_DAMPHYSICAL:
			m_DamPhysical = static_cast<int>(s.GetArgVal());
			break;
		case OC_DAMPOISON:
			m_DamPoison = static_cast<int>(s.GetArgVal());
			break;
		case OC_INCREASEDAM:
			m_DamIncrease = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_INCREASEDEFCHANCE:
			m_DefChanceIncrease = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_INCREASEDEFCHANCEMAX:
			m_DefChanceIncreaseMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_INCREASEHITCHANCE:
			m_HitChanceIncrease = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_INCREASESPELLDAM:
			m_SpellDamIncrease = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_INCREASESWINGSPEED:
			m_SwingSpeedIncrease = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_FASTERCASTING:
			m_FasterCasting = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_FASTERCASTRECOVERY:
			m_FasterCastRecovery = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_HITLEECHLIFE:
			m_HitLifeLeech = static_cast<int>(s.GetArgVal());
			break;
		case OC_HITLEECHMANA:
			m_HitManaLeech = static_cast<int>(s.GetArgVal());
			break;
		case OC_HITLEECHSTAM:
			m_HitStaminaLeech = static_cast<int>(s.GetArgVal());
			break;
		case OC_HITMANADRAIN:
			m_HitManaDrain = static_cast<int>(s.GetArgVal());
			break;
		case OC_LOWERMANACOST:
			m_LowerManaCost = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_LOWERREAGENTCOST:
			m_LowerReagentCost = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_ENHANCEPOTIONS:
			m_EnhancePotions = static_cast<int>(s.GetArgVal());
			break;
		case OC_NIGHTSIGHT:
			m_NightSight = static_cast<int>(s.GetArgVal());
			break;
		case OC_REFLECTPHYSICALDAM:
			m_ReflectPhysicalDamage = static_cast<int>(s.GetArgVal());
			break;
		case OC_RANGE:
		{
			INT64 piVal[2];
			size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			if ( iQty > 1 )
			{
				INT64 iRange = ((piVal[0] & 0xFF) << 8) & 0xFF00;
				iRange |= (piVal[1] & 0xFF);
				SetDefNum(s.GetKey(), iRange, false);
			}
			else
				SetDefNum(s.GetKey(), piVal[0], false);
			break;
		}
		case OC_CAN:
			m_Can = static_cast<DWORD>(s.GetArgVal());
			break;
		case OC_MODMAXWEIGHT:
			m_ModMaxWeight = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_COLOR:
		{
			if ( !strcmpi(s.GetArgStr(), "match_shirt") || !strcmpi(s.GetArgStr(), "match_hair") )
			{
				CChar *pChar = dynamic_cast<CChar *>(GetTopLevelObj());
				if ( pChar )
				{
					CItem *pMatch = pChar->LayerFind(!strcmpi(s.GetArgStr() + 6, "shirt") ? LAYER_SHIRT : LAYER_HAIR);
					if ( pMatch )
					{
						m_wHue = pMatch->GetHue();
						break;
					}
				}
				m_wHue = HUE_GRAY;
				break;
			}
			SetHue(static_cast<HUE_TYPE>(s.GetArgVal()), false, &g_Serv);	// @Dye is called from @Create/.xcolor/script command here. Since we can not receive pSrc on this r_LoadVal function ARGO/SRC will be null
			Update();
			break;
		}
		case OC_EVENTS:
			return m_OEvents.r_LoadVal(s, RES_EVENTS);
		case OC_LUCK:
			m_Luck = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_MAP:
		{
			// Move to another map
			if ( !IsTopLevel() )
				return false;

			CPointMap pt = GetTopPoint();
			pt.m_map = static_cast<BYTE>(s.GetArgVal());

			// Is the desired mapplane allowed?
			if ( !g_MapList.m_maps[pt.m_map] )
				return false;

			MoveTo(pt);
			if ( IsItem() )
				Update();
			break;
		}
		case OC_MODAR:
		case OC_MODAC:
			m_ModAr = s.GetArgVal();
			bUpdateClientStats = true;
			break;
		case OC_NAME:
			SetName(static_cast<LPCTSTR>(s.GetArgStr()));
			break;
		case OC_P:
			return false;	// must set the point via the CItem or CChar methods
		case OC_RESCOLD:
			m_ResCold = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESCOLDMAX:
			m_ResColdMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESENERGY:
			m_ResEnergy = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESENERGYMAX:
			m_ResEnergyMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESFIRE:
			m_ResFire = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESFIREMAX:
			m_ResFireMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESPHYSICAL:
			m_ResPhysical = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESPHYSICALMAX:
			m_ResPhysicalMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESPOISON:
			m_ResPoison = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_RESPOISONMAX:
			m_ResPoisonMax = static_cast<int>(s.GetArgVal());
			bUpdateClientStats = true;
			break;
		case OC_SPEED:
		{
			if ( !IsItem() )
				return false;
			CItem *pItem = static_cast<CItem *>(this);
			pItem->m_speed = static_cast<BYTE>(s.GetArgVal());
			break;
		}
		case OC_TIMER:
			SetTimeout(s.GetArgLLVal() * TICK_PER_SEC);
			break;
		case OC_TIMERD:
			SetTimeout(s.GetArgLLVal());
			break;
		case OC_TIMESTAMP:
			SetTimeStamp(s.GetArgLLVal());
			break;
		case OC_SPAWNITEM:
			if ( !g_Serv.IsLoading() )	// SPAWNITEM is read-only
				return false;
			m_uidSpawnItem = static_cast<CGrayUID>(s.GetArgVal());
			break;
		case OC_UID:
		case OC_SERIAL:
			// Don't set container flags through this.
			SetUID(s.GetArgVal(), dynamic_cast<CItem *>(this) ? true : false);
			break;
		default:
			return false;
	}

	if ( !g_Serv.IsLoading() )
	{
		if ( IsItem() )
			ResendTooltip();
		else if ( bUpdateClientStats )
		{
			CChar *pChar = static_cast<CChar *>(this);
			pChar->UpdateStatsFlag();
		}
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CObjBase::r_Write");
	s.WriteKeyHex("SERIAL", GetUID());
	if ( IsIndividualName() )
		s.WriteKey("NAME", GetIndividualName());
	if ( m_wHue != HUE_DEFAULT )
		s.WriteKeyHex("COLOR", GetHue());
	if ( m_timeout.IsTimeValid() )
		s.WriteKeyVal("TIMER", GetTimerAdjusted());
	if ( m_timestamp.IsTimeValid() )
		s.WriteKeyVal("TIMESTAMP", GetTimeStamp().GetTimeRaw());
	if ( m_uidSpawnItem.IsValidUID() )
		s.WriteKeyHex("SPAWNITEM", m_uidSpawnItem);
	if ( m_ModAr )
		s.WriteKeyVal("MODAR", m_ModAr);
	if ( m_ModMaxWeight )
		s.WriteKeyVal("MODMAXWEIGHT", m_ModMaxWeight);

	CBaseBaseDef *pBaseDef = Base_GetDef();
	ASSERT(pBaseDef);

	if ( m_DamPhysical != pBaseDef->m_DamPhysical )
		s.WriteKeyVal("DAMPHYSICAL", m_DamPhysical);
	if ( m_DamFire != pBaseDef->m_DamFire )
		s.WriteKeyVal("DAMFIRE", m_DamFire);
	if ( m_DamCold != pBaseDef->m_DamCold )
		s.WriteKeyVal("DAMCOLD", m_DamCold);
	if ( m_DamPoison != pBaseDef->m_DamPoison )
		s.WriteKeyVal("DAMPOISON", m_DamPoison);
	if ( m_DamEnergy != pBaseDef->m_DamEnergy )
		s.WriteKeyVal("DAMENERGY", m_DamEnergy);

	if ( m_ResPhysical != pBaseDef->m_ResPhysical )
		s.WriteKeyVal("RESPHYSICAL", m_ResPhysical);
	if ( m_ResPhysicalMax != pBaseDef->m_ResPhysicalMax )
		s.WriteKeyVal("RESPHYSICALMAX", m_ResPhysicalMax);
	if ( m_ResFire != pBaseDef->m_ResFire )
		s.WriteKeyVal("RESFIRE", m_ResFire);
	if ( m_ResFireMax != pBaseDef->m_ResFireMax )
		s.WriteKeyVal("RESFIREMAX", m_ResFireMax);
	if ( m_ResCold != pBaseDef->m_ResCold )
		s.WriteKeyVal("RESCOLD", m_ResCold);
	if ( m_ResColdMax != pBaseDef->m_ResColdMax )
		s.WriteKeyVal("RESCOLDMAX", m_ResColdMax);
	if ( m_ResPoison != pBaseDef->m_ResPoison )
		s.WriteKeyVal("RESPOISON", m_ResPoison);
	if ( m_ResPoisonMax != pBaseDef->m_ResPoisonMax )
		s.WriteKeyVal("RESPOISONMAX", m_ResPoisonMax);
	if ( m_ResEnergy != pBaseDef->m_ResEnergy )
		s.WriteKeyVal("RESENERGY", m_ResEnergy);
	if ( m_ResEnergyMax != pBaseDef->m_ResEnergyMax )
		s.WriteKeyVal("RESENERGYMAX", m_ResEnergyMax);

	if ( m_Luck != pBaseDef->m_Luck )
		s.WriteKeyVal("LUCK", m_Luck);
	if ( m_DamIncrease != pBaseDef->m_DamIncrease )
		s.WriteKeyVal("INCREASEDAM", m_DamIncrease);
	if ( m_SpellDamIncrease != pBaseDef->m_SpellDamIncrease )
		s.WriteKeyVal("INCREASESPELLDAM", m_SpellDamIncrease);
	if ( m_HitLifeLeech != pBaseDef->m_HitLifeLeech )
		s.WriteKeyVal("HITLEECHLIFE", m_HitLifeLeech);
	if ( m_HitManaLeech != pBaseDef->m_HitManaLeech )
		s.WriteKeyVal("HITLEECHMANA", m_HitManaLeech);
	if ( m_HitStaminaLeech != pBaseDef->m_HitStaminaLeech )
		s.WriteKeyVal("HITLEECHSTAM", m_HitStaminaLeech);
	if ( m_HitManaDrain != pBaseDef->m_HitManaDrain )
		s.WriteKeyVal("HITMANADRAIN", m_HitManaDrain);
	if ( m_HitChanceIncrease != pBaseDef->m_HitChanceIncrease )
		s.WriteKeyVal("INCREASEHITCHANCE", m_HitChanceIncrease);
	if ( m_DefChanceIncrease != pBaseDef->m_DefChanceIncrease )
		s.WriteKeyVal("INCREASEDEFCHANCE", m_DefChanceIncrease);
	if ( m_DefChanceIncreaseMax != pBaseDef->m_DefChanceIncreaseMax )
		s.WriteKeyVal("INCREASEDEFCHANCEMAX", m_DefChanceIncreaseMax);
	if ( m_SwingSpeedIncrease != pBaseDef->m_SwingSpeedIncrease )
		s.WriteKeyVal("INCREASESWINGSPEED", m_SwingSpeedIncrease);
	if ( m_FasterCasting != pBaseDef->m_FasterCasting )
		s.WriteKeyVal("FASTERCASTING", m_FasterCasting);
	if ( m_FasterCastRecovery != pBaseDef->m_FasterCastRecovery )
		s.WriteKeyVal("FASTERCASTRECOVERY", m_FasterCastRecovery);
	if ( m_LowerManaCost != pBaseDef->m_LowerManaCost )
		s.WriteKeyVal("LOWERMANACOST", m_LowerManaCost);
	if ( m_LowerReagentCost != pBaseDef->m_LowerReagentCost )
		s.WriteKeyVal("LOWERREAGENTCOST", m_LowerReagentCost);
	if ( m_EnhancePotions != pBaseDef->m_EnhancePotions )
		s.WriteKeyVal("ENHANCEPOTIONS", m_EnhancePotions);
	if ( m_NightSight != pBaseDef->m_NightSight )
		s.WriteKeyVal("NIGHTSIGHT", m_NightSight);
	if ( m_ReflectPhysicalDamage != pBaseDef->m_ReflectPhysicalDamage )
		s.WriteKeyVal("REFLECTPHYSICALDAM", m_ReflectPhysicalDamage);

	m_BaseDefs.r_WritePrefix(s);	// new variable storage system
	m_TagDefs.r_WritePrefix(s, "TAG");
	m_OEvents.r_Write(s, "EVENTS");
}

enum OV_TYPE
{
	#define ADD(a,b) OV_##a,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	OV_QTY
};

LPCTSTR const CObjBase::sm_szVerbKeys[OV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	NULL
};

// Execute command from script
bool CObjBase::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CObjBase::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	LPCTSTR	pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	CGString sVal;
	CScriptTriggerArgs Args(s.GetArgRaw());
	if ( r_Call(pszKey, pSrc, &Args, &sVal) )
		return true;

	int index;
	if ( !strnicmp(pszKey, "TARGET", 6) )
		index = OV_TARGET;
	else
	{
		index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
		if ( index < 0 )
			return CScriptObj::r_Verb(s, pSrc);
	}

	CChar *pCharSrc = pSrc->GetChar();
	CClient *pClientSrc = (pCharSrc && pCharSrc->m_pClient) ? pCharSrc->m_pClient : NULL;

	switch ( static_cast<OV_TYPE>(index) )
	{
		case OV_DAMAGE:	//	"Dmg, SourceFlags, SourceCharUid, DmgPhysical(%), DmgFire(%), DmgCold(%), DmgPoison(%), DmgEnergy(%)" = do me some damage.
		{
			EXC_SET("DAMAGE");
			INT64 piCmd[8];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty < 1 )
				return false;
			if ( iArgQty > 2 )	// Give it a new source char UID
			{
				CObjBaseTemplate *pObj = CGrayUID(static_cast<DWORD>(piCmd[2])).ObjFind();
				if ( pObj )
					pObj = pObj->GetTopLevelObj();
				pCharSrc = dynamic_cast<CChar *>(pObj);
			}

			CChar *pChar = dynamic_cast<CChar *>(this);
			CItem *pItem = dynamic_cast<CItem *>(this);
			if ( pChar )
				pChar->OnTakeDamage(static_cast<int>(piCmd[0]),
					pCharSrc,
					(iArgQty >= 1) ? static_cast<DAMAGE_TYPE>(piCmd[1]) : DAMAGE_HIT_BLUNT|DAMAGE_GENERAL,
					(iArgQty >= 3) ? static_cast<int>(piCmd[3]) : 0,		// physical damage %
					(iArgQty >= 4) ? static_cast<int>(piCmd[4]) : 0,		// fire damage %
					(iArgQty >= 5) ? static_cast<int>(piCmd[5]) : 0,		// cold damage %
					(iArgQty >= 6) ? static_cast<int>(piCmd[6]) : 0,		// poison damage %
					(iArgQty >= 7) ? static_cast<int>(piCmd[7]) : 0			// energy damage %
				);
			else if ( pItem )
				pItem->OnTakeDamage(static_cast<int>(piCmd[0]),
					pCharSrc,
					(iArgQty >= 1) ? static_cast<DAMAGE_TYPE>(piCmd[1]) : DAMAGE_HIT_BLUNT|DAMAGE_GENERAL
				);
			break;
		}
		case OV_EDIT:
		{
			EXC_SET("EDIT");
			// Put up a list of items in the container. (if it is a container)
			if ( !pClientSrc )
				return false;
			pClientSrc->m_Targ_Text = s.GetArgStr();
			pClientSrc->Cmd_EditItem(this, -1);
			break;
		}
		case OV_EFFECT: // some visual effect.
		{
			EXC_SET("EFFECT");
			INT64 piCmd[12];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty < 2 )
				return false;
			CObjBase *pThis = this;
			if ( piCmd[0] == -1 )
			{
				if ( pCharSrc )
				{
					piCmd[0] = EFFECT_BOLT;
					pThis = pCharSrc;
					pCharSrc = dynamic_cast<CChar *>(this);
				}

			}
			pThis->Effect(static_cast<EFFECT_TYPE>(piCmd[0]), static_cast<ITEMID_TYPE>(RES_GET_INDEX(piCmd[1])),
				pCharSrc,
				(iArgQty >= 3) ? static_cast<BYTE>(piCmd[2]) : 5,		// BYTE bSpeedSeconds = 5,
				(iArgQty >= 4) ? static_cast<BYTE>(piCmd[3]) : 1,		// BYTE bLoop = 1,
				(iArgQty >= 5) ? (piCmd[4] != 0) : false,				// bool fExplode = false
				(iArgQty >= 6) ? static_cast<DWORD>(piCmd[5]) : 0,		// Hue
				(iArgQty >= 7) ? static_cast<DWORD>(piCmd[6]) : 0,		// Render mode,		
				(iArgQty >= 8) ? static_cast<WORD>(piCmd[7]) : 0,		// EffectID	//New Packet 0xc7
				(iArgQty >= 9) ? static_cast<WORD>(piCmd[8]) : 0,		// ExplodeID
				(iArgQty >= 10) ? static_cast<WORD>(piCmd[9]) : 0,		// ExplodeSound
				(iArgQty >= 11) ? static_cast<DWORD>(piCmd[10]) : 0,	// EffectUID
				(iArgQty >= 12) ? static_cast<BYTE>(piCmd[11]) : 0		// Type
			);
			break;
		}
		case OV_EMOTE:
			EXC_SET("EMOTE");
			Emote(s.GetArgStr());
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
			if ( !pClientSrc )
				return false;

			TCHAR *Arg_ppCmd[2];		// Maximum parameters in one line
			size_t iQty = Str_ParseCmds(s.GetArgStr(), Arg_ppCmd, COUNTOF(Arg_ppCmd));

			CGString sOrgValue;
			if ( !r_WriteVal(Arg_ppCmd[0], sOrgValue, pSrc) )
				sOrgValue = ".";

			pClientSrc->m_Targ_Text = Arg_ppCmd[0];	// The attribute we want to edit.
			int iMaxLength = (iQty > 1) ? ATOI(Arg_ppCmd[1]) : 1;

			CGString sPrompt;
			sPrompt.Format("%s (# = default)", static_cast<LPCTSTR>(Arg_ppCmd[0]));
			pClientSrc->addGumpInpVal(true, INPVAL_STYLE_TEXTEDIT, iMaxLength, sPrompt, sOrgValue, this);
			break;
		}
		case OV_MENU:
		{
			EXC_SET("MENU");
			if ( !pClientSrc )
				return false;
			pClientSrc->Menu_Setup(g_Cfg.ResourceGetIDType(RES_MENU, s.GetArgStr()), this);
			break;
		}
		case OV_MESSAGE:	//put info message (for pSrc client only) over item.
		case OV_MSG:
		{
			EXC_SET("MESSAGE or MSG");
			if ( !pCharSrc )
				UpdateObjMessage(s.GetArgStr(), s.GetArgStr(), NULL, HUE_TEXT_DEF, TALKMODE_OBJ);
			else
				pCharSrc->ObjMessage(s.GetArgStr(), this);
			break;
		}
		case OV_MESSAGEUA:
		{
			EXC_SET("MESSAGEUA");
			if ( !pClientSrc )
				return false;

			TCHAR *pszArgs[5];
			NCHAR ncBuffer[MAX_TALK_BUFFER];

			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), pszArgs, COUNTOF(pszArgs));
			if ( iArgQty < 5 )
				break;

			CvtSystemToNUNICODE(ncBuffer, COUNTOF(ncBuffer), pszArgs[4], -1);
			pClientSrc->addBarkUNICODE(ncBuffer, this,
				static_cast<HUE_TYPE>(pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF),
				static_cast<TALKMODE_TYPE>(pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY),
				static_cast<FONT_TYPE>(pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL),
				CLanguageID(pszArgs[3]));
			break;
		}
		case OV_MOVE:
		{
			// move without restriction. east,west,etc. (?up,down,)
			EXC_SET("MOVE");
			if ( IsTopLevel() )
			{
				CPointMap pt = GetTopPoint();
				if ( !GetDeltaStr(pt, s.GetArgStr()) )
					return false;
				MoveTo(pt);
				Update();
			}
			break;
		}
		case OV_MOVENEAR:
		{
			EXC_SET("MOVENEAR");
			CObjBase *pObjNear;
			INT64 piCmd[4];

			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty <= 0 )
				return false;
			if ( iArgQty < 3 )
				piCmd[2] = 1;
			if ( iArgQty < 2 )
				piCmd[1] = 1;

			CGrayUID uid = static_cast<CGrayUID>(piCmd[0]);
			pObjNear = uid.ObjFind();
			if ( !pObjNear )
				return false;
			MoveNearObj(pObjNear, static_cast<WORD>(piCmd[1]));
			if ( piCmd[2] )
				Update();
			break;
		}
		case OV_NUDGEDOWN:
		{
			EXC_SET("NUDGEDOWN");
			if ( IsTopLevel() )
			{
				signed char zdiff = static_cast<signed char>(s.GetArgVal());
				SetTopZ(GetTopZ() - (zdiff ? zdiff : 1));
				Update();
			}
			break;
		}
		case OV_NUDGEUP:
		{
			EXC_SET("NUDGEUP");
			if ( IsTopLevel() )
			{
				signed char zdiff = static_cast<signed char>(s.GetArgVal());
				SetTopZ(GetTopZ() + (zdiff ? zdiff : 1));
				Update();
			}
			break;
		}
		case OV_MOVETO:
		case OV_P:
		{
			EXC_SET("P or MOVETO");
			MoveTo(g_Cfg.GetRegionPoint(s.GetArgStr()));
			if ( IsItem() )
				Update();
			break;
		}
		case OV_PROMPTCONSOLE:
		case OV_PROMPTCONSOLEU:
		{
			EXC_SET("PROMPTCONSOLE/U");
			if ( !pClientSrc )
				return false;

			TCHAR *pszArgs[2];
			int iArgQty = Str_ParseCmds(s.GetArgRaw(), pszArgs, COUNTOF(pszArgs));
			if ( iArgQty == 0 )
				break;

			pClientSrc->addPromptConsoleFunction(pszArgs[0], pszArgs[1], (index == OV_PROMPTCONSOLEU));
			break;
		}
		case OV_INFO:
			EXC_SET("INFO");
			if ( !pClientSrc )
				return false;
			return pClientSrc->addGumpDialogProps(GetUID());
		case OV_REMOVE:
			EXC_SET("REMOVE");
			Delete();	// remove this object now
			return true;
		case OV_REMOVEFROMVIEW:
			EXC_SET("REMOVEFROMVIEW");
			RemoveFromView(NULL, false);	// remove this object from all clients
			return true;
		case OV_RESENDTOOLTIP:
		{
			EXC_SET("RESENDTOOLTIP");
			INT64 piCmd[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));

			bool bSendFull = false;
			bool bUseCache = false;
			if ( iArgQty >= 1 )
				bSendFull = (piCmd[0] != 0);
			if ( iArgQty >= 2 )
				bUseCache = (piCmd[1] != 0);

			ResendTooltip(bSendFull, bUseCache);
			return true;
		}
		case OV_SAY:		// speak so everyone can here
			EXC_SET("SAY");
			Speak(s.GetArgStr());
			break;
		case OV_SAYU:		// speak in unicode from the UTF8 system format
			EXC_SET("SAYU");
			SpeakUTF8(s.GetArgStr(), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
			break;
		case OV_SAYUA:		// this can have full args. SAYUA Color, Mode, Font, Lang, Text Text
		{
			EXC_SET("SAYUA");
			TCHAR *pszArgs[5];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), pszArgs, COUNTOF(pszArgs));
			if ( iArgQty < 5 )
				break;

			SpeakUTF8(pszArgs[4],
				static_cast<HUE_TYPE>(pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF),
				static_cast<TALKMODE_TYPE>(pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY),
				static_cast<FONT_TYPE>(pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL),
				CLanguageID(pszArgs[3]));
			break;
		}
		case OV_SOUND:
		{
			EXC_SET("SOUND");
			INT64 piCmd[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			Sound(static_cast<SOUND_TYPE>(piCmd[0]), (iArgQty > 1) ? static_cast<BYTE>(piCmd[1]) : 1);
			break;
		}
		case OV_SPELLEFFECT:	// spell, strength, noresist
		{
			EXC_SET("SPELLEFFECT");
			INT64 piCmd[4];
			size_t iArgs = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			CItem *pItemSrc = NULL;
			switch ( iArgs )
			{
				case 4:
				{
					CGrayUID uid = static_cast<CGrayUID>(piCmd[3]);
					pItemSrc = uid.ItemFind();
				}
				case 3:
					if ( piCmd[2] == -1 )
						pCharSrc = dynamic_cast<CChar *>(this);
					else
					{
						CGrayUID uid = static_cast<CGrayUID>(piCmd[2]);
						pCharSrc = uid.CharFind();
					}
					break;
				default:
					break;
			}
			OnSpellEffect(static_cast<SPELL_TYPE>(RES_GET_INDEX(piCmd[0])), pCharSrc, static_cast<int>(piCmd[1]), pItemSrc);
			break;
		}
		case OV_TAGLIST:
		{
			EXC_SET("TAGLIST");
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = &g_Serv;
			m_TagDefs.DumpKeys(pSrc, "TAG.");
			break;
		}
		case OV_PROPSLIST:
		{
			EXC_SET("PROPSLIST");
			if ( !strcmpi(s.GetArgStr(), "log") )
				pSrc = &g_Serv;
			m_BaseDefs.DumpKeys(pSrc, NULL);
			break;
		}
		case OV_TARGET:
		{
			EXC_SET("TARGET");
			if ( !pClientSrc )
				return false;
			pszKey += 6;
			bool bAllowGround = false;
			bool bCheckCrime = false;
			bool bFunction = false;
			bool bMulti = false;

			TCHAR low = static_cast<TCHAR>(tolower(*pszKey));
			while ( (low >= 'a') && (low <= 'z') )
			{
				if ( low == 'f' )
					bFunction = true;
				else if ( low == 'g' )
					bAllowGround = true;
				else if ( low == 'w' )
					bCheckCrime = true;
				else if ( low == 'm' )
					bMulti = true;
				low = static_cast<TCHAR>(tolower(*(++pszKey)));
			}

			pClientSrc->m_Targ_UID = GetUID();
			pClientSrc->m_tmUseItem.m_pParent = GetParent();	// cheat verify

			if ( bFunction )
			{
				if ( bMulti )
				{
					if ( IsStrEmpty(s.GetArgStr()) )
						break;
					char *ppArg[3];
					Str_ParseCmds(s.GetArgStr(), ppArg, COUNTOF(ppArg), ",");
					if ( !IsStrNumeric(ppArg[1]) )
						DEBUG_ERR(("Invalid argument in Target Multi\n"));
					ITEMID_TYPE itemid = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArg[1]));
					HUE_TYPE color = static_cast<HUE_TYPE>(Exp_GetVal(ppArg[2]));
					pClientSrc->addTargetFunctionMulti(ppArg[0], itemid, color, bAllowGround);
				}
				else
					pClientSrc->addTargetFunction(s.GetArgStr(), bAllowGround, bCheckCrime);
			}
			else
			{
				if ( bMulti )
				{
					char *ppArg[2];
					Str_ParseCmds(s.GetArgStr(), ppArg, COUNTOF(ppArg), ",");
					if ( !IsStrNumeric(ppArg[0]) )
						DEBUG_ERR(("Invalid argument in Target Multi\n"));
					ITEMID_TYPE itemid = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArg[0]));
					HUE_TYPE color = static_cast<HUE_TYPE>(Exp_GetVal(ppArg[1]));
					pClientSrc->addTargetItems(CLIMODE_TARG_USE_ITEM, itemid, color, bAllowGround);
				}
				else
					pClientSrc->addTarget(CLIMODE_TARG_USE_ITEM, s.GetArgStr(), bAllowGround, bCheckCrime);
			}
			break;
		}
		case OV_TIMERF:
		{
			EXC_SET("TIMERF");
			if ( !strnicmp(s.GetArgStr(), "CLEAR", 5) )
				g_World.m_TimedFunctions.Erase(GetUID());
			else if ( !strnicmp(s.GetArgStr(), "STOP", 4) )
				g_World.m_TimedFunctions.Stop(GetUID(), s.GetArgStr() + 5);
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
					if ( !*p || (strlen(p) >= 1024) )
					{
						g_Log.EventError("TimerF function name empty or args too long - total length must be less than 1024 characters\n");
						return false;
					}
					else
						g_World.m_TimedFunctions.Add(GetUID(), el, p);
				}
			}
			break;
		}
		case OV_TRIGGER:
		{
			if ( s.HasArgs() )
			{
				TRIGRET_TYPE tResult;
				CallPersonalTrigger(s.GetArgRaw(), pSrc, tResult, false);
			}
			break;
		}
		case OV_DIALOG:
		case OV_SDIALOG:
		{
			EXC_SET("DIALOG or SDIALOG");
			if ( !pClientSrc )
				return false;

			TCHAR *Arg_ppCmd[3];		// maximum parameters in one line
			size_t iQty = Str_ParseCmds(s.GetArgStr(), Arg_ppCmd, COUNTOF(Arg_ppCmd));
			if ( iQty < 1 )
				return false;

			if ( index == OV_SDIALOG )
			{
				RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, Arg_ppCmd[0]);
				int context;

				if ( pClientSrc->m_NetState->isClientKR() )
					context = g_Cfg.GetKRDialog(static_cast<DWORD>(rid)) & 0x00FFFFFF;
				else
					context = static_cast<DWORD>(rid) & 0x00FFFFFF;

				CClient::OpenedGumpsMap_t::iterator itGumpFound = pClientSrc->m_mapOpenedGumps.find(context);
				if ( pCharSrc && (itGumpFound != pClientSrc->m_mapOpenedGumps.end()) && ((*itGumpFound).second > 0) )
					break;
			}
			pClientSrc->Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, Arg_ppCmd[0]), (iQty > 1) ? Exp_GetVal(Arg_ppCmd[1]) : 0, this, Arg_ppCmd[2]);
			break;
		}
		case OV_DIALOGCLOSE:
		{
			EXC_SET("DIALOGCLOSE");
			if ( !pClientSrc )
				return false;

			TCHAR *Arg_ppCmd[2];		// maximum parameters in one line
			size_t iQty = Str_ParseCmds(s.GetArgStr(), Arg_ppCmd, COUNTOF(Arg_ppCmd));
			if ( iQty < 1 )
				return false;

			DWORD rid = g_Cfg.ResourceGetIDType(RES_DIALOG, Arg_ppCmd[0]);
			if ( pClientSrc->m_NetState->isClientKR() )
				rid = g_Cfg.GetKRDialog(rid);

			pClientSrc->Dialog_Close(this, rid, (iQty > 1) ? Exp_GetVal(Arg_ppCmd[1]) : 0);
			break;
		}
		case OV_TRYP:
		{
			EXC_SET("TRYP");
			int iMinPriv = s.GetArgVal();
			if ( iMinPriv >= PLEVEL_QTY )
			{
				pSrc->SysMessagef("The %s property can't be changed.", static_cast<LPCTSTR>(s.GetArgStr()));
				return false;
			}

			if ( pSrc->GetPrivLevel() < iMinPriv )
			{
				pSrc->SysMessagef("You lack the privilege to change the %s property.", static_cast<LPCTSTR>(s.GetArgStr()));
				return false;
			}

			// do this verb only if we can touch it.
			if ( pSrc->GetPrivLevel() <= PLEVEL_Counsel )
			{
				if ( !pCharSrc || !pCharSrc->CanTouch(this) )
				{
					pSrc->SysMessagef("Can't touch %s object %s", static_cast<LPCTSTR>(s.GetArgStr()), GetName());
					return false;
				}
			}
			// no break here, TRYP only does extra checks
		}
		case OV_TRY:
		{
			EXC_SET("TRY or TRYP");
			LPCTSTR pszVerb = s.GetArgStr();
			CScript script(pszVerb);
			if ( !r_Verb(script, pSrc) )
			{
				DEBUG_ERR(("Can't try %s object %s (0%lx)\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
				return false;
			}
			return true;
		}
		case OV_TRYSRC:
		case OV_TRYSRV:
		{
			EXC_SET("TRYSRC or TRYSRV");
			CGrayUID NewSrc;
			CTextConsole *pNewSrc = NULL;

			if ( index == OV_TRYSRC )
				NewSrc = s.GetArgVal();

			LPCTSTR pszVerb = s.GetArgStr();

			if ( index == OV_TRYSRC )
			{
				if ( NewSrc.IsValidUID() )
					pNewSrc = NewSrc.CharFind();
			}
			else
				pNewSrc = &g_Serv;

			if ( !pNewSrc )
			{
				if ( index == OV_TRYSRC )
					DEBUG_ERR(("Can't trysrc %s object %s (0%lx): invalid src uid 0%lx\n", pszVerb, GetName(), static_cast<DWORD>(GetUID()), static_cast<DWORD>(NewSrc)));
				else
					DEBUG_ERR(("Can't trysrv %s object %s (0%lx)\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
				return false;
			}

			CScript script(pszVerb);
			if ( !r_Verb(script, pNewSrc) )
			{
				if ( index == OV_TRYSRC )
					DEBUG_ERR(("Can't trysrc %s object %s (0%lx) with src %s (0%lx)\n", pszVerb, GetName(), static_cast<DWORD>(GetUID()), pNewSrc->GetName(), static_cast<DWORD>(NewSrc)));
				else
					DEBUG_ERR(("Can't trysrv %s object %s (0%lx)\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
				return false;
			}
			return true;
		}
		case OV_UID:
			EXC_SET("UID");
			// block anyone from ever doing this.
			if ( pSrc )
				pSrc->SysMessage("Setting the UID this way is not allowed");
			return false;
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
		case OV_CLICK:
		{
			EXC_SET("CLICK");
			if ( !pCharSrc || !pCharSrc->m_pClient )
				return false;
			if ( s.HasArgs() )
			{
				CGrayUID uid = static_cast<CGrayUID>(s.GetArgVal());
				if ( !uid.ObjFind() || !IsChar() )
					return false;
				pCharSrc->m_pClient->Event_SingleClick(uid);
			}
			else
				pCharSrc->m_pClient->Event_SingleClick(GetUID());
			return true;
		}
		case OV_DCLICK:
		{
			EXC_SET("DCLICK");
			if ( !pCharSrc )
				return false;
			if ( s.HasArgs() )
			{
				CGrayUID uid = static_cast<CGrayUID>(s.GetArgVal());
				if ( !uid.ObjFind() || !IsChar() )
					return false;
				CChar *pChar = static_cast<CChar *>(this);
				return pChar->Use_Obj(uid.ObjFind(), true, true);
			}
			else
				return pCharSrc->Use_Obj(this, true, true);
		}
		case OV_USEITEM:
		{
			EXC_SET("USEITEM");
			if ( !pCharSrc )
				return false;
			if ( s.HasArgs() )
			{
				CGrayUID uid = static_cast<CGrayUID>(s.GetArgVal());
				if ( !uid.ObjFind() || !IsChar() )
					return false;

				CChar *pChar = static_cast<CChar *>(this);
				return pChar->Use_Obj(uid.ObjFind(), false, true);
			}
			else
				return pCharSrc->Use_Obj(this, false, true);
		}
		case OV_FIX:
			s.GetArgStr()[0] = '\0';
		case OV_Z:
		{
			EXC_SET("FIX or Z");
			if ( IsItemEquipped() )
				return false;
			if ( s.HasArgs() )
				SetTopZ(static_cast<signed char>(s.GetArgVal()));
			else if ( IsTopLevel() )
			{
				CChar *pChar = dynamic_cast<CChar *>(this);
				CItem *pItem = dynamic_cast<CItem *>(this);
				if ( pChar )
					SetTopZ(pChar->GetFixZ(GetTopPoint()));
				else if ( pItem )
					SetTopZ(pItem->GetFixZ(GetTopPoint()));
				else
				{
					g_Log.EventDebug("Failed to get reference in FIX or Z\n");
					break;
				}
			}
			Update();
			break;
		}
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::RemoveFromView(CClient *pClientExclude, bool fHardcoded)
{
	ADDTOCALLSTACK("CObjBase::RemoveFromView");
	// Remove this item from all clients.
	// In a destructor this can do funny things.

	if ( IsDisconnected() )
		return;	// not in the world.

	CObjBaseTemplate *pObjTop = GetTopLevelObj();
	CItem *pItem = fHardcoded ? (dynamic_cast<CItem *>(this)) : NULL;
	CChar *pChar = NULL;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClientExclude == pClient )
			continue;
		pChar = pClient->GetChar();
		if ( !pChar )
			continue;
		if ( pChar->GetTopDistSight(pObjTop) > pChar->GetSight() )	// client does not support removing of items which are farther (will be removed from the radar on the next step, cause the server won't resend it)
			continue;
		if ( pItem && pItem->IsItemEquipped() )
		{
			if ( (pItem->GetEquipLayer() > LAYER_HORSE) && (pItem->GetEquipLayer() != LAYER_BANKBOX) && (pItem->GetEquipLayer() != LAYER_DRAGGING) )
				continue;
		}

		if ( GetEquipLayer() == LAYER_BANKBOX )
			pClient->closeContainer(this);

		pClient->addObjectRemove(this);
	}
}

void CObjBase::ResendOnEquip(bool fAllClients)
{
	ADDTOCALLSTACK("CObjBase::RemoveFromView");
	// Remove this item from all clients if fAllClients == true, from Enhanced Client only if not.
	// Then resend it.

	if ( IsDisconnected() )
		return;	// not in the world.

	CObjBaseTemplate *pObjTop = GetTopLevelObj();
	CItem *pItem = dynamic_cast<CItem *>(this);
	CChar *pChar = NULL;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !fAllClients && !pClient->m_NetState->isClientEnhanced() )
			continue;
		pChar = pClient->GetChar();
		if ( !pChar )
			continue;
		if ( pChar->GetTopDistSight(pObjTop) > pChar->GetSight() )	// client does not support removing of items which are farther (will be removed from the radar on the next step, cause the server won't resend it)
			continue;
		if ( pItem )
		{
			if ( pItem->IsItemEquipped() && !pChar->IsPriv(PRIV_GM) )
			{
				if ( (pItem->GetEquipLayer() > LAYER_HORSE) && (pItem->GetEquipLayer() != LAYER_BANKBOX) && (pItem->GetEquipLayer() != LAYER_DRAGGING) )
					continue;
			}
			if ( pItem->IsTypeSpellbook() && pItem->IsItemInContainer() )	// items must be removed from view before equipping in EC when on the floor, however spellbooks cannot be removed from view or client will crash
				continue;
		}

		if ( GetEquipLayer() == LAYER_BANKBOX )
			pClient->closeContainer(this);

		pClient->addObjectRemove(this);
		pClient->addItem(pItem);

	}
}

void CObjBase::SetPropertyList(PacketPropertyList *pPropertyList)
{
	ADDTOCALLSTACK("CObjBase::SetPropertyList");
	// set the property list for this object

	if ( pPropertyList == GetPropertyList() )
		return;

	FreePropertyList();
	m_PropertyList = pPropertyList;
}

void CObjBase::FreePropertyList()
{
	ADDTOCALLSTACK("CObjBase::FreePropertyList");
	// free m_PropertyList

	if ( m_PropertyList == NULL )
		return;

	delete m_PropertyList;
	m_PropertyList = NULL;
}

DWORD CObjBase::UpdatePropertyRevision(DWORD dwHash)
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyRevision");

	if ( dwHash != m_PropertyHash )
	{
		// the property list has changed, increment the revision number
		m_PropertyHash = dwHash;
		m_PropertyRevision++;
	}

	return m_PropertyRevision;
}

void CObjBase::UpdatePropertyFlag(int iMask)
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyFlag");
	if ( g_Serv.IsLoading() || ((g_Cfg.m_iAutoTooltipResend & iMask) == 0) )
		return;

	m_fStatusUpdate |= SU_UPDATE_TOOLTIP;

	// Items equipped, inside containers or with timer expired doesn't receive ticks and need to be added to a list of items to be processed separately
	if ( (!IsTopLevel() || IsTimerExpired()) && !g_World.m_ObjStatusUpdates.ContainsPtr(this) )
		g_World.m_ObjStatusUpdates.Add(this);
}

void CObjBase::OnTickStatusUpdate()
{
	ADDTOCALLSTACK("CObjBase::OnTickStatusUpdate");
	// process m_fStatusUpdate flags

	if ( m_fStatusUpdate & SU_UPDATE_TOOLTIP )
		ResendTooltip();
}

void CObjBase::ResendTooltip(bool bSendFull, bool bUseCache)
{
	ADDTOCALLSTACK("CObjBase::ResendTooltip");
	// Send tooltip packet to all nearby clients

	m_fStatusUpdate &= ~SU_UPDATE_TOOLTIP;
	if ( IsDisconnected() || g_Serv.IsLoading() )
		return;	// not in the world.

	if ( !bUseCache )
		FreePropertyList();

	CChar *pChar = NULL;
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->m_TooltipEnabled )
			continue;

		pChar = pClient->GetChar();
		if ( !pChar || !pChar->CanSee(this) )
			continue;

		pClient->addAOSTooltip(this, bSendFull);
	}
}

void CObjBase::DeletePrepare()
{
	ADDTOCALLSTACK("CObjBase::DeletePrepare");
	// Prepare to delete.
	RemoveFromView();
	RemoveSelf();	// Must remove early or else virtuals will fail.
}

void CObjBase::Delete(bool bForce)
{
	ADDTOCALLSTACK("CObjBase::Delete");
	UNREFERENCED_PARAMETER(bForce);		// CObjBase doesnt use it, but CItem and CChar does use it, do not remove

	if ( m_uidSpawnItem.ItemFind() )
		static_cast<CItemSpawn *>(m_uidSpawnItem.ItemFind())->DelObj(GetUID());

	DeletePrepare();
	g_World.m_TimedFunctions.Erase(GetUID());
	g_World.m_ObjDelete.InsertHead(this);
}

bool CObjBase::IsTriggerActive(LPCTSTR trig)
{
	return (m_RunningTrigger == trig);
}

LPCTSTR CObjBase::GetTriggerActive()
{
	return m_RunningTrigger ? m_RunningTrigger : "none";
}

void CObjBase::SetTriggerActive(LPCTSTR trig)
{
	if ( trig )
	{
		char *text = Str_GetTemp();
		sprintf(text, "Trigger: %s", trig);
		ADDTOCALLSTACK(text);
	}
	m_RunningTrigger = trig ? trig : NULL;
}

TRIGRET_TYPE CObjBase::Spell_OnTrigger(SPELL_TYPE spell, SPTRIG_TYPE stage, CChar *pSrc, CScriptTriggerArgs *pArgs)
{
	ADDTOCALLSTACK("CObjBase::Spell_OnTrigger");
	CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return TRIGRET_RET_TRUE;

	if ( pSpellDef->HasTrigger(stage) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSpellDef->ResourceLock(s) )
			return CScriptObj::OnTriggerScript(s, CSpellDef::sm_szTrigName[stage], pSrc, pArgs);
	}
	return TRIGRET_RET_DEFAULT;
}

inline bool CObjBase::CallPersonalTrigger(TCHAR *pszArgs, CTextConsole *pSrc, TRIGRET_TYPE &trResult, bool bFull)
{
	ADDTOCALLSTACK("CObjBase::CallPersonalTrigger");
	UNREFERENCED_PARAMETER(bFull);
	TCHAR *ppCmdTrigger[3];
	size_t iResultArgs = Str_ParseCmds(pszArgs, ppCmdTrigger, COUNTOF(ppCmdTrigger), ",");
	if ( iResultArgs > 0 )
	{
		LPCTSTR callTrigger = ppCmdTrigger[0];
		CScriptTriggerArgs csTriggerArgs;

		if ( iResultArgs == 3 )
		{
			int iTriggerArgType = ATOI(ppCmdTrigger[1]);
			if ( iTriggerArgType == 1 )		// 3 ARGNs
			{
				INT64 Arg_piCmd[3];
				iResultArgs = Str_ParseCmds(ppCmdTrigger[2], Arg_piCmd, COUNTOF(Arg_piCmd), ",");

				if ( iResultArgs == 3 )
					csTriggerArgs.m_iN3 = Arg_piCmd[2];
				if ( iResultArgs >= 2 )
					csTriggerArgs.m_iN2 = Arg_piCmd[1];
				if ( iResultArgs >= 1 )
					csTriggerArgs.m_iN1 = Arg_piCmd[0];
			}
			else if ( iTriggerArgType == 2 )	// ARGS
			{
				csTriggerArgs.m_s1 = ppCmdTrigger[2];
				csTriggerArgs.m_s1_raw = ppCmdTrigger[2];
			}
			else if ( iTriggerArgType == 3 )	// ARGO
			{
				CGrayUID guTriggerArg = static_cast<CGrayUID>(Exp_GetVal(ppCmdTrigger[2]));
				CObjBase *pTriggerArgObj = guTriggerArg.ObjFind();
				if ( pTriggerArgObj )
					csTriggerArgs.m_pO1 = pTriggerArgObj;
			}
			else if ( iTriggerArgType == 4 )	// FULL TRIGGER
			{
				TCHAR *Arg_ppCmd[5];
				iResultArgs = Str_ParseCmds(ppCmdTrigger[2], Arg_ppCmd, COUNTOF(Arg_ppCmd), ",");

				// ARGS
				if ( iResultArgs == 5 )
				{
					csTriggerArgs.m_s1 = Arg_ppCmd[4];
					csTriggerArgs.m_s1_raw = Arg_ppCmd[4];
				}
				// ARGNs
				if ( iResultArgs >= 4 )
					csTriggerArgs.m_iN3 = Exp_GetVal(Arg_ppCmd[3]);
				if ( iResultArgs >= 3 )
					csTriggerArgs.m_iN2 = Exp_GetVal(Arg_ppCmd[2]);
				if ( iResultArgs >= 2 )
					csTriggerArgs.m_iN1 = Exp_GetVal(Arg_ppCmd[1]);
				// ARGO
				if ( iResultArgs >= 1 )
				{
					CGrayUID guTriggerArg = static_cast<CGrayUID>(Exp_GetVal(Arg_ppCmd[0]));
					CObjBase *pTriggerArgObj = guTriggerArg.ObjFind();
					if ( pTriggerArgObj )
						csTriggerArgs.m_pO1 = pTriggerArgObj;
				}
			}
		}

		trResult = OnTrigger(callTrigger, pSrc, &csTriggerArgs);
		return true;
	}
	return false;
}
