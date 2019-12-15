// An item is targetted.
#include "graysvr.h"	// predef header.
#include "../network/send.h"

////////////////////////////////////////////////////////
// Targetted GM functions.

void CClient::OnTarg_Obj_Set(CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Set");
	// CLIMODE_TARG_OBJ_SET
	// Targeted a command at an CObjBase object
	// ARGS:
	//  m_Targ_Text = new command and value.

	if ( !pObj )
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_UNEXPECTED));
		return;
	}

	// Parse the command.
	TCHAR *pszLogMsg = Str_GetTemp();
	sprintf(pszLogMsg, "%lx:'%s' commands UID=0%" FMTDWORDH " (%s) to '%s'", m_NetState->id(), GetName(), static_cast<DWORD>(pObj->GetUID()), pObj->GetName(), static_cast<LPCTSTR>(m_Targ_Text));

	// Check priv level for the new verb.
	if ( !g_Cfg.CanUsePrivVerb(pObj, m_Targ_Text, this) )
	{
		SysMessageDefault(DEFMSG_MSG_ACC_PRIV);
		g_Log.Event(LOGM_GM_CMDS, "%s=0\n", pszLogMsg);
		return;
	}

	CScript sCmd(m_Targ_Text);
	if ( sCmd.IsKey("COLOR") && !sCmd.HasArgs() )	// ".xCOLOR" command without arguments should open dye gump
	{
		addDyeOption(pObj);
		return;
	}

	bool fRet = pObj->r_Verb(sCmd, this);
	if ( !fRet )
		SysMessageDefault(DEFMSG_MSG_ERR_INVSET);
	if ( GetPrivLevel() >= g_Cfg.m_iCommandLog )
		g_Log.Event(LOGM_GM_CMDS, "%s=%d\n", pszLogMsg, fRet);
}

void CClient::OnTarg_Obj_Function(CObjBase *pObj, const CPointMap &pt, ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Function");
	m_Targ_p = pt;
	LPCTSTR	pSpace = strchr(m_Targ_Text, ' ');
	if ( !pSpace )
		pSpace = strchr(m_Targ_Text, '\t');
	if ( pSpace )
		GETNONWHITESPACE(pSpace);

	CScriptTriggerArgs Args(pSpace ? pSpace : "");
	Args.m_VarsLocal.SetNum("ID", id, true);
	Args.m_pO1 = pObj;
	CGString sVal;
	m_pChar->r_Call(static_cast<LPCTSTR>(m_Targ_Text), this, &Args, &sVal);
}

void CClient::OnTarg_Obj_Info(CObjBase *pObj, const CPointMap &pt, ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Info");
	// CLIMODE_TARG_OBJ_INFO "INFO"

	if ( pObj )
	{
		SetTargMode();
		addGumpDialogProps(pObj);
		return;
	}

	TCHAR *pszTemp = Str_GetTemp();
	size_t len = 0;
	if ( id )
	{
		len = sprintf(pszTemp, "[Static z=%hhd, 0%x=", pt.m_z, id);

		// static items have no uid's but we can still use them.
		CItemBase *pItemDef = CItemBase::FindItemBase(id);
		if ( pItemDef )
			len += sprintf(pszTemp + len, "%s->%s], ", pItemDef->GetResourceName(), g_Cfg.ResourceGetName(RESOURCE_ID(RES_TYPEDEF, pItemDef->GetType())));
		else
			len += sprintf(pszTemp + len, "NON scripted], ");
	}
	else
	{
		// tile info for location.
		len = strcpylen(pszTemp, "[No static tile], ");
	}

	const CUOMapMeter *pMeter = g_World.GetMapMeter(pt);
	if ( pMeter )
		len += sprintf(pszTemp + len, "TERRAIN=0%hx TYPE=%s", pMeter->m_wTerrainIndex, g_World.GetTerrainItemTypeDef(pMeter->m_wTerrainIndex)->GetResourceName());

	SysMessage(pszTemp);
}

bool CClient::Cmd_Control(CChar *pChar2)
{
	ADDTOCALLSTACK("CClient::Cmd_Control");
	// I wish to control pChar2
	// Leave my own body behind.

	if ( !pChar2 || pChar2->IsDisconnected() || (GetPrivLevel() < pChar2->GetPrivLevel()) )
		return false;

	ASSERT(m_pChar);
	CChar *pChar1 = m_pChar;

	// Put my newbie equipped items on it.
	CItem *pItemNext = NULL;
	for ( CItem *pItem = pChar1->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !CItemBase::IsVisibleLayer(pItem->GetEquipLayer()) || !pItem->IsAttr(ATTR_MOVE_NEVER) )
			continue;
		switch ( pItem->GetEquipLayer() )
		{
			case LAYER_HAIR:
			case LAYER_FACE:
			case LAYER_BEARD:
			case LAYER_PACK:
				continue;
			default:
				pChar2->LayerAdd(pItem);	// add content
		}
	}

	// Put my GM pack stuff in it's inventory.
	CItemContainer *pPack1 = pChar1->GetContainer(LAYER_PACK);
	CItemContainer *pPack2 = pChar2->GetContainerCreate(LAYER_PACK);
	if ( pPack1 && pPack2 )
	{
		for ( CItem *pItem = pPack1->GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( !pItem->IsAttr(ATTR_MOVE_NEVER) )	// keep newbie stuff.
				continue;
			pPack2->ContentAdd(pItem);	// add content
		}
	}

	pChar1->ClientDetach();
	m_pChar = NULL;
	CClient *pClient2 = pChar2->m_pClient;
	if ( pClient2 )		// controlled char is a player/client
	{
		pChar2->ClientDetach();
		pClient2->m_pChar = NULL;
	}

	CCharPlayer *pPlayer1 = pChar1->m_pPlayer;
	if ( pPlayer1 )
		pPlayer1->m_pAccount->DetachChar(pChar1);

	CCharPlayer *pPlayer2 = pChar2->m_pPlayer;
	if ( pPlayer2 )
		pPlayer2->m_pAccount->DetachChar(pChar2);

	pChar1->m_pPlayer = pPlayer2;
	pChar2->m_pPlayer = pPlayer1;

	ASSERT(!pChar1->m_pNPC);
	pChar1->m_pNPC = pChar2->m_pNPC;	// turn my old body into a NPC (if it was)
	pChar2->m_pNPC = NULL;

	if ( pPlayer1 )
	{
		pPlayer1->m_pAccount->AttachChar(pChar2);
		pPlayer1->m_pAccount->m_uidLastChar = pChar2->GetUID();
	}
	if ( pPlayer2 )
	{
		pPlayer2->m_pAccount->AttachChar(pChar1);
		pPlayer2->m_pAccount->m_uidLastChar = pChar1->GetUID();
	}
	if ( pClient2 )
		pClient2->addPlayerStart(pChar1);
	else
	{
		switch ( pChar1->GetID() )
		{
			case CREID_EQUIP_GM_ROBE:
			case CREID_GHOSTMAN:
			case CREID_GHOSTWOMAN:
			case CREID_ELFGHOSTMAN:
			case CREID_ELFGHOSTWOMAN:
			case CREID_GARGGHOSTMAN:
			case CREID_GARGGHOSTWOMAN:
				pChar1->Delete();	// delete my ghost
			default:
				pChar1->SetTimeout(1);	// must kick start the NPC
		}
	}
	addPlayerStart(pChar2);
	return true;
}

void CClient::OnTarg_UnExtract(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_UnExtract");
	UNREFERENCED_PARAMETER(pObj);
	// CLIMODE_TARG_UNEXTRACT
	// ??? Get rid of this in favor of a more .SCP file type approach.
	// result of the MULTI command.
	// Break a multi out of the multi.txt files and turn it into items.

	if ( !pt.GetRegion(REGION_TYPE_AREA) )
		return;

	CScript s;	// It is not really a valid script type file.
	if ( !g_Cfg.OpenResourceFind(s, m_Targ_Text) )
		return;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%d template id", m_tmTile.m_id);
	if ( !s.FindTextHeader(pszTemp) )
		return;
	if ( !s.ReadKey() )
		return;		// throw this one away
	if ( !s.ReadKeyParse() )
		return;		// this has the item count

	int iItemCount = ATOI(s.GetKey());	// item count
	while ( iItemCount > 0 )
	{
		if ( !s.ReadKeyParse() )
			return;		// this has the item count

		INT64 piCmd[4];		// Maximum parameters in one line
		Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));

		CItem *pItem = CItem::CreateTemplate(static_cast<ITEMID_TYPE>(ATOI(s.GetKey())), NULL, m_pChar);
		if ( !pItem )
			return;

		CPointMap ptOffset(static_cast<signed short>(piCmd[0]), static_cast<signed short>(piCmd[1]), static_cast<signed char>(piCmd[2]));
		ptOffset += pt;
		ptOffset.m_map = pt.m_map;
		pItem->MoveToUpdate(ptOffset);
		--iItemCount;
	}
}

void CClient::OnTarg_Char_Add(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Char_Add");
	// CLIMODE_TARG_ADDCHAR
	// m_tmAdd.m_id
	// m_tmAdd.m_amount
	ASSERT(m_pChar);

	if ( !pt.GetRegion(REGION_TYPE_AREA) || (pObj && pObj->IsItemInContainer()) )
		return;

	for ( WORD i = 0; i < m_tmAdd.m_amount; ++i )
	{
		CChar *pChar = CChar::CreateBasic(static_cast<CREID_TYPE>(m_tmAdd.m_id));
		if ( !pChar )
			return;

		pChar->NPC_LoadScript(true);
		pChar->MoveToChar(pt);
		pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
		pChar->Update();
		pChar->UpdateAnimate(ANIM_CAST_DIR);
		pChar->SoundChar(CRESND_GETHIT);
		m_pChar->m_Act_Targ = pChar->GetUID();		// for last target stuff (trigger stuff)
	}
}

