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
	size_t iArgQty = Str_ParseCmds(pszDir, ppCmd, COUNTOF(ppCmd));
	if ( iArgQty <= 0 )
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

///////////////////////////////////////////////////////////
// CObjBase

CObjBase::CObjBase(bool fItem)
{
	m_timeout.Init();
	m_timestamp.Init();
	m_wHue = HUE_DEFAULT;
	m_RunningTrigger = NULL;

	m_Can = 0;
	m_ModMaxWeight = 0;

	m_attackBase = 0;
	m_attackRange = 0;

	m_defenseBase = 0;
	m_defenseRange = 0;

	m_DamPhysical = 0;
	m_DamFire = 0;
	m_DamCold = 0;
	m_DamPoison = 0;
	m_DamEnergy = 0;

	m_ResPhysical = 0;
	m_ResPhysicalMax = 0;
	m_ResFire = 0;
	m_ResFireMax = 0;
	m_ResCold = 0;
	m_ResColdMax = 0;
	m_ResPoison = 0;
	m_ResPoisonMax = 0;
	m_ResEnergy = 0;
	m_ResEnergyMax = 0;

	m_Luck = 0;
	m_DamIncrease = 0;
	m_SpellDamIncrease = 0;
	m_HitLifeLeech = 0;
	m_HitManaDrain = 0;
	m_HitManaLeech = 0;
	m_HitStaminaLeech = 0;
	m_HitChanceIncrease = 0;
	m_DefChanceIncrease = 0;
	m_DefChanceIncreaseMax = 0;
	m_SwingSpeedIncrease = 0;
	m_FasterCasting = 0;
	m_FasterCastRecovery = 0;
	m_LowerManaCost = 0;
	m_LowerReagentCost = 0;
	m_EnhancePotions = 0;
	m_NightSight = 0;
	m_ReflectPhysicalDamage = 0;

	m_uidSpawnItem = UID_UNUSED;

	++sm_iCount;

	m_ModAr = 0;

	m_fStatusUpdate = 0;

	m_PropertyList = NULL;
	m_PropertyHash = 0;
	m_PropertyRevision = 0;

	if ( g_Serv.IsLoading() )
	{
		// Don't do this yet if we are loading, UID will be set later. Just check if this is an item
		CObjBaseTemplate::SetUID(UID_O_DISCONNECT|UID_O_INDEX_MASK|(fItem ? UID_F_ITEM : 0));
	}
	else
	{
		// Find a free UID slot for this
		SetUID(UID_CLEAR, fItem);
		ASSERT(IsValidUID());
		SetContainerFlags(UID_O_DISCONNECT);	// it is no place for now
	}

	// Put in the idle list while this don't get moved to world
	g_World.m_ObjNew.InsertHead(this);
}

CObjBase::~CObjBase()
{
	FreePropertyList();
	g_World.m_ObjStatusUpdates.RemovePtr(this);

	--sm_iCount;
	ASSERT(IsDisconnected());

	// Free up the UID slot
	SetUID(UID_UNUSED, false);
}

bool CObjBase::IsContainer() const
{
	ADDTOCALLSTACK("CObjBase::IsContainer");
	// Simple test if object is a container.
	return (dynamic_cast<const CContainer *>(this) != NULL);
}

void CObjBase::SetHue(HUE_TYPE wHue, bool fSkipTrigger, CTextConsole *pSrc, CObjBase *pObj, SOUND_TYPE wSound)
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

	if ( IsTrigUsed("@Dye") && !fSkipTrigger )
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
			pFace->SetHue(m_wHue, fSkipTrigger, pSrc, pObj);
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

bool CObjBase::SetNamePool(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CObjBase::SetNamePool");
	ASSERT(pszName);
	TCHAR szTemp[MAX_ITEM_NAME_SIZE];

	if ( pszName[0] == '#' )
	{
		// Pick random name from the given #NAMES list
		++pszName;
		strncpy(szTemp, pszName, sizeof(szTemp) - 1);

		TCHAR *ppArgs[2];
		Str_ParseCmds(szTemp, ppArgs, COUNTOF(ppArgs));

		CResourceLock s;
		if ( !g_Cfg.ResourceLock(s, RES_NAMES, ppArgs[0]) || !s.ReadKey() )
			return false;

		int iCount = Calc_GetRandVal(ATOI(s.GetKey())) + 1;
		while ( iCount-- )
		{
			if ( !s.ReadKey() )
			{
				DEBUG_ERR(("Name list '#%s' have invalid entries\n", ppArgs[0]));
				return false;
			}
		}

		strncpy(szTemp, s.GetKey(), sizeof(szTemp) - 1);
	}
	else
	{
		// Directly set the given name
		strncpy(szTemp, pszName, sizeof(szTemp) - 1);
	}

	if ( !CObjBaseTemplate::SetName(szTemp) )
		return false;

	UpdatePropertyFlag();
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
		if ( !g_Cfg.m_fSaveGarbageCollect && g_World.FixObj(this) )
			return;

		r_Write(s);
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Write Object 0%" FMTDWORDH, uid);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )	// catch all
	{
		g_Log.CatchEvent(NULL, "Write Object 0%" FMTDWORDH, uid);
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

void CObjBase::Sound(SOUND_TYPE id, BYTE bRepeat) const
{
	ADDTOCALLSTACK("CObjBase::Sound");
	// Send sound to all nearby clients

	if ( !g_Cfg.m_fGenericSounds || (id <= SOUND_NONE) )
		return;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( !pClient->CanHear(this, TALKMODE_OBJ) )
			continue;
		pClient->addSound(id, this, bRepeat);
	}
}

void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate *pSrc, BYTE bSpeed, BYTE bFrames, bool fExplode, DWORD dwColor, DWORD dwRender, WORD wEffectID, WORD wExplodeID, WORD wExplodeSound, DWORD dwItemUID, BYTE bLayer) const
{
	ADDTOCALLSTACK("CObjBase::Effect");
	// Object-based effect

	if ( motion == EFFECT_FADE_SCREEN )
	{
		// This effect must be used only on client chars (and send it only to this client)
		const CChar *pChar = dynamic_cast<const CChar *>(this);
		if ( pChar && pChar->m_pClient )
			pChar->m_pClient->addEffect(motion, NULL, NULL, NULL, NULL, id);
		return;
	}

	CPointMap ptSrc = NULL;
	CObjBaseTemplate *pDest = GetTopLevelObj();
	CPointMap ptDest = pDest->GetTopPoint();

	if ( motion == EFFECT_BOLT )
	{
		// Source should be a valid object when using moving effects
		if ( !pSrc )
			return;
		pSrc = pSrc->GetTopLevelObj();
		ptSrc = pSrc->GetTopPoint();
	}
	else
	{
		// Otherwise, source is not needed (set the same as dest just to fill the packet)
		pSrc = pDest;
		ptSrc = ptDest;
	}

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		CChar *pChar = pClient->GetChar();
		if ( !pChar || !pChar->CanSee(pDest) )
			continue;
		pClient->addEffect(motion, pSrc, ptSrc, pDest, ptDest, id, bSpeed, bFrames, fExplode, dwColor, dwRender, wEffectID, wExplodeID, wExplodeSound, dwItemUID, bLayer);
	}
}