void CClient::OnTarg_Item_Add(CObjBase *pObj, CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Add");
	// CLIMODE_TARG_ADDITEM
	// m_tmAdd.m_id
	// m_tmAdd.m_amount
	ASSERT(m_pChar);

	if ( !pt.GetRegion(REGION_TYPE_AREA) || (pObj && pObj->IsItemInContainer()) )
		return;

	CItem *pItem = CItem::CreateTemplate(static_cast<ITEMID_TYPE>(m_tmAdd.m_id), NULL, m_pChar);
	if ( !pItem )
		return;

	if ( pItem->IsTypeMulti() )
	{
		OnTarg_Use_Multi(pItem->Item_GetDef(), pt, pItem->m_Attr, pItem->GetHue());
		pItem->Delete();
		return;
	}

	pItem->SetAmount(m_tmAdd.m_amount);
	pItem->MoveToCheck(pt, m_pChar);
	m_pChar->m_Act_Targ = pItem->GetUID();		// for last target stuff (trigger stuff) and to make AxisII able to initialize placed spawn items.
}

void CClient::OnTarg_Item_Link(CObjBase *pObj2)
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Link");
	// CLIMODE_LINK

	if ( !pObj2 )
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_DYNAMIC));
		return;
	}

	CItem *pItem2 = dynamic_cast<CItem *>(pObj2);
	CItem *pItem1 = m_Targ_UID.ItemFind();
	if ( !pItem1 )
	{
		if ( !pItem2 )
		{
			m_Targ_UID.InitUID();
			addTarget(CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_LINK));
		}
		else
		{
			m_Targ_UID = pObj2->GetUID();
			addTarget(CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_LINK2));
		}
		return;
	}

	if ( pItem2 == pItem1 )
	{
		// Break any existing links.
		SysMessageDefault(DEFMSG_MSG_TARG_LINK_SAME);
		return;
	}

	if ( pItem2 && (pItem1->IsType(IT_KEY) || pItem2->IsType(IT_KEY)) )
	{
		// Linking a key to something is a special case.
		if ( !pItem1->IsType(IT_KEY) )
		{
			CItem *pTmp = pItem1;
			pItem1 = pItem2;
			pItem2 = pTmp;
		}
		// pItem1 = the IT_KEY
		if ( pItem2->m_itContainer.m_lockUID )
			pItem1->m_itKey.m_lockUID = pItem2->m_itContainer.m_lockUID;
		else if ( pItem1->m_itKey.m_lockUID )
			pItem2->m_itContainer.m_lockUID = pItem1->m_itKey.m_lockUID;
		else
			pItem1->m_itKey.m_lockUID = pItem2->m_itContainer.m_lockUID = pItem2->GetUID();
	}
	else
	{
		pItem1->m_uidLink = pObj2->GetUID();
		if ( pItem2 && !pItem2->m_uidLink.IsValidUID() )
			pItem2->m_uidLink = pItem1->GetUID();
	}

	SysMessageDefault(DEFMSG_MSG_TARG_LINKS);
}

int CClient::Cmd_Extract(CScript *pScript, CRectMap &rect, signed char &zLowest)
{
	ADDTOCALLSTACK("CClient::Cmd_Extract");
	// RETURN: Number of statics here.
	CPointMap ptCtr = rect.GetCenter();

	int iCount = 0;
	for ( int mx = rect.m_left; mx <= rect.m_right; ++mx )
	{
		for ( int my = rect.m_top; my <= rect.m_bottom; ++my )
		{
			CPointMap ptCur(static_cast<signed short>(mx), static_cast<signed short>(my), 0, static_cast<BYTE>(rect.m_map));
			const CGrayMapBlock *pBlock = g_World.GetMapBlock(ptCur);
			if ( !pBlock )
				continue;
			size_t iQty = pBlock->m_Statics.GetStaticQty();
			if ( iQty <= 0 )	// no static items here
				continue;

			int x2 = pBlock->GetOffsetX(mx);
			int y2 = pBlock->GetOffsetY(my);
			for ( size_t i = 0; i < iQty; ++i )
			{
				if ( !pBlock->m_Statics.IsStaticPoint(i, x2, y2) )
					continue;
				const CUOStaticItemRec *pStatic = pBlock->m_Statics.GetStatic(i);
				ASSERT(pStatic);
				++iCount;
				if ( pScript )
				{
					// This static is at the coordinates in question
					pScript->Printf("%u %d %d %hhd 0\n", pStatic->GetDispID(), mx - ptCtr.m_x, my - ptCtr.m_y, pStatic->m_z - zLowest);
				}
				else
				{
					if ( pStatic->m_z < zLowest )
						zLowest = pStatic->m_z;
				}
			}
		}
	}

	// Extract Multi's ???

	// Extract dynamics as well
	int rx = 1 + abs(rect.m_right - rect.m_left) / 2;
	int ry = 1 + abs(rect.m_bottom - rect.m_top) / 2;

	CWorldSearch AreaItem(ptCtr, maximum(rx, ry));
	for (;;)
	{
		CItem *pItem = AreaItem.GetItem();
		if ( !pItem )
			break;
		if ( !rect.IsInside2d(pItem->GetTopPoint()) )
			continue;

		CPointMap pt = pItem->GetTopPoint();
		++iCount;
		if ( pScript )
		{
			// This static is at the coordinates in question.
			pScript->Printf("%u %hd %hd %hhd 0\n", pItem->GetDispID(), pt.m_x - ptCtr.m_x, pt.m_y - ptCtr.m_y, pt.m_z - zLowest);
		}
		else
		{
			if ( pt.m_z < zLowest )
				zLowest = pt.m_z;
		}
	}
	return iCount;
}

void CClient::OnTarg_Tile(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Tile");
	// CLIMODE_TARG_TILE

	ASSERT(m_pChar);

	if ( !pt.IsValidPoint() || (pObj && !pObj->IsTopLevel()) )
		return;

	if ( !m_tmTile.m_ptFirst.IsValidPoint() )
	{
		m_tmTile.m_ptFirst = pt;
		addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_PC), true);
		return;
	}

	if ( (pt == m_tmTile.m_ptFirst) && (m_tmTile.m_Code != CV_EXTRACT) )	// extract can work with one square
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_TILE_SAME_POINT));
		addTarget(CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_PC), true);
		return;
	}

	CRectMap rect;
	rect.SetRect(m_tmTile.m_ptFirst.m_x, m_tmTile.m_ptFirst.m_y, pt.m_x, pt.m_y, pt.m_map);
	CPointMap ptCtr = rect.GetCenter();
	ptCtr.m_map = pt.m_map;

	int rx = 1 + abs(rect.m_right - rect.m_left) / 2;
	int ry = 1 + abs(rect.m_bottom - rect.m_top) / 2;
	int iRadius = maximum(rx, ry);
	int iCount = 0;

	switch ( m_tmTile.m_Code )
	{
		case CV_EXTRACT:
		{
			// "EXTRACT" all map statics in the region.
			// First find the lowest Z to use as a base and count the statics
			signed char zLowest = UO_SIZE_Z;
			iCount = Cmd_Extract(NULL, rect, zLowest);
			if ( iCount )
			{
				CScript s;
				if ( !s.Open(m_Targ_Text, OF_WRITE|OF_TEXT|OF_DEFAULTMODE) )
					return;

				// Write a header for this multi in XXX format (I have no idea what most of this means)
				s.Printf("6 version\n");
				s.Printf("%d template id\n", m_tmTile.m_id);
				s.Printf("-1 item version\n");
				s.Printf("%d num components\n", iCount);
				Cmd_Extract(&s, rect, zLowest);
			}
			SysMessagef("%d Statics Extracted to '%s', id=%d", iCount, static_cast<LPCTSTR>(m_Targ_Text), m_tmTile.m_id);
			break;
		}
		case CV_NUDGE:
		{
			TCHAR szTmp[512];
			strcpylen(szTmp, m_Targ_Text, COUNTOF(szTmp));

			INT64 piArgs[3];		// maximum parameters in one line
			Str_ParseCmds(szTmp, piArgs, COUNTOF(piArgs));

			CPointMap ptNudge(static_cast<signed short>(piArgs[0]), static_cast<signed short>(piArgs[1]), static_cast<signed char>(piArgs[2]));

			CWorldSearch AreaItem(ptCtr, iRadius);
			AreaItem.SetAllShow(IsPriv(PRIV_ALLSHOW));
			for (;;)
			{
				CItem *pItem = AreaItem.GetItem();
				if ( !pItem )
					break;
				if ( !rect.IsInside2d(pItem->GetTopPoint()) )
					continue;

				CPointMap ptMove = pItem->GetTopPoint();
				ptMove += ptNudge;
				pItem->MoveToCheck(ptMove);
				++iCount;
			}

			CWorldSearch AreaChar(ptCtr, iRadius);
			AreaChar.SetAllShow(IsPriv(PRIV_ALLSHOW));
			for (;;)
			{
				CChar *pChar = AreaChar.GetChar();
				if ( !pChar )
					break;
				if ( !rect.IsInside2d(pChar->GetTopPoint()) )
					continue;

				CPointMap ptMove = pChar->GetTopPoint();
				ptMove += ptNudge;
				pChar->MoveTo(ptMove);
				++iCount;
			}
			SysMessagef("%d %s", iCount, g_Cfg.GetDefaultMsg(DEFMSG_NUDGED_OBJECTS));
			break;
		}
		case CV_NUKE:
		{
			CWorldSearch AreaItem(ptCtr, iRadius);
			AreaItem.SetAllShow(IsPriv(PRIV_ALLSHOW));
			for (;;)
			{
				CItem *pItem = AreaItem.GetItem();
				if ( !pItem )
					break;
				if ( !rect.IsInside2d(pItem->GetTopPoint()) )
					continue;

				if ( m_Targ_Text.IsEmpty() )
					pItem->Delete();
				else
				{
					CScript script(m_Targ_Text);
					if ( !pItem->r_Verb(script, this) )
						continue;
				}
				++iCount;
			}
			SysMessagef("%d %s", iCount, g_Cfg.GetDefaultMsg(DEFMSG_NUKED_ITEMS));
			break;
		}
		case CV_NUKECHAR:
		{
			CWorldSearch AreaChar(ptCtr, iRadius);
			AreaChar.SetAllShow(IsPriv(PRIV_ALLSHOW));
			for (;;)
			{
				CChar *pChar = AreaChar.GetChar();
				if ( !pChar )
					break;
				if ( !rect.IsInside2d(pChar->GetTopPoint()) || pChar->m_pPlayer )
					continue;

				if ( m_Targ_Text.IsEmpty() )
					pChar->Delete();
				else
				{
					CScript script(m_Targ_Text);
					if ( !pChar->r_Verb(script, this) )
						continue;
				}
				++iCount;
			}
			SysMessagef("%d %s", iCount, g_Cfg.GetDefaultMsg(DEFMSG_NUKED_CHARS));
			break;
		}
		case CV_TILE:
		{
			TCHAR szTmp[256];
			strcpylen(szTmp, m_Targ_Text, COUNTOF(szTmp));

			INT64 piArgs[16];		// maximum parameters in one line
			size_t iArgQty = Str_ParseCmds(szTmp, piArgs, COUNTOF(piArgs));
			size_t iArg = 0;

			signed char z = static_cast<signed char>(piArgs[0]);	// z height is the first arg
			for ( int mx = rect.m_left; mx <= rect.m_right; ++mx )
			{
				for ( int my = rect.m_top; my <= rect.m_bottom; ++my )
				{
					if ( ++iArg >= iArgQty )
						iArg = 1;
					CItem *pItem = CItem::CreateTemplate(static_cast<ITEMID_TYPE>(RES_GET_INDEX(piArgs[iArg])), NULL, m_pChar);
					ASSERT(pItem);
					pItem->SetAttr(ATTR_MOVE_NEVER);
					CPointMap ptCur(static_cast<signed short>(mx), static_cast<signed short>(my), z, pt.m_map);
					pItem->MoveToUpdate(ptCur);
					++iCount;
				}
			}
			SysMessagef("%d %s", iCount, g_Cfg.GetDefaultMsg(DEFMSG_TILED_ITEMS));
			break;
		}
	}
}