void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, CPointMap ptSrc, CPointMap ptDest, BYTE bSpeed, BYTE bFrames, bool fExplode, DWORD dwColor, DWORD dwRender, WORD wEffectID, WORD wExplodeID, WORD wExplodeSound, DWORD dwItemUID, BYTE bLayer) const
{
	ADDTOCALLSTACK("CObjBase::Effect(2)");
	// Ground-based effect

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		CChar *pChar = pClient->GetChar();
		if ( !pChar || (pChar->GetTopPoint().GetDist(ptDest) > pChar->GetSight()) )
			continue;
		pClient->addEffect(motion, NULL, ptSrc, NULL, ptDest, id, bSpeed, bFrames, fExplode, dwColor, dwRender, wEffectID, wExplodeID, wExplodeSound, dwItemUID, bLayer);
	}
}

void CObjBase::Emote(LPCTSTR pszTextYou, LPCTSTR pszTextThem, CClient *pClientExclude)
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// Show message as *emote*
	// ARGS:
	//  pszTextYou = text to client itself
	//  pszTextThem = text to clients nearby (if null, inherit the same message from pszTextYou)

	CObjBase *pObjTop = static_cast<CObjBase *>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	HUE_TYPE defaultHue = HUE_TEXT_DEF;
	FONT_TYPE defaultFont = FONT_NORMAL;
	bool defaultUnicode = false;

	if ( pszTextYou && (*pszTextYou == '@') )
	{
		++pszTextYou;
		const char *s = pszTextYou;
		pszTextYou = strchr(s, ' ');

		WORD wArgs[] = { static_cast<WORD>(defaultHue), static_cast<WORD>(defaultFont), static_cast<WORD>(defaultUnicode) };

		if ( pszTextYou )
		{
			for ( int i = 0; (s < pszTextYou) && (i < 3); ++i, ++s )
			{
				if ( *s != ',' )
					wArgs[i] = static_cast<WORD>(Exp_GetLLVal(s));
			}
			++pszTextYou;
		}

		defaultHue = static_cast<HUE_TYPE>(wArgs[0]);
		defaultFont = static_cast<FONT_TYPE>(wArgs[1]);
		defaultUnicode = static_cast<bool>(wArgs[2]);
	}

	if ( !pszTextThem )
		pszTextThem = pszTextYou;

	TCHAR *pszOthers = Str_GetTemp();
	TCHAR *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		if ( pObjTop != this )
		{
			// Equipped items
			sprintf(pszOthers, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_1), pObjTop->GetName(), GetName(), pszTextThem);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pszTextYou);
		}
		else
		{
			// Chars
			sprintf(pszOthers, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_5), GetName(), pszTextThem);
			sprintf(pszYou, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pszTextYou);
		}
	}
	else
	{
		// Items on ground or inside container
		sprintf(pszOthers, g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_7), GetName(), pszTextThem);
		strcpy(pszYou, pszOthers);
	}

	pObjTop->UpdateObjMessage(pszOthers, pszYou, pClientExclude, defaultHue, TALKMODE_EMOTE, defaultFont, defaultUnicode);
}

void CObjBase::Speak(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font)
{
	ADDTOCALLSTACK("CObjBase::Speak");
	g_World.Speak(this, pszText, wHue, mode, font);
}

void CObjBase::SpeakUTF8(LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8");
	// convert UTF8 to UNICODE.
	NCHAR szBuffer[MAX_TALK_BUFFER];
	CvtSystemToNUNICODE(szBuffer, COUNTOF(szBuffer), pszText, -1);
	g_World.SpeakUNICODE(this, szBuffer, wHue, mode, font, lang);
}

void CObjBase::SpeakUTF8Ex(const NWORD *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CObjBase::SpeakUTF8Ex");
	g_World.SpeakUNICODE(this, pszText, wHue, mode, font, lang);
}

bool CObjBase::MoveNear(CPointMap pt, WORD wSteps)
{
	ADDTOCALLSTACK("CObjBase::MoveNear");
	// Move to nearby this other object.
	// Actually move it within +/- iSteps

	CChar *pChar = dynamic_cast<CChar *>(this);
	CPointMap ptOld = pt;

	for ( WORD i = 0; i < 20; ++i )
	{
		pt = ptOld;
		pt.m_x += static_cast<signed short>(Calc_GetRandVal(-wSteps, wSteps));
		pt.m_y += static_cast<signed short>(Calc_GetRandVal(-wSteps, wSteps));

		if ( !MoveTo(pt) )
			continue;

		if ( pChar )
		{
			pChar->m_zClimbHeight = 0;
			if ( !pChar->CanMoveWalkTo(pt, false) )
				continue;
		}
		else
			Update();
		return true;
	}
	return false;
}