//-----------------------------------------------------------------------
// Targetted Informational skills

int CClient::OnSkill_AnimalLore(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_AnimalLore");
	// SKILL_ANIMALLORE

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_ANIMALLORE);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 12;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_ANIMALLORE_TOOFAR);
		return -SKTRIG_QTY;
	}

	CChar *pChar = dynamic_cast<CChar *>(pObj);
	if ( !pChar || !pChar->m_pNPC || (!pChar->NPC_IsAnimal() && !pChar->NPC_IsMonster()) )
	{
		SysMessageDefault(DEFMSG_ANIMALLORE_NOTANIMAL);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	LPCTSTR pszLoyaltyRating;
	if ( pChar->IsStatFlag(STATF_Pet) )
	{
		static const LPCTSTR sm_szAnimalLore_LoyaltyRating[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_1),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_2),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_3),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_4),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_5),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_6),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_7),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_8),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_9),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_10),
			g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_11)
		};

		size_t iQty = COUNTOF(sm_szAnimalLore_LoyaltyRating);
		int iMax = pChar->Stat_GetMax(STAT_FOOD);
		if ( iMax )
		{
			int i = IMULDIV(pChar->Stat_GetVal(STAT_FOOD), iQty, iMax);
			if ( i < 0 )
				i = 0;
			else if ( static_cast<size_t>(i) >= iQty )
				i = iQty - 1;
			pszLoyaltyRating = sm_szAnimalLore_LoyaltyRating[i];
		}
		else
			pszLoyaltyRating = sm_szAnimalLore_LoyaltyRating[iQty - 1];
	}
	else
		pszLoyaltyRating = g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_WILD);

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_LOYALTY_RESULT), pChar->GetPronoun(), pszLoyaltyRating);
	addObjMessage(pszTemp, pChar);

	return m_pChar->Skill_GetBase(SKILL_ANIMALLORE);
}

int CClient::OnSkill_ItemID(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_ItemID");
	// SKILL_ITEMID

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_ITEMID);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 3;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_ITEMID_TOOFAR);
		return -SKTRIG_QTY;
	}

	CItem *pItem = dynamic_cast<CItem *>(pObj);
	if ( !pItem )
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMID_CHAR), pObj->GetName());
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	pItem->SetAttr(ATTR_IDENTIFIED);
	CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMID_VALUE), pItemVend ? static_cast<int>(pItemVend->GetVendorPrice(0) * pItem->GetAmount()) : 0);

	return m_pChar->Skill_GetBase(SKILL_ITEMID);
}

static const LPCTSTR sm_szPercentLevel[] =
{
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_0),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_10),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_20),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_30),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_40),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_50),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_60),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_70),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_80),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_90),
	g_Cfg.GetDefaultMsg(DEFMSG_NUMBER_100)
};

int CClient::OnSkill_EvalInt(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_EvalInt");
	// SKILL_EVALINT

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_EVALINT);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 8;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_REACH_FAIL);
		return -SKTRIG_QTY;
	}

	CChar *pChar = dynamic_cast<CChar *>(pObj);
	if ( !pChar )
	{
		addObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_TARG_ITEM), pObj);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	static const LPCTSTR sm_szEvalInt_IntLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_1),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_2),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_3),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_4),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_5),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_6),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_7),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_8),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_9),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_10),
		g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_11)
	};

	int iInt = pChar->Stat_GetBase(STAT_INT) / 10;
	if ( iInt < 0 )
		iInt = 0;
	else if ( static_cast<size_t>(iInt) >= COUNTOF(sm_szEvalInt_IntLevel) )
		iInt = COUNTOF(sm_szEvalInt_IntLevel) - 1;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_INT_RESULT), pChar->GetPronoun(), sm_szEvalInt_IntLevel[iInt]);
	addObjMessage(pszTemp, pChar);

	WORD wSkill = m_pChar->Skill_GetBase(SKILL_EVALINT);
	if ( wSkill >= 760 )
	{
		int iMana = ((pChar->Stat_GetVal(STAT_INT) * 100) / maximum(pChar->Stat_GetMax(STAT_INT), 1)) / 10;
		if ( iMana < 0 )
			iMana = 0;
		else if ( static_cast<size_t>(iMana) >= COUNTOF(sm_szPercentLevel) )
			iMana = COUNTOF(sm_szPercentLevel) - 1;

		sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_EVALINT_MANA_RESULT), sm_szPercentLevel[iMana]);
		addObjMessage(pszTemp, pChar);
	}

	return wSkill;
}

int CClient::OnSkill_ArmsLore(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_ArmsLore");
	// SKILL_ARMSLORE

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_ARMSLORE);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 3;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_ARMSLORE_TOOFAR);
		return -SKTRIG_QTY;
	}

	CItem *pItem = dynamic_cast<CItem *>(pObj);
	if ( !pItem || !pItem->IsTypeArmorWeapon() )
	{
		SysMessageDefault(DEFMSG_ARMSLORE_UNABLE);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_DURABILITY_RESULT), pItem->Armor_GetRepairDesc());

	if ( pItem->IsTypeWeapon() )
	{
		static const LPCTSTR sm_szArmsLore_DmgLevel[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_1),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_2),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_3),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_4),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_5),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_6),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_DAM_7),
		};

		int iDmg = (pItem->Weapon_GetAttack(false) + pItem->Weapon_GetAttack(true)) / 2;
		if ( iDmg > 30 )
			iDmg = 30;
		iDmg /= 5;

		if ( iDmg < 0 )
			iDmg = 0;
		if ( static_cast<size_t>(iDmg) >= COUNTOF(sm_szArmsLore_DmgLevel) )
			iDmg = COUNTOF(sm_szArmsLore_DmgLevel) - 1;

		LPCTSTR pszHitType;
		switch ( pItem->GetType() )
		{
			case IT_WEAPON_MACE_SMITH:
			case IT_WEAPON_MACE_SHARP:
			case IT_WEAPON_MACE_STAFF:
			case IT_WEAPON_MACE_CROOK:
			case IT_WEAPON_WHIP:
				pszHitType = g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_HIT_BASH);
				break;
			case IT_WEAPON_SWORD:
			case IT_WEAPON_MACE_PICK:
			case IT_WEAPON_AXE:
			case IT_WEAPON_THROWING:
				pszHitType = g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_HIT_SLASH);
				break;
			case IT_WEAPON_FENCE:
				pszHitType = g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_HIT_STAB);
				break;
			case IT_WEAPON_BOW:
			case IT_WEAPON_XBOW:
				pszHitType = g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_HIT_SHOT);
				break;
			default:
				pszHitType = g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_HIT);
				break;
		}

		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_WEAPON_RESULT), sm_szArmsLore_DmgLevel[iDmg], pszHitType, g_Cfg.GetDefaultMsg((pItem->Item_GetDef()->GetEquipLayer() == LAYER_HAND2) ? DEFMSG_ARMSLORE_WEAPON_RANGE_LONG : DEFMSG_ARMSLORE_WEAPON_RANGE_SHORT));

		if ( pItem->m_itWeapon.m_poison_skill )
			SysMessageDefault(DEFMSG_ARMSLORE_WEAPON_POISON);
	}
	else
	{
		static const LPCTSTR sm_szArmsLore_ArmorLevel[] =
		{
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_1),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_2),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_3),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_4),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_5),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_6),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_7),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_AR_8)
		};

		int iArmor = pItem->Armor_GetDefense();
		if ( iArmor > 35 )
			iArmor = 35;
		iArmor /= 5;

		if ( iArmor < 0 )
			iArmor = 0;
		if ( static_cast<size_t>(iArmor) >= COUNTOF(sm_szArmsLore_ArmorLevel) )
			iArmor = COUNTOF(sm_szArmsLore_ArmorLevel) - 1;

		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_ARMOR_RESULT), sm_szArmsLore_ArmorLevel[iArmor]);
	}

	return m_pChar->Skill_GetBase(SKILL_ARMSLORE);
}