void CObjBase::UpdateObjMessage(LPCTSTR pszTextThem, LPCTSTR pszTextYou, CClient *pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool fUnicode) const
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

		pClient->addBarkParse((pClient->GetChar() == this) ? pszTextYou : pszTextThem, this, wHue, mode, font, fUnicode);
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

		TRIGRET_TYPE tr = CObjBase::OnTriggerRunVal(s, TRIGRUN_SECTION_EXEC, pSrc, &Args);
		if ( tr != TRIGRET_RET_FALSE )
			return tr;

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
	int index = FindTableHeadSorted(pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys) - 1);
	if ( index >= 0 )
	{
		pszKey += strlen(sm_szRefKeys[index]);
		SKIP_SEPARATORS(pszKey);
		switch ( index )
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
			++pszArgs;
			SKIP_SEPARATORS(pszArgs);
		}

		CScriptTriggerArgs Args(pszArgs ? pszArgs : "");
		if ( r_Call(pszKey, pSrc, &Args, &sVal) )
			return true;
		if ( Base_GetDef()->r_WriteVal(pszKey, sVal, pSrc) )
			return true;

		return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
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
			const CChar *pChar = dynamic_cast<const CChar *>(this);
			if ( pChar )
			{
				sVal.FormatVal(pChar->m_defense);
				break;
			}

			pszKey += 5;
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
			pszKey += 3;
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
			bool fCanSee = (index == OC_CANSEE);
			bool fUseFlags = (index == OC_CANSEELOSFLAG);
			WORD wFlags = 0;

			pszKey += fCanSee ? 6 : (fUseFlags ? 13 : 9);
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			if ( fUseFlags && *pszKey )
			{
				wFlags = static_cast<WORD>(Exp_GetVal(pszKey));
				SKIP_ARGSEP(pszKey);
			}

			if ( *pszKey )		// has an argument - UID to see(los) or POS to los only
			{
				CPointMap pt;
				CObjBase *pObj = NULL;

				if ( !fCanSee )
					pt = g_Cfg.GetRegionPoint(pszKey);

				if ( fCanSee || !pt.IsValidPoint() )
				{
					pObj = static_cast<CGrayUID>(Exp_GetVal(pszKey)).ObjFind();
					if ( !fCanSee && pObj )
						pt = pObj->GetTopPoint();
				}

				pChar = GetUID().CharFind();
				if ( pChar )
					sVal.FormatVal(fCanSee ? pChar->CanSee(pObj) : pChar->CanSeeLOS(pt, NULL, pChar->GetSight(), wFlags));
				else
					sVal.FormatVal(0);
			}
			else
			{
				if ( pChar )	// standard way src TO current object
					sVal.FormatVal(fCanSee ? pChar->CanSee(this) : pChar->CanSeeLOS(this, wFlags));
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
			sVal.FormatVal((pChar && pChar->m_pClient) ? pChar->m_pClient->m_TagDefs.GetCount() : 0);
			break;
		}
		case OC_TEXTF:
		{
			TCHAR *pszFormat = const_cast<TCHAR *>(pszKey);
			pszFormat += 5;

			TCHAR *pszArg[4];
			size_t iArgQty = Str_ParseCmds(pszFormat, pszArg, COUNTOF(pszArg));
			if ( iArgQty < 2 )
			{
				g_Log.EventError("%s: function can't have less than 2 args\n", sm_szLoadKeys[index]);
				return false;
			}
			if ( iArgQty > 4 )
			{
				g_Log.EventError("%s: function exceeded max args allowed (%" FMTSIZE_T "/%d)\n", sm_szLoadKeys[index], iArgQty, 4);
				return false;
			}

			if ( *pszArg[0] == '"' )	// skip quotes
				++pszArg[0];

			BYTE bCount = 0;
			for ( TCHAR *pszEnd = pszArg[0] + strlen(pszArg[0]) - 1; pszEnd >= pszArg[0]; --pszEnd )
			{
				if ( *pszEnd == '"' )
				{
					*pszEnd = '\0';
					break;
				}
				++bCount;
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
						CClient::OpenedGumpsMap_t *pDialogList = &pClient->m_mapOpenedGumps;
						size_t iDialogIndex = static_cast<size_t>(Exp_GetVal(pszKey));
						SKIP_SEPARATORS(pszKey);

						if ( iDialogIndex <= pDialogList->size() )
						{
							CClient::OpenedGumpsMap_t::iterator itDialogFound = pDialogList->begin();
							while ( iDialogIndex-- )
								++itDialogFound;

							if ( !strnicmp(pszKey, "ID", 2) )
								sVal.Format("%s", g_Cfg.ResourceGetName(RESOURCE_ID(RES_DIALOG, (*itDialogFound).first)));
							else if ( !strnicmp(pszKey, "COUNT", 5) )
								sVal.FormatVal((*itDialogFound).second);
						}
					}
				}
				else
					DEBUG_ERR(("DIALOGLIST called on non-client object\n"));
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
				CPointMap pt = g_Cfg.GetRegionPoint(pszKey);

				if ( pt.IsValidPoint() )
				{
					if ( !pThis->GetTopPoint().IsValidPoint() )
						return false;
					sVal.FormatVal(pThis->GetTopDist(pt));
					return true;
				}
				pObj = static_cast<CGrayUID>(Exp_GetVal(pszKey)).ObjFind();
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
					sVal.FormatVal(pThis->GetTopPoint().GetDir(pt));
					return true;
				}
				pObj = static_cast<CGrayUID>(Exp_GetVal(pszKey)).ObjFind();
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
			bool fReturnP = false;
			pszKey += (index == OC_ISNEARTYPETOP) ? 13 : 10;
			if ( !strnicmp(pszKey, ".P", 2) )
			{
				fReturnP = true;
				pszKey += 2;
			}
			SKIP_SEPARATORS(pszKey);
			SKIP_ARGSEP(pszKey);

			if ( GetTopPoint().IsValidPoint() )
			{
				IT_TYPE iType = static_cast<IT_TYPE>(g_Cfg.ResourceGetIndexType(RES_TYPEDEF, pszKey));

				SKIP_IDENTIFIERSTRING(pszKey);
				SKIP_SEPARATORS(pszKey);
				SKIP_ARGSEP(pszKey);

				int iDistance = *pszKey ? Exp_GetVal(pszKey) : 0;
				bool fCheckMulti = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;
				bool fLimitZ = *pszKey ? (Exp_GetVal(pszKey) != 0) : false;

				if ( fReturnP )
				{
					CPointMap pt = (index == OC_ISNEARTYPETOP) ? g_World.FindTypeNear_Top(GetTopPoint(), iType, iDistance) : g_World.FindItemTypeNearby(GetTopPoint(), iType, iDistance, fCheckMulti, fLimitZ);
					if ( pt.IsValidPoint() )
						sVal = pt.WriteUsed();
					else
						sVal.FormatVal(0);
				}
				else
					sVal.FormatVal((index == OC_ISNEARTYPETOP) ? g_World.IsTypeNear_Top(GetTopPoint(), iType, iDistance) : g_World.IsItemTypeNear(GetTopPoint(), iType, iDistance, fCheckMulti, fLimitZ));
			}
			else
				sVal.FormatVal(0);
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
				DWORD context = static_cast<DWORD>(g_Cfg.ResourceGetIDType(RES_DIALOG, pszKey));
				if ( pClient->m_NetState->isClientKR() )
					context = g_Cfg.GetKRDialog(context);
				context &= 0xFFFFFF;

				CClient::OpenedGumpsMap_t::iterator itGumpFound = pClient->m_mapOpenedGumps.find(static_cast<int>(context));
				sVal.FormatVal((itGumpFound != pClient->m_mapOpenedGumps.end()) ? (*itGumpFound).second : 0);
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

				pItem = dynamic_cast<CItem *>(static_cast<CGrayUID>(Exp_GetVal(pszKey)).ObjFind());
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

				pItem = dynamic_cast<CItem *>(static_cast<CGrayUID>(Exp_GetVal(pszKey)).ObjFind());
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
			++pszKey;
			// fall through
		case OC_TAG:
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
				TRIGRET_TYPE tr;
				bool fRet = CallPersonalTrigger(const_cast<TCHAR *>(pszKey), pSrc, tr, false);
				if ( fRet )
					sVal.FormatVal(tr);

				return fRet;
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
			// fall through
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
			SKIP_SEPARATORS(pszKey);
			GETNONWHITESPACE(pszKey);

			CPointMap pt = *pszKey ? g_Cfg.GetRegionPoint(pszKey) : GetTopPoint();
			if ( !pt.IsValidPoint() )
				return false;

			sVal = g_Cfg.Calc_MaptoSextant(pt);
			break;
		}
		case OC_TIMESTAMP:
			sVal.FormatLLVal(GetTimeStamp().GetTimeRaw());
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
				else if ( !strnicmp(pszKey, "KEY", 3) )
				{
					sVal = pTagAt->GetKey();
					return true;
				}
				else if ( !strnicmp(pszKey, "VAL", 3) )
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
				else if ( !strnicmp(pszKey, "KEY", 3) )
				{
					sVal = pTagAt->GetKey();
					return true;
				}
				else if ( !strnicmp(pszKey, "VAL", 3) )
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

	bool fSendUpdate = false;

	switch ( index )
	{
		// Set as numeric
		case OC_DAMCHAOS:
		case OC_DAMDIRECT:
		case OC_EXPANSION:
		case OC_NAMELOC:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			fSendUpdate = true;
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
			fSendUpdate = true;
			break;

		case OC_ARMOR:
		{
			if ( IsChar() )
				return false;

			INT64 piVal[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_defenseBase = static_cast<WORD>(piVal[0]);
			m_defenseRange = (iArgQty > 1) ? static_cast<WORD>(piVal[1]) - m_defenseBase : 0;
			fSendUpdate = true;
			break;
		}
		case OC_DAM:
		{
			INT64 piVal[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			m_attackBase = static_cast<WORD>(piVal[0]);
			m_attackRange = (iArgQty > 1) ? static_cast<WORD>(piVal[1]) - m_attackBase : 0;
			fSendUpdate = true;
			break;
		}
		case OC_DAMCOLD:
			m_DamCold = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_DAMENERGY:
			m_DamEnergy = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_DAMFIRE:
			m_DamFire = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_DAMPHYSICAL:
			m_DamPhysical = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_DAMPOISON:
			m_DamPoison = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASEDAM:
			m_DamIncrease = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASEDEFCHANCE:
			m_DefChanceIncrease = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASEDEFCHANCEMAX:
			m_DefChanceIncreaseMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASEHITCHANCE:
			m_HitChanceIncrease = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASESPELLDAM:
			m_SpellDamIncrease = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_INCREASESWINGSPEED:
			m_SwingSpeedIncrease = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_FASTERCASTING:
			m_FasterCasting = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_FASTERCASTRECOVERY:
			m_FasterCastRecovery = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_HITLEECHLIFE:
			m_HitLifeLeech = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_HITLEECHMANA:
			m_HitManaLeech = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_HITLEECHSTAM:
			m_HitStaminaLeech = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_HITMANADRAIN:
			m_HitManaDrain = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_LOWERMANACOST:
			m_LowerManaCost = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_LOWERREAGENTCOST:
			m_LowerReagentCost = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_ENHANCEPOTIONS:
			m_EnhancePotions = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_NIGHTSIGHT:
			m_NightSight = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_REFLECTPHYSICALDAM:
			m_ReflectPhysicalDamage = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RANGE:
		{
			INT64 piVal[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piVal, COUNTOF(piVal));
			if ( iArgQty > 1 )
			{
				INT64 iRange = ((piVal[0] & 0xFF) << 8) & 0xFF00;
				iRange |= (piVal[1] & 0xFF);
				SetDefNum(s.GetKey(), iRange, false);
			}
			else
				SetDefNum(s.GetKey(), piVal[0], false);
			fSendUpdate = true;
			break;
		}
		case OC_CAN:
			m_Can = static_cast<DWORD>(s.GetArgVal());
			break;
		case OC_MODMAXWEIGHT:
			m_ModMaxWeight = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_COLOR:
		{
			if ( !strcmpi(s.GetArgStr(), "match_shirt") || !strcmpi(s.GetArgStr(), "match_hair") )
			{
				const CChar *pChar = dynamic_cast<const CChar *>(GetTopLevelObj());
				if ( pChar )
				{
					const CItem *pMatch = pChar->LayerFind(!strcmpi(s.GetArgStr() + 6, "shirt") ? LAYER_SHIRT : LAYER_HAIR);
					if ( pMatch )
					{
						m_wHue = pMatch->GetHue();
						break;
					}
				}
				m_wHue = HUE_DEFAULT;
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
			fSendUpdate = true;
			break;
		case OC_MAP:
		{
			// Move to another map
			if ( !IsTopLevel() )
				return false;

			CPointMap pt = GetTopPoint();
			pt.m_map = static_cast<BYTE>(s.GetArgVal());

			// Is the desired mapplane allowed?
			if ( !g_MapList.IsMapSupported(pt.m_map) )
				return false;

			MoveTo(pt);
			if ( IsItem() )
				Update();
			break;
		}
		case OC_MODAR:
		{
			m_ModAr = s.GetArgVal();
			CChar *pChar = dynamic_cast<CChar *>(this);
			if ( pChar )
				pChar->m_defense = pChar->CalcArmorDefense();
			fSendUpdate = true;
			break;
		}
		case OC_NAME:
			SetName(static_cast<LPCTSTR>(s.GetArgStr()));
			fSendUpdate = true;
			break;
		case OC_P:
			return false;	// must set the point via the CItem or CChar methods
		case OC_RESCOLD:
			m_ResCold = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESCOLDMAX:
			m_ResColdMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESENERGY:
			m_ResEnergy = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESENERGYMAX:
			m_ResEnergyMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESFIRE:
			m_ResFire = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESFIREMAX:
			m_ResFireMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESPHYSICAL:
			m_ResPhysical = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESPHYSICALMAX:
			m_ResPhysicalMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESPOISON:
			m_ResPoison = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
		case OC_RESPOISONMAX:
			m_ResPoisonMax = static_cast<int>(s.GetArgVal());
			fSendUpdate = true;
			break;
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

	if ( fSendUpdate )
	{
		if ( IsItem() )
			UpdatePropertyFlag();
		else
			static_cast<CChar *>(this)->UpdateStatsFlag();
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

	LPCTSTR pszKey = s.GetKey();
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

	switch ( index )
	{
		case OV_DAMAGE:	// "Dmg, SourceFlags, SourceCharUid, DmgPhysical(%), DmgFire(%), DmgCold(%), DmgPoison(%), DmgEnergy(%)" = do me some damage.
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
					(iArgQty >= 2) ? static_cast<DAMAGE_TYPE>(piCmd[1]) : DAMAGE_HIT_BLUNT|DAMAGE_GENERAL,
					(iArgQty >= 4) ? static_cast<int>(piCmd[3]) : 0,		// physical damage %
					(iArgQty >= 5) ? static_cast<int>(piCmd[4]) : 0,		// fire damage %
					(iArgQty >= 6) ? static_cast<int>(piCmd[5]) : 0,		// cold damage %
					(iArgQty >= 7) ? static_cast<int>(piCmd[6]) : 0,		// poison damage %
					(iArgQty >= 8) ? static_cast<int>(piCmd[7]) : 0			// energy damage %
				);
			else if ( pItem )
				pItem->OnTakeDamage(static_cast<int>(piCmd[0]),
					pCharSrc,
					(iArgQty >= 2) ? static_cast<DAMAGE_TYPE>(piCmd[1]) : DAMAGE_HIT_BLUNT|DAMAGE_GENERAL
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
		case OV_EFFECT:
		{
			EXC_SET("EFFECT");
			// Visual effect based on obj location
			// Syntax: EFFECT motion, id, speed, frames, [explode, color, render, effectID, explodeID, explodeSound, itemUID, layer]

			INT64 piCmd[12];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty < 2 )
				return true;

			Effect(
				static_cast<EFFECT_TYPE>(piCmd[0]),						// Motion
				static_cast<ITEMID_TYPE>(RES_GET_INDEX(piCmd[1])),		// ID
				pCharSrc,												// Source
				(iArgQty >= 3) ? static_cast<BYTE>(piCmd[2]) : 0,		// Speed
				(iArgQty >= 4) ? static_cast<BYTE>(piCmd[3]) : 0,		// Frames
				(iArgQty >= 5) ? (piCmd[4] != 0) : false,				// Explode
				(iArgQty >= 6) ? static_cast<DWORD>(piCmd[5]) : 0,		// Color
				(iArgQty >= 7) ? static_cast<DWORD>(piCmd[6]) : 0,		// Render
				(iArgQty >= 8) ? static_cast<WORD>(piCmd[7]) : 0,		// EffectID
				(iArgQty >= 9) ? static_cast<WORD>(piCmd[8]) : 0,		// ExplodeID
				(iArgQty >= 10) ? static_cast<WORD>(piCmd[9]) : 0,		// ExplodeSound
				(iArgQty >= 11) ? static_cast<DWORD>(piCmd[10]) : 0,	// ItemUID
				(iArgQty >= 12) ? static_cast<BYTE>(piCmd[11]) : 0		// Layer
			);
			break;
		}
		case OV_EFFECTP:
		{
			EXC_SET("EFFECTP");
			// Visual effect based on P location
			// Syntax: EFFECTLOC srcX, srcY, srcZ, srcM, destX, destY, destZ, destM, motion, id, speed, frames, [explode, color, render, effectID, explodeID, explodeSound, itemUID, layer]

			INT64 piCmd[20];
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty < 10 )
				return true;

			Effect(
				static_cast<EFFECT_TYPE>(piCmd[8]),						// Motion
				static_cast<ITEMID_TYPE>(RES_GET_INDEX(piCmd[9])),		// ID
				CPointMap(static_cast<signed short>(piCmd[0]), static_cast<signed short>(piCmd[1]), static_cast<signed char>(piCmd[2]), static_cast<BYTE>(piCmd[3])),	// Source P
				CPointMap(static_cast<signed short>(piCmd[4]), static_cast<signed short>(piCmd[5]), static_cast<signed char>(piCmd[6]), static_cast<BYTE>(piCmd[7])),	// Dest P
				(iArgQty >= 11) ? static_cast<BYTE>(piCmd[10]) : 0,		// Speed
				(iArgQty >= 12) ? static_cast<BYTE>(piCmd[11]) : 0,		// Frames
				(iArgQty >= 13) ? (piCmd[12] != 0) : false,				// Explode
				(iArgQty >= 14) ? static_cast<DWORD>(piCmd[13]) : 0,	// Color
				(iArgQty >= 15) ? static_cast<DWORD>(piCmd[14]) : 0,	// Render
				(iArgQty >= 16) ? static_cast<WORD>(piCmd[15]) : 0,		// EffectID
				(iArgQty >= 17) ? static_cast<WORD>(piCmd[16]) : 0,		// ExplodeID
				(iArgQty >= 18) ? static_cast<WORD>(piCmd[17]) : 0,		// ExplodeSound
				(iArgQty >= 19) ? static_cast<DWORD>(piCmd[18]) : 0,	// ItemUID
				(iArgQty >= 20) ? static_cast<BYTE>(piCmd[19]) : 0		// Layer
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
		{
			// "INPDLG" verb maxchars
			// else assume it was a property button
			EXC_SET("INPDLG");
			if ( !pClientSrc )
				return false;

			TCHAR *ppArgs[2];		// Maximum parameters in one line
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));

			CGString sOrgValue;
			if ( !r_WriteVal(ppArgs[0], sOrgValue, pSrc) )
				sOrgValue = ".";

			pClientSrc->m_Targ_Text = ppArgs[0];	// The attribute we want to edit.
			int iMaxLength = (iArgQty > 1) ? ATOI(ppArgs[1]) : 1;

			CGString sPrompt;
			sPrompt.Format("%s (# = default)", static_cast<LPCTSTR>(ppArgs[0]));
			pClientSrc->addGumpInpVal(true, INPVAL_Text, iMaxLength, sPrompt, sOrgValue, this);
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

			TCHAR *ppArgs[5];
			NCHAR ncBuffer[MAX_TALK_BUFFER];

			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 5 )
				break;

			CvtSystemToNUNICODE(ncBuffer, COUNTOF(ncBuffer), ppArgs[4], -1);
			pClientSrc->addBarkUNICODE(ncBuffer, this, static_cast<HUE_TYPE>(ppArgs[0][0] ? Exp_GetVal(ppArgs[0]) : HUE_TEXT_DEF), static_cast<TALKMODE_TYPE>(ppArgs[1][0] ? Exp_GetVal(ppArgs[1]) : TALKMODE_SAY), static_cast<FONT_TYPE>(ppArgs[2][0] ? Exp_GetVal(ppArgs[2]) : FONT_NORMAL), CLanguageID(ppArgs[3]));
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

				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
				{
					pChar->Update(true, pChar->m_pClient);
					if ( pChar->m_pClient )
						pChar->m_pClient->addPlayerUpdate();
					break;
				}
				CItem *pItem = dynamic_cast<CItem *>(this);
				if ( pItem )
				{
					pItem->Update();
					break;
				}
			}
			break;
		}
		case OV_MOVENEAR:
		{
			EXC_SET("MOVENEAR");
			CObjBase *pObjNear;
			INT64 piCmd[2];

			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			if ( iArgQty <= 0 )
				return false;
			if ( iArgQty < 2 )
				piCmd[1] = 1;

			pObjNear = static_cast<CGrayUID>(piCmd[0]).ObjFind();
			if ( !pObjNear )
				return false;

			MoveNearObj(pObjNear, static_cast<WORD>(piCmd[1]));
			break;
		}
		case OV_NUDGEDOWN:
		{
			EXC_SET("NUDGEDOWN");
			if ( IsTopLevel() )
			{
				signed char zDiff = static_cast<signed char>(s.GetArgVal());
				SetTopZ(GetTopZ() - (zDiff ? zDiff : 1));
				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
				{
					pChar->Update(true, pChar->m_pClient);
					if ( pChar->m_pClient )
						pChar->m_pClient->addPlayerUpdate();
					break;
				}
				CItem *pItem = dynamic_cast<CItem *>(this);
				if ( pItem )
				{
					pItem->Update();
					break;
				}
			}
			break;
		}
		case OV_NUDGEUP:
		{
			EXC_SET("NUDGEUP");
			if ( IsTopLevel() )
			{
				signed char zDiff = static_cast<signed char>(s.GetArgVal());
				SetTopZ(GetTopZ() + (zDiff ? zDiff : 1));
				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
				{
					pChar->Update(true, pChar->m_pClient);
					if ( pChar->m_pClient )
						pChar->m_pClient->addPlayerUpdate();
					break;
				}
				CItem *pItem = dynamic_cast<CItem *>(this);
				if ( pItem )
				{
					pItem->Update();
					break;
				}
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

			TCHAR *ppArgs[2];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty == 0 )
				break;

			pClientSrc->addPromptConsoleFunction(ppArgs[0], ppArgs[1], (index == OV_PROMPTCONSOLEU));
			break;
		}
		case OV_INFO:
			EXC_SET("INFO");
			if ( !pClientSrc )
				return false;
			pClientSrc->addGumpDialogProps(this);
			break;
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

			bool fSendFull = (iArgQty >= 1) ? (piCmd[0] != 0) : false;
			bool fUseCache = (iArgQty >= 2) ? (piCmd[1] != 0) : false;
			ResendTooltip(fSendFull, fUseCache);
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
		case OV_SAYUA:		// this can have full args. SAYUA Color, Mode, Font, Lang, Text
		{
			EXC_SET("SAYUA");
			TCHAR *ppArgs[5];
			size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 5 )
				break;

			SpeakUTF8(ppArgs[4], static_cast<HUE_TYPE>(ppArgs[0][0] ? Exp_GetVal(ppArgs[0]) : HUE_TEXT_DEF), static_cast<TALKMODE_TYPE>(ppArgs[1][0] ? Exp_GetVal(ppArgs[1]) : TALKMODE_SAY), static_cast<FONT_TYPE>(ppArgs[2][0] ? Exp_GetVal(ppArgs[2]) : FONT_NORMAL), CLanguageID(ppArgs[3]));
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
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
			CItem *pItemSrc = NULL;
			switch ( iArgQty )
			{
				case 4:
					pItemSrc = static_cast<CGrayUID>(piCmd[3]).ItemFind();
				case 3:
					pCharSrc = (piCmd[2] == -1) ? dynamic_cast<CChar *>(this) : static_cast<CGrayUID>(piCmd[2]).CharFind();
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
			bool fAllowGround = false;
			bool fCheckCrime = false;
			bool fFunction = false;
			bool fMulti = false;

			TCHAR chLow = static_cast<TCHAR>(tolower(*pszKey));
			while ( (chLow >= 'a') && (chLow <= 'z') )
			{
				if ( chLow == 'f' )
					fFunction = true;
				else if ( chLow == 'g' )
					fAllowGround = true;
				else if ( chLow == 'w' )
					fCheckCrime = true;
				else if ( chLow == 'm' )
					fMulti = true;
				chLow = static_cast<TCHAR>(tolower(*(++pszKey)));
			}

			pClientSrc->m_Targ_UID = GetUID();
			pClientSrc->m_tmUseItem.m_pParent = GetParent();	// cheat verify

			if ( fFunction )
			{
				if ( fMulti )
				{
					if ( IsStrEmpty(s.GetArgStr()) )
						break;
					char *ppArg[3];
					Str_ParseCmds(s.GetArgStr(), ppArg, COUNTOF(ppArg), ",");
					if ( !IsStrNumeric(ppArg[1]) )
						DEBUG_ERR(("Invalid argument in Target Multi\n"));
					ITEMID_TYPE itemid = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArg[1]));
					HUE_TYPE color = static_cast<HUE_TYPE>(Exp_GetVal(ppArg[2]));
					pClientSrc->addTargetFunctionMulti(ppArg[0], itemid, color, fAllowGround);
				}
				else
					pClientSrc->addTargetFunction(s.GetArgStr(), fAllowGround, fCheckCrime);
			}
			else
			{
				if ( fMulti )
				{
					char *ppArg[2];
					Str_ParseCmds(s.GetArgStr(), ppArg, COUNTOF(ppArg), ",");
					if ( !IsStrNumeric(ppArg[0]) )
						DEBUG_ERR(("Invalid argument in Target Multi\n"));
					ITEMID_TYPE itemid = static_cast<ITEMID_TYPE>(Exp_GetVal(ppArg[0]));
					HUE_TYPE color = static_cast<HUE_TYPE>(Exp_GetVal(ppArg[1]));
					pClientSrc->addTargetItems(CLIMODE_TARG_USE_ITEM, itemid, color, fAllowGround);
				}
				else
					pClientSrc->addTarget(CLIMODE_TARG_USE_ITEM, s.GetArgStr(), fAllowGround, fCheckCrime);
			}
			break;
		}
		case OV_TIMERF:
		{
			EXC_SET("TIMERF");
			if ( !strnicmp(s.GetArgStr(), "CLEAR", 5) )
			{
				g_World.m_TimedFunctions.Erase(GetUID());
				break;
			}
			else if ( !strnicmp(s.GetArgStr(), "STOP", 4) )
			{
				g_World.m_TimedFunctions.Stop(GetUID(), s.GetArgStr() + 5);
				break;
			}

			TCHAR *pszFuncName = s.GetArgRaw();
			int iTimer = Exp_GetVal(pszFuncName);
			if ( iTimer < 0 )
			{
				g_Log.EventError("%s: invalid timer '%d'\n", sm_szVerbKeys[index], iTimer);
				return true;
			}
			SKIP_ARGSEP(pszFuncName);
			if ( !pszFuncName )
			{
				g_Log.EventError("%s: args can't be empty\n", sm_szVerbKeys[index]);
				return true;
			}
			else if ( strlen(pszFuncName) > 1024 )
			{
				g_Log.EventError("%s: args exceeded max length allowed (%" FMTSIZE_T "/%d)\n", sm_szVerbKeys[index], strlen(pszFuncName), 1024);
				return true;
			}
			g_World.m_TimedFunctions.Add(GetUID(), iTimer, pszFuncName);
			break;
		}
		case OV_TRIGGER:
		{
			if ( s.HasArgs() )
			{
				TRIGRET_TYPE tr;
				CallPersonalTrigger(s.GetArgRaw(), pSrc, tr, false);
			}
			break;
		}
		case OV_DIALOG:
		case OV_SDIALOG:
		{
			EXC_SET("DIALOG or SDIALOG");
			if ( !pClientSrc )
				return false;

			TCHAR *ppArgs[3];		// maximum parameters in one line
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 1 )
				return false;

			if ( index == OV_SDIALOG )
			{
				DWORD context = static_cast<DWORD>(g_Cfg.ResourceGetIDType(RES_DIALOG, ppArgs[0]));
				if ( pClientSrc->m_NetState->isClientKR() )
					context = g_Cfg.GetKRDialog(context);
				context &= 0xFFFFFF;

				CClient::OpenedGumpsMap_t::iterator itGumpFound = pClientSrc->m_mapOpenedGumps.find(static_cast<int>(context));
				if ( pCharSrc && (itGumpFound != pClientSrc->m_mapOpenedGumps.end()) && ((*itGumpFound).second > 0) )
					break;
			}
			pClientSrc->Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, ppArgs[0]), (iArgQty > 1) ? Exp_GetVal(ppArgs[1]) : 0, this, ppArgs[2]);
			break;
		}
		case OV_DIALOGCLOSE:
		{
			EXC_SET("DIALOGCLOSE");
			if ( !pClientSrc )
				return false;

			TCHAR *ppArgs[2];		// maximum parameters in one line
			size_t iArgQty = Str_ParseCmds(s.GetArgStr(), ppArgs, COUNTOF(ppArgs));
			if ( iArgQty < 1 )
				return false;

			DWORD context = static_cast<DWORD>(g_Cfg.ResourceGetIDType(RES_DIALOG, ppArgs[0]));
			if ( pClientSrc->m_NetState->isClientKR() )
				context = g_Cfg.GetKRDialog(context);

			pClientSrc->Dialog_Close(this, context, (iArgQty > 1) ? Exp_GetVal(ppArgs[1]) : 0);
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
				DEBUG_ERR(("Can't try %s object %s (0%" FMTDWORDH ")\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
				return false;
			}
			return true;
		}
		case OV_TRYSRC:
		case OV_TRYSRV:
		{
			EXC_SET("TRYSRC or TRYSRV");
			CGrayUID uidNewSrc;
			CTextConsole *pNewSrc = NULL;

			if ( index == OV_TRYSRC )
				uidNewSrc = s.GetArgVal();

			LPCTSTR pszVerb = s.GetArgStr();

			if ( index == OV_TRYSRC )
			{
				if ( uidNewSrc.IsValidUID() )
					pNewSrc = uidNewSrc.CharFind();
			}
			else
				pNewSrc = &g_Serv;

			if ( !pNewSrc )
			{
				if ( index == OV_TRYSRC )
					DEBUG_ERR(("Can't trysrc %s object %s (0%" FMTDWORDH "): invalid src UID=0%" FMTDWORDH "\n", pszVerb, GetName(), static_cast<DWORD>(GetUID()), static_cast<DWORD>(uidNewSrc)));
				else
					DEBUG_ERR(("Can't trysrv %s object %s (0%" FMTDWORDH ")\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
				return false;
			}

			CScript script(pszVerb);
			if ( !r_Verb(script, pNewSrc) )
			{
				if ( index == OV_TRYSRC )
					DEBUG_ERR(("Can't trysrc %s object %s (0%" FMTDWORDH ") with src %s (0%" FMTDWORDH ")\n", pszVerb, GetName(), static_cast<DWORD>(GetUID()), pNewSrc->GetName(), static_cast<DWORD>(uidNewSrc)));
				else
					DEBUG_ERR(("Can't trysrv %s object %s (0%" FMTDWORDH ")\n", pszVerb, GetName(), static_cast<DWORD>(GetUID())));
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
		case OV_UPDATEX:
			EXC_SET("UPDATEX");
			RemoveFromView();
			// no break here, UPDATEX just remove the object from view before update it
		case OV_UPDATE:
		{
			EXC_SET("UPDATE");
			CClient *pClientExclude = NULL;
			CChar *pChar = dynamic_cast<CChar *>(this);
			if ( pChar && pChar->m_pClient )
			{
				pChar->m_pClient->addReSync();
				pClientExclude = pChar->m_pClient;	// don't update this client again
			}
			Update(true, pClientExclude);
			break;
		}
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
			// fall through
		case OV_Z:
		{
			EXC_SET("FIX or Z");
			if ( IsItemEquipped() )
				return false;

			CChar *pChar = dynamic_cast<CChar *>(this);
			if ( pChar )
			{
				SetTopZ(s.HasArgs() ? static_cast<signed char>(s.GetArgVal()) : pChar->GetFixZ(GetTopPoint()));
				pChar->Update(true, pChar->m_pClient);
				if ( pChar->m_pClient )
					pChar->m_pClient->addPlayerUpdate();
				break;
			}
			CItem *pItem = dynamic_cast<CItem *>(this);
			if ( pItem )
			{
				SetTopZ(s.HasArgs() ? static_cast<signed char>(s.GetArgVal()) : pItem->GetFixZ(GetTopPoint()));
				pItem->Update();
				break;
			}
			return false;
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
	CItem *pItem = fHardcoded ? dynamic_cast<CItem *>(this) : NULL;
	CChar *pChar = NULL;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		if ( pClientExclude == pClient )
			continue;
		pChar = pClient->GetChar();
		if ( !pChar )
			continue;
		if ( pChar->GetTopDist(pObjTop) > pChar->GetSight() )	// client does not support removing of items which are farther (will be removed from the radar on the next step, cause the server won't resend it)
			continue;
		if ( pItem && pItem->IsItemEquipped() )
		{
			if ( (pItem->GetEquipLayer() > LAYER_HORSE) && (pItem->GetEquipLayer() != LAYER_BANKBOX) && (pItem->GetEquipLayer() != LAYER_DRAGGING) )
				continue;
		}

		if ( GetEquipLayer() == LAYER_BANKBOX )
			pClient->closeUIWindow(PacketCloseUIWindow::Container, this);

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
		if ( pChar->GetTopDist(pObjTop) > pChar->GetSight() )	// client does not support removing of items which are farther (will be removed from the radar on the next step, cause the server won't resend it)
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
			pClient->closeUIWindow(PacketCloseUIWindow::Container, this);

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

	if ( m_PropertyHash != dwHash )
	{
		// Increment revision number when property hash get changed
		m_PropertyHash = dwHash;
		++m_PropertyRevision;
	}

	return m_PropertyRevision;
}

void CObjBase::UpdatePropertyFlag()
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyFlag");
	if ( (!(g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B)) || g_Serv.IsLoading() )
		return;

	m_fStatusUpdate |= SU_UPDATE_TOOLTIP;

	// Items equipped, inside containers or with timer expired doesn't receive ticks and need to be added to a list of items to be processed separately
	if ( (!IsTopLevel() || IsTimerExpired()) && !g_World.m_ObjStatusUpdates.ContainsPtr(this) )
		g_World.m_ObjStatusUpdates.Add(this);
}

void CObjBase::OnTickStatusUpdate()
{
	ADDTOCALLSTACK("CObjBase::OnTickStatusUpdate");
	// Process m_fStatusUpdate flags

	if ( m_fStatusUpdate & SU_UPDATE_TOOLTIP )
		ResendTooltip();
}

void CObjBase::ResendTooltip(bool fSendFull, bool fUseCache)
{
	ADDTOCALLSTACK("CObjBase::ResendTooltip");
	// Send tooltip packet to all nearby clients

	m_fStatusUpdate &= ~SU_UPDATE_TOOLTIP;
	if ( IsDisconnected() || g_Serv.IsLoading() )
		return;	// not in the world.

	if ( !fUseCache )
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

		pClient->addAOSTooltip(this, fSendFull);
	}
}

void CObjBase::DeletePrepare()
{
	ADDTOCALLSTACK("CObjBase::DeletePrepare");
	// Prepare to delete.
	RemoveFromView();
	RemoveSelf();	// Must remove early or else virtuals will fail.
}

void CObjBase::Delete(bool fForce)
{
	ADDTOCALLSTACK("CObjBase::Delete");
	UNREFERENCED_PARAMETER(fForce);		// CObjBase doesnt use it, but CItem and CChar does use it, do not remove

	if ( m_uidSpawnItem.ItemFind() )
		static_cast<CItemSpawn *>(m_uidSpawnItem.ItemFind())->DelObj(GetUID());

	DeletePrepare();
	g_World.m_TimedFunctions.Erase(GetUID());
	g_World.m_ObjDelete.InsertHead(this);
}

bool CObjBase::IsTriggerActive(LPCTSTR pszTrig)
{
	return (m_RunningTrigger == pszTrig);
}

LPCTSTR CObjBase::GetTriggerActive()
{
	return m_RunningTrigger ? m_RunningTrigger : "none";
}

void CObjBase::SetTriggerActive(LPCTSTR pszTrig)
{
	if ( pszTrig )
	{
		char *chText = Str_GetTemp();
		sprintf(chText, "Trigger: %s", pszTrig);
		ADDTOCALLSTACK(chText);
	}
	m_RunningTrigger = pszTrig ? pszTrig : NULL;
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

inline bool CObjBase::CallPersonalTrigger(TCHAR *pszArgs, CTextConsole *pSrc, TRIGRET_TYPE &tr, bool fFull)
{
	ADDTOCALLSTACK("CObjBase::CallPersonalTrigger");
	UNREFERENCED_PARAMETER(fFull);
	TCHAR *ppCmdTrigger[3];
	size_t iArgQty = Str_ParseCmds(pszArgs, ppCmdTrigger, COUNTOF(ppCmdTrigger), ",");
	if ( iArgQty > 0 )
	{
		LPCTSTR pszTrigger = ppCmdTrigger[0];
		CScriptTriggerArgs csTriggerArgs;

		if ( iArgQty == 3 )
		{
			int iTriggerArgType = ATOI(ppCmdTrigger[1]);
			if ( iTriggerArgType == 1 )		// 3 ARGNs
			{
				INT64 Arg_piCmd[3];
				iArgQty = Str_ParseCmds(ppCmdTrigger[2], Arg_piCmd, COUNTOF(Arg_piCmd), ",");

				if ( iArgQty == 3 )
					csTriggerArgs.m_iN3 = Arg_piCmd[2];
				if ( iArgQty >= 2 )
					csTriggerArgs.m_iN2 = Arg_piCmd[1];
				if ( iArgQty >= 1 )
					csTriggerArgs.m_iN1 = Arg_piCmd[0];
			}
			else if ( iTriggerArgType == 2 )	// ARGS
			{
				csTriggerArgs.m_s1 = ppCmdTrigger[2];
				csTriggerArgs.m_s1_raw = ppCmdTrigger[2];
			}
			else if ( iTriggerArgType == 3 )	// ARGO
			{
				CObjBase *pTriggerArgObj = static_cast<CGrayUID>(Exp_GetVal(ppCmdTrigger[2])).ObjFind();
				if ( pTriggerArgObj )
					csTriggerArgs.m_pO1 = pTriggerArgObj;
			}
			else if ( iTriggerArgType == 4 )	// FULL TRIGGER
			{
				TCHAR *ppArgs[5];
				iArgQty = Str_ParseCmds(ppCmdTrigger[2], ppArgs, COUNTOF(ppArgs), ",");

				// ARGS
				if ( iArgQty == 5 )
				{
					csTriggerArgs.m_s1 = ppArgs[4];
					csTriggerArgs.m_s1_raw = ppArgs[4];
				}
				// ARGNs
				if ( iArgQty >= 4 )
					csTriggerArgs.m_iN3 = Exp_GetVal(ppArgs[3]);
				if ( iArgQty >= 3 )
					csTriggerArgs.m_iN2 = Exp_GetVal(ppArgs[2]);
				if ( iArgQty >= 2 )
					csTriggerArgs.m_iN1 = Exp_GetVal(ppArgs[1]);
				// ARGO
				if ( iArgQty >= 1 )
				{
					CObjBase *pTriggerArgObj = static_cast<CGrayUID>(Exp_GetVal(ppArgs[0])).ObjFind();
					if ( pTriggerArgObj )
						csTriggerArgs.m_pO1 = pTriggerArgObj;
				}
			}
		}

		tr = OnTrigger(pszTrigger, pSrc, &csTriggerArgs);
		return true;
	}
	return false;
}