int CClient::OnSkill_Anatomy(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_Anatomy");
	// SKILL_ANATOMY

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_ANATOMY);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 8;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_ANATOMY_TOOFAR);
		return -SKTRIG_QTY;
	}

	CChar *pChar = dynamic_cast<CChar *>(pObj);
	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_ANATOMY_TARG_ITEM);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	static const LPCTSTR sm_szAnatomy_StrLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_1),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_2),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_3),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_4),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_5),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_6),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_7),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_8),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_9),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_10),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STR_11)
	};

	int iStr = pChar->Stat_GetBase(STAT_STR) / 10;
	if ( iStr < 0 )
		iStr = 0;
	else if ( static_cast<size_t>(iStr) >= COUNTOF(sm_szAnatomy_StrLevel) )
		iStr = COUNTOF(sm_szAnatomy_StrLevel) - 1;

	static const LPCTSTR sm_szAnatomy_DexLevel[] =
	{
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_1),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_2),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_3),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_4),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_5),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_6),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_7),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_8),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_9),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_10),
		g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_DEX_11)
	};

	int iDex = pChar->Stat_GetBase(STAT_DEX) / 10;
	if ( iDex < 0 )
		iDex = 0;
	else if ( static_cast<size_t>(iDex) >= COUNTOF(sm_szAnatomy_DexLevel) )
		iDex = COUNTOF(sm_szAnatomy_DexLevel) - 1;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_RESULT), sm_szAnatomy_StrLevel[iStr], sm_szAnatomy_DexLevel[iDex]);
	addObjMessage(pszTemp, pChar);

	WORD wSkill = m_pChar->Skill_GetBase(SKILL_ANATOMY);
	if ( wSkill >= 650 )
	{
		int iStam = ((pChar->Stat_GetVal(STAT_DEX) * 100) / maximum(pChar->Stat_GetMax(STAT_DEX), 1)) / 10;
		if ( iStam < 0 )
			iStam = 0;
		else if ( static_cast<size_t>(iStam) >= COUNTOF(sm_szPercentLevel) )
			iStam = COUNTOF(sm_szPercentLevel) - 1;

		sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_STAM_RESULT), sm_szPercentLevel[iStam]);
		addObjMessage(pszTemp, pChar);
	}

	return wSkill;
}

int CClient::OnSkill_Forensics(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_Forensics");
	// SKILL_FORENSICS

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_FORENSICS);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 12;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_REACH_FAIL);
		return -SKTRIG_QTY;
	}

	CItemCorpse *pItemCorpse = dynamic_cast<CItemCorpse *>(pObj);
	if ( !pItemCorpse )
	{
		SysMessageDefault(DEFMSG_FORENSICS_NOTCORPSE);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	CChar *pChar = pItemCorpse->m_itCorpse.m_uidKiller.CharFind();
	LPCTSTR pszName = pChar ? pChar->GetName() : g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_NOONE);

	if ( pItemCorpse->m_itCorpse.m_carved )
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_CARVEDBY), pszName);
	else
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_KILLEDBY), pszName);
		SysMessageDefault(DEFMSG_FORENSICS_NOTCARVED);
	}

	return m_pChar->Skill_GetBase(SKILL_FORENSICS);
}

int CClient::OnSkill_TasteID(CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_TasteID");
	// SKILL_TASTEID

	CObjBase *pObj = uid.ObjFind();
	if ( !pObj )
		return -SKTRIG_QTY;

	CSkillDef *pSkillDef = g_Cfg.GetSkillDef(SKILL_TASTEID);
	if ( pSkillDef->m_Range <= 0 )
		pSkillDef->m_Range = 1;

	if ( m_pChar->GetDist(pObj) > pSkillDef->m_Range )
	{
		SysMessageDefault(DEFMSG_TASTEID_TOOFAR);
		return -SKTRIG_QTY;
	}

	CItem *pItem = dynamic_cast<CItem *>(pObj);
	if ( !pItem || !m_pChar->Food_CanEat(pItem) )
	{
		SysMessageDefault(DEFMSG_TASTEID_TARG_NOFOOD);
		return -SKTRIG_QTY;
	}

	if ( fTest )
		return Calc_GetRandVal(60);

	SysMessageDefault(pItem->m_itFood.m_poison_skill ? DEFMSG_TASTEID_POISON : DEFMSG_TASTEID_NOTHING);

	return m_pChar->Skill_GetBase(SKILL_TASTEID);
}

int CClient::OnSkill_Info(SKILL_TYPE skill, CGrayUID uid, bool fTest)
{
	ADDTOCALLSTACK("CClient::OnSkill_Info");
	// Skill timer has expired
	// RETURN: difficulty credit (0 - 1000)
	//  < 0 = immediate failure

	switch ( skill )
	{
		case SKILL_ANIMALLORE:	return OnSkill_AnimalLore(uid, fTest);
		case SKILL_ARMSLORE:	return OnSkill_ArmsLore(uid, fTest);
		case SKILL_ANATOMY:		return OnSkill_Anatomy(uid, fTest);
		case SKILL_ITEMID:		return OnSkill_ItemID(uid, fTest);
		case SKILL_EVALINT:		return OnSkill_EvalInt(uid, fTest);
		case SKILL_FORENSICS:	return OnSkill_Forensics(uid, fTest);
		case SKILL_TASTEID:		return OnSkill_TasteID(uid, fTest);
		default:				return -SKTRIG_QTY;
	}
}

////////////////////////////////////////
// Targeted skills and actions.

void CClient::OnTarg_Skill(CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill");
	// Targetted skill now has it's target.
	// Response to CLIMODE_TARG_SKILL from Event_Skill_Use() select button from skill window

	if ( !pObj )
		return;

	SetTargMode();	// just make sure last targ mode is gone
	m_Targ_UID = pObj->GetUID();	// keep for 'last target' info

	if ( g_Cfg.IsSkillFlag(m_tmSkillTarg.m_Skill, SKF_SCRIPTED) )
	{
		const CSkillDef *pSkillDef = g_Cfg.GetSkillDef(m_tmSkillTarg.m_Skill);
		if ( pSkillDef && !pSkillDef->m_sTargetPrompt.IsEmpty() )
		{
			m_pChar->m_Act_Targ = m_Targ_UID;
			m_pChar->Skill_Start(m_tmSkillTarg.m_Skill);
		}
		return;
	}

	switch ( m_tmSkillTarg.m_Skill )
	{
		// Delayed response type skills
		case SKILL_BEGGING:
		case SKILL_STEALING:
		case SKILL_TAMING:
		case SKILL_ENTICEMENT:
		case SKILL_STEALTH:
		case SKILL_REMOVETRAP:

		// Informational skills (instant return)
		case SKILL_ANIMALLORE:
		case SKILL_ARMSLORE:
		case SKILL_ANATOMY:
		case SKILL_ITEMID:
		case SKILL_EVALINT:
		case SKILL_FORENSICS:
		case SKILL_TASTEID:
		{
			m_pChar->m_Act_Targ = m_Targ_UID;
			m_pChar->Skill_Start(m_tmSkillTarg.m_Skill);
		}
		case SKILL_PROVOCATION:
		{
			if ( !pObj->IsChar() )
			{
				SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
				return;
			}
			addTarget(CLIMODE_TARG_SKILL_PROVOKE, g_Cfg.GetDefaultMsg(DEFMSG_PROVOKE_SELECT), false, true);
			return;
		}
		case SKILL_POISONING:
		{
			addTarget(CLIMODE_TARG_SKILL_POISON, g_Cfg.GetDefaultMsg(DEFMSG_POISONING_SELECT_1), false, true);
			return;
		}
		default:
		{
			// Not a targetted skill
			return;
		}
	}
}

void CClient::OnTarg_Skill_Provoke(CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Provoke");
	// CLIMODE_TARG_SKILL_PROVOKE

	if ( !pObj || !pObj->IsChar() )
	{
		SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
		return;
	}

	m_pChar->m_Act_TargPrv = m_Targ_UID;		// provoke him
	m_pChar->m_Act_Targ = pObj->GetUID();		// against him
	m_pChar->Skill_Start(SKILL_PROVOCATION);
}

void CClient::OnTarg_Skill_Poison(CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Poison");
	// CLIMODE_TARG_SKILL_POISON

	if ( !pObj )
		return;

	m_pChar->m_Act_TargPrv = m_Targ_UID;		// poison this
	m_pChar->m_Act_Targ = pObj->GetUID();		// with this poison
	m_pChar->Skill_Start(SKILL_POISONING);
}

void CClient::OnTarg_Skill_Herd_Dest(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Herd_Dest");
	UNREFERENCED_PARAMETER(pObj);
	// CLIMODE_TARG_SKILL_HERD_DEST

	m_pChar->m_Act_p = pt;
	m_pChar->m_Act_Targ = m_Targ_UID;			// herd this
	m_pChar->m_Act_TargPrv = m_Targ_PrvUID;		// with this crook
	m_pChar->Skill_Start(SKILL_HERDING);
}

void CClient::OnTarg_Skill_Magery(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Magery");
	// The client player has targeted a spell.
	// CLIMODE_TARG_SKILL_MAGERY

	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_Spell);
	if ( !pSpellDef )
		return;

	if ( pObj )
	{
		if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_OBJ) )
		{
			SysMessageDefault(DEFMSG_MAGERY_4);
			return;
		}

		if ( pObj->IsItem() && !pSpellDef->IsSpellType(SPELLFLAG_TARG_ITEM) )
		{
			SysMessageDefault(DEFMSG_MAGERY_1);
			return;
		}

		if ( pObj->IsChar() )
		{
			if ( !pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) )
			{
				SysMessageDefault(DEFMSG_MAGERY_2);
				return;
			}
			CChar *pChar = static_cast<CChar *>(pObj);
			if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NO_PLAYER) && pChar->m_pPlayer )
			{
				SysMessageDefault(DEFMSG_MAGERY_7);
				return;
			}
			if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NO_NPC) && pChar->m_pNPC )
			{
				SysMessageDefault(DEFMSG_MAGERY_8);
				return;
			}
		}

		if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF) && (pObj == m_pChar) && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault(DEFMSG_MAGERY_3);
			return;
		}
	}

	m_pChar->m_atMagery.m_Spell = m_tmSkillMagery.m_Spell;
	m_pChar->m_atMagery.m_SummonID = m_tmSkillMagery.m_SummonID;

	m_pChar->m_Act_TargPrv = m_Targ_PrvUID;		// source (char/wand)
	m_pChar->m_Act_Targ = pObj ? pObj->GetUID() : static_cast<CGrayUID>(UID_CLEAR);
	m_pChar->m_Act_p = pt;
	m_Targ_p = pt;

	if ( IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) && m_pChar->m_pClient )
	{
		if ( g_Cfg.IsSkillFlag(m_pChar->m_Act_SkillCurrent, SKF_MAGIC) )
		{
			SysMessageDefault(DEFMSG_MAGERY_5);
			return;
		}
		m_pChar->Spell_CastDone();
		return;
	}

	int skill;
	if ( !pSpellDef->GetPrimarySkill(&skill, NULL) )
		return;

	m_pChar->Skill_Start(static_cast<SKILL_TYPE>(skill));
}

void CClient::OnTarg_Pet_Command(CObjBase *pObj, const CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Command");
	// CLIMODE_TARG_PET_CMD
	// Any pet command requiring a target.
	// m_Targ_UID = the pet i was instructing.
	// m_tmPetCmd = command index.
	// m_Targ_Text = the args to the command that got us here.

	if ( m_tmPetCmd.m_fAllPets )
	{
		bool fGhostSpeak = m_pChar->IsSpeakAsGhost();
		CWorldSearch AreaChars(m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT);
		for (;;)
		{
			CChar *pCharPet = AreaChars.GetChar();
			if ( !pCharPet )
				break;
			if ( pCharPet == m_pChar )
				continue;
			if ( fGhostSpeak && !pCharPet->CanUnderstandGhost() )
				continue;
			if ( !pCharPet->NPC_IsOwnedBy(GetChar(), true) )
				continue;
			pCharPet->NPC_OnHearPetCmdTarg(m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text);
		}
		return;
	}

	CChar *pCharPet = m_Targ_UID.CharFind();
	if ( pCharPet )
		pCharPet->NPC_OnHearPetCmdTarg(m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text);
}

void CClient::OnTarg_Pet_Stable(CChar *pCharPet)
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Stable");
	// CLIMODE_PET_STABLE
	// NOTE: You are only allowed to stable x creatures here.
	// m_Targ_PrvUID = stable master.

	CChar *pCharMaster = m_Targ_PrvUID.CharFind();
	if ( !pCharMaster )
		return;

	if ( !pCharPet || pCharPet->m_pPlayer || (pCharMaster == pCharPet) )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_FAIL));
		return;
	}
	if ( !pCharMaster->CanSeeLOS(pCharPet) )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_LOS));
		return;
	}
	if ( !pCharPet->NPC_IsOwnedBy(m_pChar) )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_OWNER));
		return;
	}
	if ( pCharPet->IsStatFlag(STATF_Conjured) )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_SUMMON));
		return;
	}

	CItemContainer *pPetPack = pCharPet->GetContainer(LAYER_PACK);
	if ( pPetPack && !pPetPack->IsEmpty() )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_UNLOAD));
		return;
	}

	// Shrink the pet and put it in the bank box of the stable master
	CItem *pPetItem = pCharPet->Make_Figurine(m_pChar->GetUID());
	if ( !pPetItem )
	{
		pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_FAIL));
		return;
	}

	if ( IsSetOF(OF_PetSlots) )
		m_pChar->FollowersUpdate(pCharPet, -pCharPet->m_FollowerSlots);

	pCharMaster->GetContainerCreate(LAYER_BANKBOX)->ContentAdd(pPetItem);
	pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_TARG_SUCCESS));
}

//-----------------------------------------------------------------------
// Targetted items with special props.

bool CClient::OnTarg_Use_Deed(CItem *pDeed, CPointMap &pt)
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Deed");
	// Check if an deed (IT_DEED) can create an multi here

	if ( !m_pChar->CanUse(pDeed, true) )
		return false;

	const CItemBase *pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(pDeed->m_itDeed.m_Type)));
	if ( !OnTarg_Use_Multi(pItemDef, pt, pDeed->m_Attr, pDeed->GetHue()) )
		return false;

	pDeed->Delete();
	return true;
}

CItem *CClient::OnTarg_Use_Multi(const CItemBase *pItemDef, CPointMap &pt, DWORD dwAttr, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Multi");
	// Check if an multi can be created here
	// NOTE: ATTR_MAGIC will skip region checks

	if ( !pItemDef || !pt.GetRegion(REGION_TYPE_AREA) )
		return NULL;

	const CItemBaseMulti *pMultiDef = dynamic_cast<const CItemBaseMulti *>(pItemDef);
	if ( pMultiDef && !(dwAttr & ATTR_MAGIC) )
	{
		if ( CItemBase::IsID_House(pItemDef->GetID()) )
			pt.m_y -= static_cast<signed short>(pMultiDef->m_rect.m_bottom) - 1;

		// Check region and bumpy terrain
		CGRect rect = pMultiDef->m_rect;
		rect.m_map = pt.m_map;
		rect.OffsetRect(pt.m_x, pt.m_y);
		CPointMap ptCheck = pt;

		for ( int x = rect.m_left; x < rect.m_right; ++x )
		{
			ptCheck.m_x = static_cast<signed short>(x);
			for ( int y = rect.m_top; y < rect.m_bottom; ++y )
			{
				ptCheck.m_y = static_cast<signed short>(y);
				if ( !ptCheck.IsValidPoint() )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_MULTI_FAIL);
					return NULL;
				}

				CRegionBase *pRegion = ptCheck.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
				if ( !pRegion || pRegion->IsFlag(REGION_FLAG_NOBUILDING) )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_MULTI_FAIL);
					if ( !IsPriv(PRIV_GM) )
						return NULL;
				}

				bool fShip = pItemDef->IsType(IT_SHIP);
				DWORD dwBlockFlags = fShip ? CAN_C_SWIM : CAN_C_WALK;
				ptCheck.m_z = g_World.GetHeightPoint2(ptCheck, dwBlockFlags, true);	//should really use the 2nd function? it does fixed #2373

				if ( fShip )
				{
					if ( !(dwBlockFlags & CAN_I_WATER) )
					{
						SysMessageDefault(DEFMSG_ITEMUSE_MULTI_SHIPW);
						if ( !IsPriv(PRIV_GM) )
							return NULL;
					}
				}
				else
				{
					if ( dwBlockFlags & (CAN_I_WATER|CAN_I_BLOCK|CAN_I_CLIMB) )
					{
						SysMessageDefault(DEFMSG_ITEMUSE_MULTI_BLOCKED);
						if ( !IsPriv(PRIV_GM) )
							return NULL;
					}
				}

				if ( abs(ptCheck.m_z - pt.m_z) > 4 )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_MULTI_BUMP);
					if ( !IsPriv(PRIV_GM) )
						return NULL;
				}
			}
		}

		// Check for chars in the way
		CWorldSearch Area(pt, maximum(rect.GetWidth(), rect.GetHeight()));
		for (;;)
		{
			CChar *pChar = Area.GetChar();
			if ( !pChar )
				break;
			if ( !rect.IsInside2d(pChar->GetTopPoint()) )
				continue;
			if ( pChar->IsPriv(PRIV_GM) && !CanSee(pChar) )
				continue;

			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_MULTI_INTWAY), pChar->GetName());
			if ( !IsPriv(PRIV_GM) )
				return NULL;
			if ( pChar != m_pChar )
				pChar->Spell_Teleport(m_pChar->GetTopPoint(), false, false);
		}
	}

	CItem *pItemNew = CItem::CreateTemplate(pItemDef->GetID(), NULL, m_pChar);
	if ( !pItemNew )
	{
		SysMessageDefault(DEFMSG_ITEMUSE_MULTI_COLLAPSE);
		return NULL;
	}

	pItemNew->SetAttr(ATTR_MOVE_NEVER | (dwAttr & (ATTR_MAGIC|ATTR_INVIS)));
	pItemNew->SetHue(wHue);
	pItemNew->MoveToUpdate(pt);

	CItemMulti *pMulti = dynamic_cast<CItemMulti *>(pItemNew);
	if ( pMulti )
		pMulti->Multi_Create(m_pChar);

	if ( pItemDef->IsType(IT_STONE_GUILD) )
	{
		// Set guild master and ask for an guild name
		CItemStone *pStone = dynamic_cast<CItemStone *>(pItemNew);
		if ( pStone )
			pStone->AddRecruit(m_pChar, STONEPRIV_MASTER);
		addPromptConsole(CLIMODE_PROMPT_STONE_NAME, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_GUILDSTONE_NEW), pItemNew->GetUID());
	}

	return pItemNew;
}

bool CClient::OnTarg_Use_Item(CObjBase *pObjTarg, CPointMap &pt, ITEMID_TYPE id)
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Item");
	// CLIMODE_TARG_USE_ITEM started from Event_DoubleClick()
	// Targetted an item to be used on some other object (char or item).
	// Solve for the intersection of the items.
	// ARGS:
	//  id = static item id
	//  uid = the target.
	//  m_Targ_UID = what is the used object on the target
	//
	// NOTE:
	//  Assume we can see the target.
	//
	// RETURN:
	//  true = success.

	CItem *pItemUse = m_Targ_UID.ItemFind();
	if ( !pItemUse )
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_GONE));
		return false;
	}
	if ( pItemUse->GetParent() != m_tmUseItem.m_pParent )
	{
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_MOVED));
		return false;
	}

	m_Targ_PrvUID = m_Targ_UID;		// item used
	m_Targ_p = pt;

	ITRIG_TYPE type;
	if ( pObjTarg )
	{
		type = pObjTarg->IsChar() ? ITRIG_TARGON_CHAR : ITRIG_TARGON_ITEM;
		m_Targ_UID = pObjTarg->GetUID();
		m_pChar->UpdateDir(pObjTarg);
	}
	else
	{
		type = ITRIG_TARGON_GROUND;
		m_Targ_UID.ClearUID();
		if ( pt.IsValidPoint() )
			m_pChar->UpdateDir(pt);
	}

	if ( IsTrigUsed(CItem::sm_szTrigName[type]) || IsTrigUsed(CChar::sm_szTrigName[(CTRIG_itemAfterClick - 1) + type]) )	//ITRIG_TARGON_GROUND, ITRIG_TARGON_CHAR, ITRIG_TARGON_ITEM
	{
		CScriptTriggerArgs Args(id, 0, pObjTarg);
		if ( pItemUse->OnTrigger(type, m_pChar, &Args) == TRIGRET_RET_TRUE )
			return true;
	}

	// NOTE: We have NOT checked to see if the targetted object is within reasonable distance
	// Call CanUse(pItemTarg)

	// What did I target it on ? this could be null if ground is the target.
	CChar *pCharTarg = dynamic_cast<CChar *>(pObjTarg);
	CItem *pItemTarg = dynamic_cast<CItem *>(pObjTarg);

	switch ( pItemUse->GetType() )
	{
		case IT_COMM_CRYSTAL:
		{
			if ( !pItemTarg || !pItemTarg->IsType(IT_COMM_CRYSTAL) )
				return false;
			pItemUse->m_uidLink = pItemTarg->GetUID();
			pItemUse->Speak("Linked");
			pItemUse->UpdatePropertyFlag();
			return true;
		}
		case IT_POTION:
		{
			// Use a potion on something else
			if ( RES_GET_INDEX(pItemUse->m_itPotion.m_Type) == SPELL_Explosion )
			{
				// Throw explosion potion
				if ( !pItemUse->IsItemEquipped() || (pItemUse->GetEquipLayer() != LAYER_DRAGGING) )
					return false;

				// Check if we have clear LOS to target location and also if it's not over a wall (to prevent hit chars on the other side of the wall)
				if ( m_pChar->CanSeeLOS(pt, NULL, UO_MAP_VIEW_SIGHT, LOS_NB_WINDOWS) && !g_World.IsItemTypeNear(pt, IT_WALL, 0, true) )
				{
					pItemUse->SetAttr(ATTR_MOVE_NEVER);
					pItemUse->MoveToUpdate(pt);
					pItemUse->Effect(EFFECT_BOLT, pItemUse->GetDispID(), m_pChar, 7, 0, false, maximum(0, pItemUse->GetHue() - 1));
				}
			}
			return true;
		}
		case IT_KEY:
		{
			return m_pChar->Use_Key(pItemUse, pItemTarg);
		}
		case IT_ORE:
		{
			if ( !m_pChar->CanUse(pItemUse, true) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_ORE_TOOFAR);
				return false;
			}
			else if ( !pItemTarg || !m_pChar->CanTouch(pItemTarg) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_TOOFAR);
				return false;
			}
			else if ( pItemTarg->IsType(IT_FORGE) )
			{
				// Target is an forge, so smelt the ore
				return m_pChar->Skill_SmeltOre(pItemUse);
			}
			else if ( pItemTarg->IsType(IT_ORE) )
			{
				// Target is an ore pile, so just combine both piles
				if ( pItemTarg == pItemUse )
					return false;
				else if ( pItemTarg->GetID() != pItemUse->GetID() )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_ORE_COMBINE);
					return false;
				}
				else if ( pItemTarg->GetAmount() + pItemUse->GetAmount() > g_Cfg.m_iItemsMaxAmount )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_ORE_TOOMUCH);
					return false;
				}
				pItemTarg->SetAmountUpdate(pItemTarg->GetAmount() + pItemUse->GetAmount());
				pItemUse->Delete();
				return true;
			}
			break;
		}
		case IT_CANNON_MUZZLE:
		{
			// We have targetted the cannon to something
			if ( (pItemUse->m_itCannon.m_Load & 3) != 3 )
			{
				if ( m_pChar->Use_Cannon_Feed(pItemUse, pItemTarg) )
					pItemTarg->ConsumeAmount();
				return true;
			}

			// Fire!
			pItemUse->m_itCannon.m_Load = 0;
			pItemUse->Sound(0x207);
			pItemUse->Effect(EFFECT_OBJ, ITEMID_FX_TELE_VANISH, pItemUse, 9, 6);

			// Just explode on the ground?
			if ( pObjTarg )
			{
				// Check distance and LOS
				if ( pItemUse->GetDist(pObjTarg) > UO_MAP_VIEW_SIGHT )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_TOOFAR);
					return true;
				}

				// Hit the target
				pObjTarg->Sound(0x207);
				pObjTarg->Effect(EFFECT_BOLT, ITEMID_Cannon_Ball, pItemUse, 8, 0, true);

				CChar *pChar = dynamic_cast<CChar *>(this);
				if ( pChar )
					pChar->OnTakeDamage(80 + Calc_GetRandVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE);

				CItem *pItem = dynamic_cast<CItem *>(this);
				if ( pItem )
					pItem->OnTakeDamage(80 + Calc_GetRandVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE);
			}
			return true;
		}
		case IT_WEAPON_MACE_PICK:
		{
			// Mine at the location (shovel)
			m_pChar->m_Act_p = pt;
			m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
			m_pChar->m_atResource.m_ridType = RESOURCE_ID(RES_TYPEDEF, IT_ROCK);
			return m_pChar->Skill_Start(SKILL_MINING);
		}
		case IT_WEAPON_MACE_CROOK:
		{
			// SKILL_HERDING
			// Selected a creature to herd
			// Move the selected item or char to this location
			if ( !pCharTarg )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_CROOK_TRY);
				return false;
			}
			addTarget(CLIMODE_TARG_SKILL_HERD_DEST, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CROOK_TARGET), true);
			return true;
		}
		case IT_WEAPON_MACE_SMITH:
		{
			// Near a forge? smithing?
			if ( !pItemTarg )
				break;
			if ( pItemTarg->IsType(IT_INGOT) )
				return Cmd_Skill_Smith(pItemTarg);
			if ( pItemTarg->Armor_IsRepairable() )
			{
				m_pChar->Use_Repair(pItemTarg);
				return true;
			}
			break;
		}
		case IT_CARPENTRY_CHOP:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_AXE:
		case IT_WEAPON_SWORD:
		{
			// Use sharp weapon on something
			if ( pCharTarg )
			{
				if ( !m_pChar->CanTouch(pCharTarg) )
					return false;

				switch ( pCharTarg->GetID() )
				{
					case CREID_Sheep:
					{
						// Get the wool
						CItem *pWool = CItem::CreateBase(ITEMID_WOOL);
						ASSERT(pWool);
						m_pChar->ItemBounce(pWool);
						pCharTarg->SetID(CREID_Sheep_Sheered);

						// Set growth timer
						pWool = CItem::CreateBase(ITEMID_WOOL);
						ASSERT(pWool);
						pWool->SetTimeout(g_Cfg.m_iWoolGrowthTime);
						pCharTarg->LayerAdd(pWool, LAYER_FLAG_Wool);
						return true;
					}
					case CREID_Sheep_Sheered:
					{
						SysMessageDefault(DEFMSG_ITEMUSE_WEAPON_WWAIT);
						return true;
					}
					default:
						break;
				}
				break;
			}
			else
			{
				switch ( m_pChar->CanTouchStatic(pt, id, pItemTarg) )
				{
					case IT_JUNK:
					{
						SysMessageDefault(DEFMSG_ITEMUSE_JUNK_REACH);
						return false;
					}
					case IT_FOLIAGE:
					case IT_TREE:
					{
						m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
						m_pChar->m_Act_Targ = m_Targ_UID;
						m_pChar->m_Act_p = pt;
						m_pChar->m_atResource.m_ridType = RESOURCE_ID(RES_TYPEDEF, IT_TREE);
						return m_pChar->Skill_Start(SKILL_LUMBERJACKING);
					}
					case IT_LOG:
					{
						if ( !pItemTarg || !m_pChar->CanUse(pItemTarg, true) )
						{
							SysMessageDefault(DEFMSG_ITEMUSE_LOG_UNABLE);
							return false;
						}
						if ( pItemUse->IsType(IT_CARPENTRY_CHOP) )
						{
							if ( IsTrigUsed(TRIGGER_SKILLMENU) )
							{
								CScriptTriggerArgs args("sm_carpentry");
								if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
									return true;
							}
							return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_carpentry"));
						}
						if ( pItemUse->IsSameDispID(ITEMID_DAGGER) )
						{
							m_Targ_UID = pItemTarg->GetUID();
							if ( IsTrigUsed(TRIGGER_SKILLMENU) )
							{
								CScriptTriggerArgs args("sm_bowcraft");
								if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
									return true;
							}
							return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_bowcraft"));
						}
						SysMessageDefault(DEFMSG_ITEMUSE_LOG_USE);
						return false;
					}
					case IT_FISH:
					{
						// Carve up fish parts
						if ( !pItemTarg || !m_pChar->CanUse(pItemTarg, true) )
						{
							SysMessageDefault(DEFMSG_ITEMUSE_FISH_UNABLE);
							return false;
						}
						pItemTarg->SetID(ITEMID_FOOD_FISH_RAW);
						pItemTarg->SetHue(HUE_DEFAULT);
						pItemTarg->SetAmount(4 * pItemTarg->GetAmount());
						pItemTarg->Update();
						return true;
					}
					case IT_CORPSE:
					{
						if ( !m_pChar->CanUse(pItemTarg, false) )
							return false;
						m_pChar->Use_CarveCorpse(dynamic_cast<CItemCorpse *>(pItemTarg));
						return true;
					}
					case IT_FRUIT:
					case IT_REAGENT_RAW:
					{
						// Turn the fruit into seed
						if ( !pItemTarg || !m_pChar->CanUse(pItemTarg, true) )
							return false;
						RESOURCE_ID defaultseed = g_Cfg.ResourceGetIDType(RES_ITEMDEF, "DEFAULTSEED");
						pItemTarg->SetDispID(static_cast<ITEMID_TYPE>(defaultseed.GetResIndex()));
						pItemTarg->SetType(IT_SEED);
						TCHAR *pszTemp = Str_GetTemp();
						sprintf(pszTemp, "%s seed", pItemTarg->GetName());
						pItemTarg->SetName(pszTemp);
						pItemTarg->Update();
						return true;
					}
					//case IT_FOLIAGE:
					case IT_CROPS:
					{
						if ( !pItemTarg )
							return false;
						pItemTarg->Plant_CropReset();
						return true;
					}
					default:
					{
						// Item to smash? furniture?
						if ( !m_pChar->CanMove(pItemTarg) )
						{
							SysMessageDefault(DEFMSG_ITEMUSE_WEAPON_IMMUNE);
							return false;
						}
						if ( m_pChar->IsTakeCrime(pItemTarg) )
						{
							SysMessageDefault(DEFMSG_ITEMUSE_STEAL);
							return false;
						}
						pItemTarg->OnTakeDamage(1, m_pChar, DAMAGE_HIT_BLUNT);
						return true;
					}
				}
			}
			break;
		}
		case IT_BANDAGE:
		{
			// Use bandages on some creature.
			if ( !pCharTarg )
				return false;
			m_pChar->m_Act_TargPrv = m_Targ_PrvUID;
			m_pChar->m_Act_Targ = m_Targ_UID;
			return m_pChar->Skill_Start(pCharTarg->NPC_IsAnimal() ? SKILL_VETERINARY : SKILL_HEALING);
		}
		case IT_SEED:
		{
			return m_pChar->Use_Seed(pItemUse, &pt);
		}
		case IT_DEED:
		{
			return OnTarg_Use_Deed(pItemUse, pt);
		}
		case IT_WOOL:
		case IT_COTTON:
		{
			// Use on a spinning wheel
			if ( !pItemTarg || !pItemTarg->IsType(IT_SPINWHEEL) )
				break;
			if ( !m_pChar->CanUse(pItemTarg, false) )
				return false;

			pItemTarg->SetAnim(static_cast<ITEMID_TYPE>(pItemTarg->GetID() + 1), 2 * TICK_PER_SEC);
			pItemUse->ConsumeAmount(1);

			CItem *pNewItem = NULL;
			if ( pItemUse->IsType(IT_WOOL) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_WOOL_CREATE);
				pNewItem = CItem::CreateScript(ITEMID_YARN1, m_pChar);
				if ( pNewItem )
				{
					pNewItem->SetAmountUpdate(3);
					m_pChar->ItemBounce(pNewItem);
				}
			}
			else
			{
				SysMessageDefault(DEFMSG_ITEMUSE_COTTON_CREATE);
				pNewItem = CItem::CreateScript(ITEMID_THREAD1, m_pChar);
				if ( pNewItem )
				{
					pNewItem->SetAmountUpdate(6);
					m_pChar->ItemBounce(pNewItem);
				}
			}
			return true;
		}
		case IT_KEYRING:
		{
			if ( !pItemTarg )
				return false;

			CItemContainer *pKeyRing = dynamic_cast<CItemContainer *>(pItemUse);
			if ( !pKeyRing )
				return false;

			if ( pItemTarg == pItemUse )
			{
				// Use a keyring on self = remove all keys
				pKeyRing->ContentsTransfer(m_pChar->GetContainer(LAYER_PACK), false);
				return true;
			}

			CItem *pKey = NULL;
			bool fLockable = pItemTarg->IsTypeLockable();

			if ( fLockable && pItemTarg->m_itContainer.m_lockUID )
			{
				// Try all the keys on the object
				pKey = pKeyRing->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_KEY), pItemTarg->m_itContainer.m_lockUID);
			}
			if ( !pKey )
			{
				// Are we trying to lock it down?
				if ( m_pChar->m_pArea->GetResourceID().IsItem() )
				{
					pKey = pKeyRing->ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_KEY), m_pChar->m_pArea->GetResourceID());
					if ( pKey )
					{
						if ( m_pChar->Use_MultiLockDown(pItemTarg) )
							return true;
					}
				}

				if ( !fLockable || !pItemTarg->m_itContainer.m_lockUID )
					SysMessageDefault(DEFMSG_ITEMUSE_KEY_NOLOCK);
				else
					SysMessageDefault(DEFMSG_ITEMUSE_KEY_NOKEY);
				return false;
			}
			return m_pChar->Use_Key(pKey, pItemTarg);
		}
		case IT_SCISSORS:
		{
			if ( pItemTarg )
			{
				if ( !m_pChar->CanUse(pItemTarg, true) )
					return false;

				ITEMID_TYPE iOutID = ITEMID_BANDAGES1;
				WORD wOutQty = 0;

				switch ( pItemTarg->GetType() )
				{
					case IT_CLOTH_BOLT:
					{
						pItemTarg->ConvertBolttoCloth();
						m_pChar->Sound(SOUND_SNIP);	// snip noise.
						return true;
					}
					case IT_CLOTH:
					{
						wOutQty = pItemTarg->GetAmount();
						break;
					}
					case IT_CLOTHING:
					{
						wOutQty = static_cast<WORD>(pItemTarg->GetWeight()) / WEIGHT_UNITS;
						break;
					}
					case IT_HIDE:
					{
						iOutID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemTarg->Item_GetDef()->m_ttNormal.m_tData1));
						if ( !iOutID )
							iOutID = ITEMID_LEATHER_1;
						wOutQty = pItemTarg->GetAmount();
						break;
					}
					default:
						break;
				}
				if ( wOutQty )
				{
					CItem *pItemNew = CItem::CreateBase(iOutID);
					ASSERT(pItemNew);
					pItemNew->SetHue(pItemTarg->GetHue());
					pItemNew->SetAmount(wOutQty);
					pItemTarg->Delete();
					m_pChar->ItemBounce(pItemNew);
					m_pChar->Sound(SOUND_SNIP);
					return true;
				}
			}
			SysMessageDefault(DEFMSG_ITEMUSE_SCISSORS_USE);
			return false;
		}
		case IT_YARN:
		case IT_THREAD:
		{
			if ( !pItemTarg || !pItemTarg->IsType(IT_LOOM) )
				break;
			if ( !m_pChar->CanUse(pItemTarg, false) )
				return false;

			//pItemTarg->SetAnim(static_cast<ITEMID_TYPE>(pItemTarg->GetID() + 1), 2 * TICK_PER_SEC);

			if ( pItemTarg->m_itLoom.m_ClothID && (pItemTarg->m_itLoom.m_ClothID != pItemUse->GetDispID()) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_LOOM_REMOVE);
				CItem *pItemCloth = CItem::CreateTemplate(pItemTarg->m_itLoom.m_ClothID, NULL, m_pChar);
				if ( pItemCloth )
				{
					pItemCloth->SetAmount(pItemTarg->m_itLoom.m_ClothQty);
					pItemTarg->m_itLoom.m_ClothQty = 0;
					pItemTarg->m_itLoom.m_ClothID = ITEMID_NOTHING;
					m_pChar->ItemBounce(pItemCloth);
				}
				return true;
			}

			pItemTarg->m_itLoom.m_ClothID = pItemUse->GetDispID();

			static const LPCTSTR sm_Txt_LoomUse[] =
			{
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BOLT_1),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BOLT_2),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BOLT_3),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BOLT_4),
				g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_BOLT_5)
			};

			WORD wUsed = 0;
			WORD wNeed = COUNTOF(sm_Txt_LoomUse) - 1;
			WORD wHave = pItemTarg->m_itLoom.m_ClothQty;
			if ( wHave < wNeed )
			{
				wNeed -= wHave;
				wUsed = static_cast<WORD>(pItemUse->ConsumeAmount(wNeed));
			}

			if ( wHave + wUsed < static_cast<WORD>(COUNTOF(sm_Txt_LoomUse) - 1) )
			{
				pItemTarg->m_itLoom.m_ClothQty += wUsed;
				SysMessage(sm_Txt_LoomUse[pItemTarg->m_itLoom.m_ClothQty]);
			}
			else
			{
				SysMessage(sm_Txt_LoomUse[COUNTOF(sm_Txt_LoomUse) - 1]);
				pItemTarg->m_itLoom.m_ClothQty = 0;
				pItemTarg->m_itLoom.m_ClothID = ITEMID_NOTHING;
				m_pChar->ItemBounce(CItem::CreateScript(ITEMID_CLOTH_BOLT1, m_pChar));
			}
			return true;
		}
		case IT_BANDAGE_BLOOD:
		{
			// Use on water to clean them
			switch ( m_pChar->CanTouchStatic(pt, id, pItemTarg) )
			{
				case IT_WATER:
				case IT_WATER_WASH:
				{
					pItemUse->SetID(ITEMID_BANDAGES1);
					pItemUse->Update();
					return true;
				}
				case IT_JUNK:
				{
					SysMessageDefault(DEFMSG_ITEMUSE_BANDAGE_REACH);
					break;
				}
				default:
				{
					SysMessageDefault(DEFMSG_ITEMUSE_BANDAGE_CLEAN);
					break;
				}
			}
			return false;
		}
		case IT_FISH_POLE:
		{
			m_pChar->m_Act_p = pt;
			m_pChar->m_atResource.m_ridType = RESOURCE_ID(RES_TYPEDEF, IT_WATER);
			return m_pChar->Skill_Start(SKILL_FISHING);
		}
		case IT_LOCKPICK:
		{
			// Using a lock pick on something
			if ( !pItemTarg )
				return false;
			m_pChar->m_Act_Targ = m_Targ_UID;			// the locked item to be picked
			m_pChar->m_Act_TargPrv = m_Targ_PrvUID;		// the pick
			if ( !m_pChar->CanUse(pItemTarg, false) )
				return false;
			return m_pChar->Skill_Start(SKILL_LOCKPICKING);
		}
		case IT_CANNON_BALL:
		{
			if ( m_pChar->Use_Cannon_Feed(pItemTarg, pItemUse) )
			{
				pItemUse->Delete();
				return true;
			}
			break;
		}
		case IT_DYE:
		{
			if ( (pItemTarg && pItemTarg->IsType(IT_DYE_VAT)) || (pCharTarg && ((pCharTarg == m_pChar) || IsPriv(PRIV_GM))) )
			{
				addDyeOption(pObjTarg);
				return true;
			}
			SysMessageDefault(DEFMSG_ITEMUSE_DYE_FAIL);
			return false;
		}
		case IT_DYE_VAT:
		{
			// Use the dye vat on some object
			if ( !pObjTarg )
				return false;

			if ( (pObjTarg->GetTopLevelObj() != m_pChar) && !IsPriv(PRIV_GM) )
			{
				SysMessageDefault(DEFMSG_ITEMUSE_DYE_REACH);
				return false;
			}
			if ( pCharTarg )
			{
				// Dye hair
				pObjTarg = pCharTarg->LayerFind(LAYER_HAIR);
				if ( pObjTarg )
				{
					pObjTarg->SetHue(pItemUse->GetHue(), false, GetChar(), pItemUse, 0x23e);
					pObjTarg->Update();
				}
				pObjTarg = pCharTarg->LayerFind(LAYER_BEARD);
				if ( !pObjTarg )
					return true;
			}
			else
			{
				if ( !m_pChar->CanUse(pItemTarg, false) )
					return false;
				if ( !IsPriv(PRIV_GM) && !pItemTarg->Item_GetDef()->Can(CAN_I_DYE) && !pItemTarg->IsType(IT_CLOTHING) )
				{
					SysMessageDefault(DEFMSG_ITEMUSE_DYE_FAIL);
					return false;
				}
			}

			pObjTarg->SetHue(pItemUse->GetHue(), false, GetChar(), pItemUse, 0x23e);
			pObjTarg->Update();
			return true;
		}
		case IT_PITCHER_EMPTY:
		{
			// Use on water
			switch ( m_pChar->CanTouchStatic(pt, id, pItemTarg) )
			{
				case IT_JUNK:
				{
					SysMessageDefault(DEFMSG_ITEMUSE_PITCHER_REACH);
					return false;
				}
				case IT_WATER:
				case IT_WATER_WASH:
				{
					pItemUse->SetID(ITEMID_PITCHER_WATER);
					pItemUse->Update();
					return true;
				}
				default:
				{
					SysMessageDefault(DEFMSG_ITEMUSE_PITCHER_FILL);
					return false;
				}
			}
			break;
		}
		case IT_SEWING_KIT:
		{
			// Use on cloth or hides
			if ( !pItemTarg )
				break;
			if ( !m_pChar->CanUse(pItemTarg, true) )
				return false;

			switch ( pItemTarg->GetType() )
			{
				case IT_LEATHER:
				case IT_HIDE:
				{
					if ( IsTrigUsed(TRIGGER_SKILLMENU) )
					{
						CScriptTriggerArgs args("sm_tailor_leather");
						if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
							return true;
					}
					return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_tailor_leather"));
				}
				case IT_CLOTH:
				{
					if ( IsTrigUsed(TRIGGER_SKILLMENU) )
					{
						CScriptTriggerArgs args("sm_tailor_cloth");
						if ( m_pChar->OnTrigger("@SkillMenu", m_pChar, &args) == TRIGRET_RET_TRUE )
							return true;
					}
					return Cmd_Skill_Menu(g_Cfg.ResourceGetIDType(RES_SKILLMENU, "sm_tailor_cloth"));
				}
				default:
					break;
			}
			SysMessageDefault(DEFMSG_ITEMUSE_SKIT_UNABLE);
			return false;
		}
		default:
			break;
	}
	SysMessageDefault(DEFMSG_ITEMUSE_UNABLE);
	return false;
}

void CClient::OnTarg_Stone_Recruit(CChar *pChar, bool fFull)
{
	ADDTOCALLSTACK("CClient::OnTarg_Stone_Recruit");
	// CLIMODE_TARG_STONE_RECRUIT / CLIMODE_TARG_STONE_RECRUITFULL

	CItemStone *pStone = dynamic_cast<CItemStone *>(m_Targ_UID.ItemFind());
	if ( pStone )
		pStone->AddRecruit(pChar, STONEPRIV_CANDIDATE, fFull);
}

void CClient::OnTarg_Party_Add(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::OnTarg_Party_Add");
	// CLIMODE_TARG_PARTY_ADD
	// Invite this char to the party

	if ( !pChar )
	{
		SysMessageDefault(DEFMSG_PARTY_TARG_ADD_ITEM);
		return;
	}
	if ( pChar == m_pChar )
	{
		SysMessageDefault(DEFMSG_PARTY_TARG_ADD_SELF);
		return;
	}
	if ( !pChar->m_pClient )
	{
		SysMessageDefault(DEFMSG_PARTY_TARG_ADD_NPC);
		return;
	}

	if ( m_pChar->m_pParty )
	{
		if ( !m_pChar->m_pParty->IsPartyMaster(m_pChar) )
		{
			SysMessageDefault(DEFMSG_PARTY_TARG_ADD_PERMISSION);
			return;
		}
		if ( m_pChar->m_pParty->IsPartyFull() )
		{
			SysMessageDefault(DEFMSG_PARTY_TARG_ADD_FULL);
			return;
		}
	}

	if ( IsPriv(PRIV_GM) && (GetPrivLevel() >= pChar->m_pClient->GetPrivLevel()) )
	{
		CPartyDef::AcceptEvent(pChar, m_pChar, true);
		return;
	}

	if ( pChar->m_pParty )
	{
		if ( m_pChar->m_pParty == pChar->m_pParty )
		{
			SysMessageDefault(DEFMSG_PARTY_ALREADY_YOUR);
			return;
		}
		else
		{
			SysMessageDefault(DEFMSG_PARTY_ALREADY_OTHER);
			return;
		}
	}

	if ( pChar->GetKeyNum("PARTY_AUTODECLINEINVITE") )
	{
		SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_DECLINE), pChar->GetName());
		return;
	}

	CVarDefCont *pTagInvitetime = m_pChar->m_TagDefs.GetKey("PARTY_LASTINVITETIME");
	if ( pTagInvitetime && (static_cast<UINT64>(pTagInvitetime->GetValNum()) + (5 * TICK_PER_SEC) > g_World.GetCurrentTime().GetTimeRaw()) )
	{
		SysMessageDefault(DEFMSG_PARTY_TARG_ADD_WAIT);
		return;
	}

	if ( IsTrigUsed(TRIGGER_PARTYINVITE) )
	{
		CScriptTriggerArgs args;
		if ( pChar->OnTrigger(CTRIG_PartyInvite, m_pChar, &args) == TRIGRET_RET_TRUE )
			return;
	}

	m_pChar->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_TARG_ADD_SUCCESS));
	pChar->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_PARTY_INVITED_PROMPT), m_pChar->GetName());

	m_pChar->SetKeyNum("PARTY_LASTINVITE", pChar->GetUID());
	m_pChar->SetKeyNum("PARTY_LASTINVITETIME", g_World.GetCurrentTime().GetTimeRaw());

	new PacketPartyInvite(pChar->m_pClient, m_pChar);
}

void CClient::OnTarg_Party_Remove(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::OnTarg_Party_Remove");
	// CLIMODE_TARG_PARTY_REMOVE
	// Remove this char from the party

	if ( !pChar || !pChar->m_pParty || !m_pChar || !m_pChar->m_pParty || !m_pChar->m_pParty->IsPartyMaster(m_pChar) )
		return;

	m_pChar->m_pParty->RemoveMember(pChar, m_pChar);
}

void CClient::OnTarg_GlobalChat_Add(CChar *pChar)
{
	ADDTOCALLSTACK("CClient::OnTarg_GlobalChat_Add");
	// CLIMODE_TARG_GLOBALCHAT_ADD
	// Invite this person to join our global chat friend list

	if ( !CGlobalChat::IsVisible() )
	{
		SysMessage("You must enable Global Chat to request a friend.");
		return;
	}
	if ( !pChar || !pChar->m_pPlayer || (pChar == m_pChar) )
	{
		SysMessage("Invalid target.");
		return;
	}
	if ( !pChar->m_pClient )
	{
		SysMessage("Player currently unavailable.");
		return;
	}
	if ( pChar->m_pPlayer->m_fRefuseGlobalChatRequests )		// TO-DO: also check if pChar is online on global chat -> CGlobalChat::IsVisible()
	{
		SysMessage("This user is not accepting Global Chat friend requests at this time.");
		return;
	}
	/*if ( iFriendsCount >= 50 )		// TO-DO
	{
		SysMessage("You have reached your global chat friend limit.");
		return;
	}*/

	if ( IsPriv(PRIV_GM) && (pChar->m_pClient->GetPrivLevel() < GetPrivLevel()) )
	{
		// TO-DO: auto-accept the request without send 'friend request' dialog
		return;
	}

	CVarDefCont *pTagInviteTime = m_pChar->m_TagDefs.GetKey("GLOBALCHAT_LASTINVITETIME");
	if ( pTagInviteTime && (g_World.GetCurrentTime().GetTimeRaw() < static_cast<UINT64>(pTagInviteTime->GetValNum())) )
	{
		SysMessage("You are unable to add new friends at this time. Please try again in a moment.");
		return;
	}
	m_pChar->SetKeyNum("GLOBALCHAT_LASTINVITETIME", g_World.GetCurrentTime().GetTimeRaw() + (30 * TICK_PER_SEC));

	// TO-DO: send 'friend request' dialog
}
