//
// CContain.cpp
//
#include "graysvr.h"	// predef header.
#include "../network/send.h"

///////////////////////////////////////////////////////////
// CContainer

void CContainer::OnWeightChange(int iChange)
{
	ADDTOCALLSTACK("CContainer::OnWeightChange");
	// Propagate the weight change up the stack if there is one
	m_totalweight += iChange;
}

int CContainer::FixWeight()
{
	ADDTOCALLSTACK("CContainer::FixWeight");
	// If there is some sort of ASSERT during item add then this is used to fix it
	m_totalweight = 0;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )
		{
			pCont->FixWeight();
			if ( !pCont->IsWeighed() )
				continue;
		}
		m_totalweight += pItem->GetWeight();
	}
	return m_totalweight;
}

void CContainer::ContentAddPrivate(CItem *pItem)
{
	ADDTOCALLSTACK("CContainer::ContentAddPrivate");
	// We are adding to a CChar or a CItemContainer
	ASSERT(pItem);
	ASSERT(pItem->IsValidUID());	// it should be valid at this point
	if ( pItem->GetParent() == this )
		return;
	if ( !CGObList::GetCount() )
		CGObList::InsertHead(pItem);
	else
	{
		CItem *pTest = GetContentHead();
		CItem *pPrevItem = pTest;
		for ( ; pTest != NULL; pTest = pTest->GetNext() )
		{
			if ( pTest->GetUID() < pPrevItem->GetUID() )
				pPrevItem = pTest;
		}
		CGObList::InsertAfter(pItem, pPrevItem);
	}
	//CGObList::InsertTail(pItem);		// reversing the order in which things are added into a container
	OnWeightChange(pItem->GetWeight());
}

void CContainer::OnRemoveOb(CGObListRec *pObRec)	// override this = called when removed from list
{
	ADDTOCALLSTACK("CContainer::OnRemoveOb");
	// Remove this object from the container list
	// Overload the RemoveAt for general lists to come here
	CItem *pItem = static_cast<CItem *>(pObRec);
	ASSERT(pItem);

	CGObList::OnRemoveOb(pItem);
	ASSERT(!pItem->GetParent());

	pItem->SetContainerFlags(UID_O_DISCONNECT);		// it is no place for the moment
	OnWeightChange(-pItem->GetWeight());
}

void CContainer::r_WriteContent(CScript &s) const
{
	ADDTOCALLSTACK("CContainer::r_WriteContent");
	ASSERT(dynamic_cast<const CGObList *>(this) != NULL);

	// Write out all the items in me
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		ASSERT(pItem->GetParent() == this);
		pItem->r_WriteSafe(s);
	}
}

CItem *CContainer::ContentFind(RESOURCE_ID_BASE rid, DWORD dwArg, int iDecendLevels) const
{
	ADDTOCALLSTACK("CContainer::ContentFind");
	// Send all the items in the container

	if ( rid.GetResIndex() == 0 )
		return NULL;

	CItem *pItem = GetContentHead();
	for ( ; pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsResourceMatch(rid, dwArg) )
			break;
		if ( iDecendLevels <= 0 )
			continue;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )
		{
			if ( !pCont->IsSearchable() )
				continue;
			CItem *pItemInCont = pCont->ContentFind(rid, dwArg, iDecendLevels - 1);
			if ( pItemInCont )
				return pItemInCont;
		}
	}
	return pItem;
}

TRIGRET_TYPE CContainer::OnContTriggerForLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, RESOURCE_ID_BASE rid, DWORD dwArg, int iDecendLevels)
{
	ADDTOCALLSTACK("CContainer::OnContTriggerForLoop");
	if ( rid.GetResIndex() != 0 )
	{
		CItem *pItemNext = NULL;
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( pItem->IsResourceMatch(rid, dwArg) )
			{
				s.SeekContext(StartContext);
				TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
			}
			if ( iDecendLevels <= 0 )
				continue;

			CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
			if ( pCont )
			{
				if ( pCont->IsSearchable() )
				{
					CContainer *pContBase = dynamic_cast<CContainer *>(pCont);
					TRIGRET_TYPE iRet = pContBase->OnContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, rid, dwArg, iDecendLevels - 1);
					if ( iRet != TRIGRET_ENDIF )
						return iRet;

					// Since the previous call has already found the EndContext, set it
					EndContext = s.GetContext();
				}
			}
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		CScriptObj *pScript = dynamic_cast<CScriptObj *>(this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);
	return TRIGRET_ENDIF;
}

TRIGRET_TYPE CContainer::OnGenericContTriggerForLoop(CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, int iDecendLevels)
{
	ADDTOCALLSTACK("CContainer::OnGenericContTriggerForLoop");
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		s.SeekContext(StartContext);
		TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
		if ( iRet == TRIGRET_BREAK )
		{
			EndContext = StartContext;
			break;
		}
		if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
			return iRet;
		if ( iRet == TRIGRET_CONTINUE )
			EndContext = StartContext;
		else
			EndContext = s.GetContext();
		if ( iDecendLevels <= 0 )
			continue;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont && pCont->IsSearchable() )
		{
			CContainer *pContBase = dynamic_cast<CContainer *>(pCont);
			iRet = pContBase->OnGenericContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, iDecendLevels - 1);
			if ( iRet != TRIGRET_ENDIF )
				return iRet;

			// Since the previous call has already found the EndContext, set it
			EndContext = s.GetContext();
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		CScriptObj *pScript = dynamic_cast<CScriptObj *>(this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext(EndContext);
	return TRIGRET_ENDIF;
}

bool CContainer::ContentFindKeyFor(CItem *pLocked) const
{
	ADDTOCALLSTACK("CContainer::ContentFindKeyFor");
	// Look for the key that fits this in my possesion.
	return (pLocked && pLocked->m_itContainer.m_lockUID && ContentFind(RESOURCE_ID(RES_TYPEDEF, IT_KEY), pLocked->m_itContainer.m_lockUID));
}

DWORD CContainer::ContentConsume(RESOURCE_ID_BASE rid, DWORD dwQty, bool fTest, DWORD dwArg)
{
	ADDTOCALLSTACK("CContainer::ContentConsume");
	// ARGS:
	//	dwArg = a hack for ores
	// RETURN:
	//	0 = all consumed ok
	//	# = number left to be consumed (still required)

	if ( rid.GetResIndex() == 0 )
		return dwQty;	// from skills menus

	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsResourceMatch(rid, dwArg) )
		{
			dwQty -= pItem->ConsumeAmount(dwQty, fTest);
			if ( dwQty <= 0 )
				break;
		}

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )	// this is a sub-container
		{
			if ( rid == RESOURCE_ID(RES_TYPEDEF, IT_GOLD) )
			{
				if ( pCont->IsType(IT_CONTAINER_LOCKED) )
					continue;
			}
			else
			{
				if ( !pCont->IsSearchable() )
					continue;
			}
			dwQty = pCont->ContentConsume(rid, dwQty, fTest, dwArg);
			if ( dwQty <= 0 )
				break;
		}
	}
	return dwQty;
}

DWORD CContainer::ContentCount(RESOURCE_ID_BASE rid, DWORD dwArg)
{
	ADDTOCALLSTACK("CContainer::ContentCount");
	// Calculate total (gold or other items) in this recursed container
	return DWORD_MAX - ContentConsume(rid, DWORD_MAX, true, dwArg);
}

void CContainer::ContentAttrMod(DWORD dwAttr, bool fSet)
{
	ADDTOCALLSTACK("CContainer::ContentAttrMod");
	// Mark the attr
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( fSet )
			pItem->SetAttr(dwAttr);
		else
			pItem->ClrAttr(dwAttr);

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )	// this is a sub-container
			pCont->ContentAttrMod(dwAttr, fSet);
	}
}

void CContainer::ContentNotifyDelete()
{
	ADDTOCALLSTACK("CContainer::ContentNotifyDelete");
	if ( !IsTrigUsed(TRIGGER_DESTROY) )		// no point entering this loop if the trigger is disabled
		return;

	// Trigger @Destroy on contained items
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !pItem->NotifyDelete() )
		{
			// Item shouldn't be destroyed and shouldn't remain in this container, so drop it on ground if it hasn't been moved already
			if ( pItem->GetParent() == this )
				pItem->MoveToCheck(pItem->GetTopLevelObj()->GetTopPoint());
		}
	}
}

void CContainer::ContentsDump(const CPointMap &pt, DWORD dwAttrLeave)
{
	ADDTOCALLSTACK("CContainer::ContentsDump");
	// Just dump the contents onto the ground
	dwAttrLeave |= (ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_BLESSED|ATTR_INSURED);
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsAttr(dwAttrLeave) )	// hair and newbie stuff
			continue;
		// ??? scatter a little ?
		pItem->MoveToCheck(pt);
	}
}

void CContainer::ContentsTransfer(CItemContainer *pCont, bool fNoNewbie)
{
	ADDTOCALLSTACK("CContainer::ContentsTransfer");
	// Move all contents to another container
	if ( !pCont )
		return;

	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( fNoNewbie && pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_BLESSED|ATTR_INSURED) )
		{
			if ( pItem->IsAttr(ATTR_INSURED) )
			{
				pItem->ClrAttr(ATTR_INSURED);
				pItem->UpdatePropertyFlag();
			}
			continue;
		}
		pCont->ContentAdd(pItem);
	}
}

size_t CContainer::ResourceConsumePart(const CResourceQtyArray *pResources, DWORD dwReplicationQty, int iDamagePercent, bool fTest, DWORD dwArg)
{
	ADDTOCALLSTACK("CContainer::ResourceConsumePart");
	// Consume just some of the resources
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product
	//	iDamagePercent = 0-100
	// RETURN:
	//	BadIndex = all needed items where present
	//	Index of the item we did not have

	if ( iDamagePercent <= 0 )
		return pResources->BadIndex();

	size_t iMissing = pResources->BadIndex();
	size_t iQtyRes = pResources->GetCount();
	for ( size_t i = 0; i < iQtyRes; ++i )
	{
		DWORD dwResQty = static_cast<DWORD>(pResources->GetAt(i).GetResQty());
		if ( dwResQty <= 0 ) // not sure why this would be true
			continue;

		DWORD dwQtyTotal = dwResQty * dwReplicationQty;
		if ( dwQtyTotal <= 0 )
			continue;
		dwQtyTotal = IMULDIV(dwQtyTotal, iDamagePercent, 100);
		if ( dwQtyTotal <= 0 )
			continue;

		if ( ContentConsume(pResources->GetAt(i).GetResourceID(), dwQtyTotal, fTest, dwArg) )
			iMissing = i;
	}

	return iMissing;
}

DWORD CContainer::ResourceConsume(const CResourceQtyArray *pResources, DWORD dwReplicationQty, bool fTest, DWORD dwArg)
{
	ADDTOCALLSTACK("CContainer::ResourceConsume");
	// Consume or test all the required resources
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product
	// RETURN:
	//	How many whole objects can be made

	if ( dwReplicationQty < 1 )
		dwReplicationQty = 1;
	if ( !fTest && (dwReplicationQty > 1) )
	{
		// Test what the max number we can really make is first (all resources must be consumed with the same number)
		dwReplicationQty = ResourceConsume(pResources, dwReplicationQty, true, dwArg);
	}

	DWORD dwQtyMin = DWORD_MAX;
	for ( size_t i = 0; i < pResources->GetCount(); ++i )
	{
		DWORD dwResQty = static_cast<DWORD>(pResources->GetAt(i).GetResQty());
		if ( dwResQty <= 0 )	// not sure why this would be true
			continue;

		DWORD dwQtyTotal = dwResQty * dwReplicationQty;
		RESOURCE_ID rid = pResources->GetAt(i).GetResourceID();
		if ( rid.GetResType() == RES_SKILL )
		{
			CChar *pChar = dynamic_cast<CChar *>(this);
			if ( !pChar )
				continue;
			if ( pChar->Skill_GetBase(static_cast<SKILL_TYPE>(rid.GetResIndex())) < dwResQty )
				return 0;
			continue;
		}

		DWORD dwQtyCur = (dwQtyTotal - ContentConsume(rid, dwQtyTotal, fTest, dwArg)) / dwResQty;
		if ( dwQtyCur < dwQtyMin )
			dwQtyMin = dwQtyCur;
	}

	if ( dwQtyMin == DWORD_MAX )	// it has no resources, so I guess we can make it from nothing?
		return dwReplicationQty;

	return dwQtyMin;
}

DWORD CContainer::ContentCountAll() const
{
	ADDTOCALLSTACK("CContainer::ContentCountAll");
	// RETURN:
	//	A count of all the items in this container and sub containers
	DWORD dwTotal = 0;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		++dwTotal;
		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( !pCont )
			continue;
		dwTotal += pCont->ContentCountAll();
	}
	return dwTotal;
}

bool CContainer::r_GetRefContainer(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CContainer::r_GetRefContainer");
	if ( !strnicmp(pszKey, "FIND", 4) )				// FIND*
	{
		pszKey += 4;
		if ( !strnicmp(pszKey, "ID", 2) )			// FINDID
		{
			pszKey += 2;
			SKIP_SEPARATORS(pszKey);
			pRef = ContentFind(g_Cfg.ResourceGetID(RES_ITEMDEF, pszKey));
			SKIP_SEPARATORS(pszKey);
			return true;
		}
		else if ( !strnicmp(pszKey, "CONT", 4) )	// FINDCONT
		{
			pszKey += 4;
			SKIP_SEPARATORS(pszKey);
			INT64 iKey = Exp_GetLLSingle(pszKey);
			pRef = (iKey >= 0) ? GetAt(static_cast<size_t>(iKey)) : NULL;
			SKIP_SEPARATORS(pszKey);
			return true;
		}
		else if ( !strnicmp(pszKey, "TYPE", 4) )	// FINDTYPE
		{
			pszKey += 4;
			SKIP_SEPARATORS(pszKey);
			pRef = ContentFind(g_Cfg.ResourceGetID(RES_TYPEDEF, pszKey));
			SKIP_SEPARATORS(pszKey);
			return true;
		}
	}
	return false;
}

bool CContainer::r_WriteValContainer(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CContainer::r_WriteValContainer");
	EXC_TRY("WriteVal");

	static LPCTSTR const sm_szParams[] =
	{
		"COUNT",
		"FCOUNT",
		"RESCOUNT",
		"RESTEST"
	};

	int i = FindTableHeadSorted(pszKey, sm_szParams, COUNTOF(sm_szParams));
	if ( i < 0 )
		return false;

	LPCTSTR	pszTemp = pszKey + strlen(sm_szParams[i]);
	SKIP_SEPARATORS(pszTemp);
	switch ( i )
	{
		case 0:		// COUNT
		{
			WORD wTotal = 0;
			for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
				++wTotal;

			sVal.FormatUVal(wTotal);
			break;
		}
		case 1:		// FCOUNT
		{
			sVal.FormatUVal(ContentCountAll());
			break;
		}
		case 2:		// RESCOUNT
		{
			sVal.FormatUVal(*pszTemp ? ContentCount(g_Cfg.ResourceGetID(RES_ITEMDEF, pszTemp)) : GetCount());
			break;
		}
		case 3:		// RESTEST
		{
			CResourceQtyArray Resources;
			sVal.FormatUVal(Resources.Load(pszTemp) ? ResourceConsume(&Resources, 1, true) : 0);
			break;
		}
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

///////////////////////////////////////////////////////////
// CItemContainer

bool CItemContainer::NotifyDelete()
{
	ADDTOCALLSTACK("CItemContainer::NotifyDelete");
	// Notify destruction of the container before its contents
	if ( !CItem::NotifyDelete() )
		return false;

	// Ensure trade contents are moved out
	if ( IsType(IT_EQ_TRADE_WINDOW) )
		Trade_Delete();

	ContentNotifyDelete();
	return true;
}

void CItemContainer::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemContainer::r_Write");
	CItemVendable::r_Write(s);
	r_WriteContent(s);
}

bool CItemContainer::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CItemContainer::r_GetRef");
	if ( r_GetRefContainer(pszKey, pRef) )
		return true;
	return CItemVendable::r_GetRef(pszKey, pRef);
}

bool CItemContainer::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemContainer::r_WriteVal");
	if ( r_WriteValContainer(pszKey, sVal, pSrc) )
		return true;
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemContainer::IsItemInTrade() const
{
	ADDTOCALLSTACK("CItemContainer::IsItemInTrade");
	// Recursively get the item on "top" level
	CItemContainer *pParent = dynamic_cast<CItemContainer *>(GetParentObj());
	if ( pParent )
		return pParent->IsItemInTrade();
	return IsType(IT_EQ_TRADE_WINDOW);
}

void CItemContainer::Trade_Status(bool fCheck)
{
	ADDTOCALLSTACK("CItemContainer::Trade_Status");
	// Update trade status check boxes to both sides
	CItemContainer *pPartner = dynamic_cast<CItemContainer *>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;

	CChar *pChar1 = dynamic_cast<CChar *>(GetParent());
	if ( !pChar1 || !pChar1->m_pClient )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->m_pClient )
		return;

	m_itEqTradeWindow.m_bCheck = fCheck ? 1 : 0;
	if ( !fCheck )
		pPartner->m_itEqTradeWindow.m_bCheck = 0;

	PacketTradeAction cmd(SECURETRADE_Accept);
	cmd.prepareReadyChange(this, pPartner);
	cmd.send(pChar1->m_pClient);

	cmd.prepareReadyChange(pPartner, this);
	cmd.send(pChar2->m_pClient);

	// Check if both clients had pressed the 'accept' buttom
	if ( (pPartner->m_itEqTradeWindow.m_bCheck == 0) || (m_itEqTradeWindow.m_bCheck == 0) )
		return;

	if ( IsTrigUsed(TRIGGER_TRADEACCEPTED) || IsTrigUsed(TRIGGER_CHARTRADEACCEPTED) )
	{
		CScriptTriggerArgs Args1(pChar1);
		WORD i = 1;
		for ( CItem *pItem = pPartner->GetContentHead(); pItem != NULL; pItem = pItem->GetNext(), ++i )
			Args1.m_VarObjs.Insert(i, pItem, true);
		Args1.m_iN1 = --i;

		CScriptTriggerArgs Args2(pChar2);
		i = 1;
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext(), ++i )
			Args2.m_VarObjs.Insert(i, pItem, true);
		Args2.m_iN2 = --i;

		Args1.m_iN2 = Args2.m_iN2;
		Args2.m_iN1 = Args1.m_iN1;
		if ( (pChar1->OnTrigger(CTRIG_TradeAccepted, pChar2, &Args1) == TRIGRET_RET_TRUE) || (pChar2->OnTrigger(CTRIG_TradeAccepted, pChar1, &Args2) == TRIGRET_RET_TRUE) )
			return;
	}

	// Transfer items
	CItem *pItemNext = NULL;
	for ( CItem *pItem = pPartner->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar1->ItemBounce(pItem, false);
	}

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar2->ItemBounce(pItem, false);
	}

	// Transfer gold/platinum
	if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
	{
		if ( m_itEqTradeWindow.m_iPlatinum && m_itEqTradeWindow.m_iGold )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT_GOLD), m_itEqTradeWindow.m_iPlatinum, m_itEqTradeWindow.m_iGold, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT_GOLD), m_itEqTradeWindow.m_iPlatinum, m_itEqTradeWindow.m_iGold, pChar1->GetName());
		}
		else if ( m_itEqTradeWindow.m_iPlatinum )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT), m_itEqTradeWindow.m_iPlatinum, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT), m_itEqTradeWindow.m_iPlatinum, pChar1->GetName());
		}
		else if ( m_itEqTradeWindow.m_iGold )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_GOLD), m_itEqTradeWindow.m_iGold, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_GOLD), m_itEqTradeWindow.m_iGold, pChar1->GetName());
		}

		if ( pPartner->m_itEqTradeWindow.m_iPlatinum && pPartner->m_itEqTradeWindow.m_iGold )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT_GOLD), pPartner->m_itEqTradeWindow.m_iPlatinum, pPartner->m_itEqTradeWindow.m_iGold, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT_GOLD), pPartner->m_itEqTradeWindow.m_iPlatinum, pPartner->m_itEqTradeWindow.m_iGold, pChar2->GetName());
		}
		else if ( pPartner->m_itEqTradeWindow.m_iPlatinum )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT), pPartner->m_itEqTradeWindow.m_iPlatinum, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT), pPartner->m_itEqTradeWindow.m_iPlatinum, pChar2->GetName());
		}
		else if ( pPartner->m_itEqTradeWindow.m_iGold )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_GOLD), pPartner->m_itEqTradeWindow.m_iGold, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_GOLD), pPartner->m_itEqTradeWindow.m_iGold, pChar2->GetName());
		}

		UINT64 uGold1 = static_cast<UINT64>(m_itEqTradeWindow.m_iGold) + static_cast<UINT64>(m_itEqTradeWindow.m_iPlatinum) * 1000000000;
		UINT64 uGold2 = static_cast<UINT64>(pPartner->m_itEqTradeWindow.m_iGold) + static_cast<UINT64>(pPartner->m_itEqTradeWindow.m_iPlatinum) * 1000000000;
		pChar1->m_virtualGold += uGold2 - uGold1;
		pChar2->m_virtualGold += uGold1 - uGold2;
	}

	// Done with trade
	Delete();
}

void CItemContainer::Trade_UpdateGold(DWORD dwPlatinum, DWORD dwGold)
{
	ADDTOCALLSTACK("CItemContainer::Trade_UpdateGold");
	// Update trade gold/platinum values on TOL clients
	CItemContainer *pPartner = dynamic_cast<CItemContainer *>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;
	CChar *pChar1 = dynamic_cast<CChar *>(GetParent());
	if ( !pChar1 || !pChar1->m_pClient )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->m_pClient )
		return;

	bool fUpdateChar1 = false;
	bool fUpdateChar2 = pChar2->m_pClient->m_NetState->isClientVersion(MINCLIVER_TOL);

	// To prevent cheating, check if the char really have these gold/platinum values
	UINT64 uMax = pChar1->m_virtualGold;
	if ( static_cast<UINT64>(dwGold) + static_cast<UINT64>(dwPlatinum) * 1000000000 > uMax )
	{
		dwGold = static_cast<DWORD>(uMax % 1000000000);
		dwPlatinum = static_cast<DWORD>(uMax / 1000000000);
		fUpdateChar1 = true;
	}

	m_itEqTradeWindow.m_iGold = dwGold;
	m_itEqTradeWindow.m_iPlatinum = dwPlatinum;

	PacketTradeAction cmd(SECURETRADE_UpdateGold);
	cmd.prepareUpdateGold(this, dwGold, dwPlatinum);
	if ( fUpdateChar1 )
		cmd.send(pChar1->m_pClient);
	if ( fUpdateChar2 )
		cmd.send(pChar2->m_pClient);
}

void CItemContainer::Trade_Delete()
{
	ADDTOCALLSTACK("CItemContainer::Trade_Delete");
	// Called when object deleted
	ASSERT(IsType(IT_EQ_TRADE_WINDOW));

	CChar *pChar = dynamic_cast<CChar *>(GetParent());
	if ( !pChar )
		return;

	if ( pChar->m_pClient )
	{
		// Send the cancel trade message
		PacketTradeAction cmd(SECURETRADE_Close);
		cmd.prepareClose(this);
		cmd.send(pChar->m_pClient);
	}

	// Drop items back in my pack
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar->ItemBounce(pItem, false);
	}

	// Kill my trading partner
	CItemContainer *pPartner = dynamic_cast<CItemContainer *>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;

	if ( IsTrigUsed(TRIGGER_TRADECLOSE) )
	{
		CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
		CScriptTriggerArgs Args(pChar2);
		pChar->OnTrigger(CTRIG_TradeClose, pChar, &Args);
		CScriptTriggerArgs Args2(pChar);
		pChar2->OnTrigger(CTRIG_TradeClose, pChar, &Args2);
	}

	m_uidLink.InitUID();	// unlink
	pPartner->m_uidLink.InitUID();
	pPartner->Delete();
}

void CItemContainer::OnWeightChange(int iChange)
{
	ADDTOCALLSTACK("CItemContainer::OnWeightChange");
	if ( !iChange )
		return;

	CContainer::OnWeightChange(iChange);
	UpdatePropertyFlag();

	if ( !IsWeighed() )
		return;

	// Propagate the weight change to parent container
	CContainer *pCont = dynamic_cast<CContainer *>(GetParent());
	if ( pCont )
		pCont->OnWeightChange(iChange);
}

CPointMap CItemContainer::GetRandContainerLoc() const
{
	ADDTOCALLSTACK("CItemContainer::GetRandContainerLoc");
	// Get a random location inside container rect
	CItemBase *pItemDef = Item_GetDef();
	GUMP_TYPE gump = pItemDef->GetContainerGumpID();

	// Check for custom values in TDATA3/TDATA4
	if ( pItemDef->m_ttContainer.m_dwMaxXY )
	{
		int iMinX = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF0000) >> 16;
		int iMinY = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF);
		int iMaxX = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF0000) >> 16;
		int iMaxY = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF);
		return CPointMap(static_cast<WORD>(Calc_GetRandVal(iMinX, iMaxX)), static_cast<WORD>(Calc_GetRandVal(iMinY, iMaxY)));
	}

	static const struct		// we can probably get this from MUL file some place
	{
		GUMP_TYPE m_gump;
		WORD m_minX;
		WORD m_minY;
		WORD m_maxX;
		WORD m_maxY;
	}
	sm_ContainerRect[] =
	{
		{ GUMP_SCROLL, 30, 30, 270, 170 },
		{ GUMP_CORPSE, 20, 85, 124, 196 },
		{ GUMP_BACKPACK, 44, 65, 186, 159 },
		{ GUMP_BAG, 29, 34, 137, 128 },
		{ GUMP_BARREL, 33, 36, 142, 148 },
		{ GUMP_BASKET_SQUARE, 19, 47, 182, 123 },
		{ GUMP_BOX_WOOD, 16, 38, 152, 125 },
		{ GUMP_BASKET_ROUND, 35, 38, 145, 116 },
		{ GUMP_CHEST_METAL_GOLD, 18, 105, 162, 178 },
		{ GUMP_BOX_WOOD_ORNATE, 16, 51, 184, 124 },
		{ GUMP_CRATE, 20, 10, 170, 100 },
		{ GUMP_LEATHER, 16, 10, 148, 138 },
		{ GUMP_DRAWER_DARK, 16, 10, 154, 94 },
		{ GUMP_CHEST_WOOD, 18, 105, 162, 178 },
		{ GUMP_CHEST_METAL, 18, 105, 162, 178 },
		{ GUMP_BOX_METAL, 16, 51, 184, 124 },
		{ GUMP_SHIP_HATCH, 46, 74, 196, 184 },
		{ GUMP_BOOK_SHELF, 76, 12, 110, 68 },
		{ GUMP_CABINET_DARK, 24, 96, 96, 152 },
		{ GUMP_CABINET_LIGHT, 24, 96, 96, 152 },
		{ GUMP_DRAWER_LIGHT, 16, 10, 154, 94 },
		{ GUMP_BULLETIN_BOARD, 0, 0, 110, 62 },
		{ GUMP_GIFT_BOX, 35, 10, 190, 95 },
		{ GUMP_STOCKING, 41, 21, 186, 111 },
		{ GUMP_ARMOIRE_ELVEN_WASH, 10, 10, 170, 115 },
		{ GUMP_ARMOIRE_RED, 10, 10, 170, 115 },
		{ GUMP_ARMOIRE_MAPLE, 10, 10, 170, 115 },
		{ GUMP_ARMOIRE_CHERRY, 10, 10, 170, 115 },
		{ GUMP_BASKET_TALL, 10, 30, 170, 145 },
		{ GUMP_CHEST_WOOD_PLAIN, 10, 10, 170, 115 },
		{ GUMP_CHEST_WOOD_GILDED, 10, 10, 170, 115 },
		{ GUMP_CHEST_WOOD_ORNATE, 10, 10, 170, 115 },
		{ GUMP_TALL_CABINET, 10, 10, 170, 115 },
		{ GUMP_CHEST_WOOD_FINISH, 10, 10, 170, 115 },
		{ GUMP_CHEST_WOOD_FINISH2, 10, 10, 170, 115 },
		//{ GUMP_BLESSED_STATUE, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_MAILBOX, 0, 0, 0, 0 },				// TO-DO: confirm gump size
		{ GUMP_GIFT_BOX_CUBE, 23, 51, 163, 151 },
		{ GUMP_GIFT_BOX_CYLINDER, 16, 51, 156, 166 },
		{ GUMP_GIFT_BOX_OCTOGON, 25, 51, 165, 166 },
		{ GUMP_GIFT_BOX_RECTANGLE, 16, 51, 156, 151 },
		{ GUMP_GIFT_BOX_ANGEL, 21, 51, 161, 151 },
		{ GUMP_GIFT_BOX_HEART_SHAPED, 56, 30, 158, 104 },
		//{ GUMP_GIFT_BOX_TALL, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		{ GUMP_GIFT_BOX_CHRISTMAS, 16, 51, 156, 166 },
		//{ GUMP_WALL_SAFE, 0, 0, 0, 0 },			// TO-DO: confirm gump size
		//{ GUMP_CHEST_PIRATE, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_FOUNTAIN_LIFE, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_SECRET_CHEST, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_MAILBOX_DOLPHIN, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_MAILBOX_SQUIRREL, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		//{ GUMP_MAILBOX_BARREL, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_MAILBOX_LANTERN, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_CABINET_LIGHT_LARGE, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		//{ GUMP_CABINET_DARK_LARGE, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		//{ GUMP_DRAWER_LIGHT_LARGE, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		//{ GUMP_DRAWER_DARK_LARGE, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		//{ GUMP_BARREL_LARGE, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		//{ GUMP_BOOK_SHELF_LARGE, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		{ GUMP_SECURE_TRADE, 20, 30, 380, 180 },
		{ GUMP_BOARD_CHECKER, 0, 0, 282, 230 },
		{ GUMP_BOARD_BACKGAMMON, 0, 0, 282, 210 },
		{ GUMP_CHEST_WEDDING, 16, 51, 184, 124 },
		{ GUMP_STONE_BASE, 16, 51, 184, 124 },
		{ GUMP_PLAGUE_BEAST, 60, 33, 460, 348 },
		//{ GUMP_KING_COLLECTION_BOX, 0, 0, 0, 0 },	// TO-DO: confirm gump size
		{ GUMP_BACKPACK_SUEDE, 44, 65, 186, 159 },
		{ GUMP_BACKPACK_POLAR_BEAR, 44, 65, 186, 159 },
		{ GUMP_BACKPACK_GHOUL_SKIN, 44, 65, 186, 159 },
		//{ GUMP_GIFT_BOX_SQUARE, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		{ GUMP_CRATE_FLETCHING, 24, 96, 196, 152 },
		{ GUMP_CHEST_WOODEN, 10, 10, 170, 115 },
		//{ GUMP_PILLOW_HEART, 0, 0, 0, 0 },		// TO-DO: confirm gump size
		{ GUMP_CHEST_METAL_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_METAL_GOLD_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_WOOD_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_CRATE_LARGE, 50, 60, 500, 300 },
		{ GUMP_MINERS_SATCHEL, 44, 65, 186, 159 },
		{ GUMP_LUMBERJACKS_SATCHEL, 44, 65, 186, 159 },
		{ GUMP_SHIP_CANNON, 35, 38, 145, 116 },
		{ GUMP_CHEST_METAL2, 18, 105, 162, 178 }
	};

	for ( size_t i = 0; i < COUNTOF(sm_ContainerRect); ++i )
	{
		if ( gump != sm_ContainerRect[i].m_gump )
			continue;
		return CPointMap(static_cast<WORD>(Calc_GetRandVal(sm_ContainerRect[i].m_minX, sm_ContainerRect[i].m_maxX)), static_cast<WORD>(Calc_GetRandVal(sm_ContainerRect[i].m_minY, sm_ContainerRect[i].m_maxY)));
	}

	DEBUG_WARN(("Can't get rect of unknown container gump TDATA2=0%hx for item '%s'\n", gump, pItemDef->GetResourceName()));
	return CPointMap(static_cast<WORD>(Calc_GetRandVal(0, 200)), static_cast<WORD>(Calc_GetRandVal(0, 200)));
}

SOUND_TYPE CItemContainer::GetDropSound() const
{
	ADDTOCALLSTACK("CItemContainer::GetDropSound");
	switch ( Item_GetDef()->GetContainerGumpID() )
	{
		case GUMP_BACKPACK:
		case GUMP_BAG:
			return SOUND_LEATHER;
		case GUMP_BASKET_SQUARE:
		case GUMP_BASKET_ROUND:
		case GUMP_BASKET_TALL:
			return SOUND_RUSTLE;
		case GUMP_PLAGUE_BEAST:
			return SOUND_FISH_SPLASH;
		default:
			return SOUND_NONE;
	}
}

void CItemContainer::ContentAdd(CItem *pItem, CPointMap pt, BYTE gridIndex)
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	// Add to CItemContainer
	if ( !pItem )
		return;
	if ( pItem == this )
		return;		// infinite loop

	if ( !g_Serv.IsLoading() )
	{
		if ( IsType(IT_EQ_TRADE_WINDOW) )
			Trade_Status(false);	// drop into a trade window

		switch ( pItem->GetType() )
		{
			case IT_LIGHT_LIT:
			{
				// Douse the light in a pack (if possible)
				pItem->Use_Light();
				break;
			}
			case IT_GAME_BOARD:
			{
				// Can't be put into any sort of a container, delete all it's pieces
				CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
				if ( pCont )
					pCont->DeleteAll();
				break;
			}
			default:
				break;
		}
	}

	if ( (pt.m_x < 0) || (pt.m_x > 512) || (pt.m_y < 0) || (pt.m_y > 512) )		// invalid container location
	{
		bool fInsert = false;
		// Try to stack it
		if ( !g_Serv.IsLoading() && pItem->Item_GetDef()->IsStackableType() )
		{
			for ( CItem *pTry = GetContentHead(); pTry != NULL; pTry = pTry->GetNext() )
			{
				pt = pTry->GetContainedPoint();
				if ( pItem->Stack(pTry) )
				{
					fInsert = true;
					break;
				}
			}
		}
		if ( !fInsert )
			pt = GetRandContainerLoc();
	}

	// Try drop it on given container grid index (if not available, drop it on next free index)
	bool fGridAvailable;
	for ( size_t i = 0; i < BYTE_MAX; ++i )
	{
		fGridAvailable = true;
		for ( CItem *pTry = GetContentHead(); pTry != NULL; pTry = pTry->GetNext() )
		{
			if ( pTry->GetContainedGridIndex() == gridIndex )
			{
				fGridAvailable = false;
				break;
			}
		}
		if ( fGridAvailable )
			break;

		if ( ++gridIndex >= MAX_ITEMS_CONT )
			gridIndex = 0;
	}

	CContainer::ContentAddPrivate(pItem);
	pItem->SetContainedPoint(pt);
	pItem->SetContainedGridIndex(gridIndex);

	// If an item needs OnTickStatusUpdate called on the next tick, it needs to be added to a separate list since it won't receive ticks whilst in this container
	if ( pItem->m_fStatusUpdate && !g_World.m_ObjStatusUpdates.ContainsPtr(pItem) )
		g_World.m_ObjStatusUpdates.Add(pItem);

	switch ( GetType() )
	{
		case IT_KEYRING:
		{
			SetKeyRing();
			break;
		}
		case IT_EQ_VENDOR_BOX:
		{
			if ( !IsItemEquipped() )
			{
				DEBUG_ERR(("Un-equipped vendor box UID=0%" FMTDWORDH " is bad\n", GetUID().GetObjUID()));
				break;
			}
			CItemVendable *pItemVend = static_cast<CItemVendable *>(pItem);
			if ( !pItemVend )
			{
				DEBUG_ERR(("Vendor UID=0%" FMTDWORDH " selling non-vendable item '%s'\n", GetParentObj()->GetUID().GetObjUID(), pItem->GetResourceName()));
				pItem->Delete();
				break;
			}
			pItemVend->SetBasePrice(0);		// unpriced yet
			pItemVend->SetContainedLayer(static_cast<BYTE>(pItem->GetAmount()));
			break;
		}
		case IT_GAME_BOARD:
		{
			// Can only place IT_GAME_PIECE inside here
			if ( pItem->IsType(IT_GAME_PIECE) )
				break;
			g_Log.Event(LOGL_WARN, "Game board contains invalid item: %s UID=0%" FMTDWORDH ", board: %s UID=0%" FMTDWORDH "\n", pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetResourceName(), GetUID().GetObjUID());
			pItem->Delete();
			break;
		}
		default:
			break;
	}

	switch ( pItem->GetID() )
	{
		case ITEMID_BEDROLL_O_EW:
		case ITEMID_BEDROLL_O_NS:
			pItem->SetDispID(ITEMID_BEDROLL_C);		// close the bedroll
			break;
		default:
			break;
	}

	pItem->Update();
	UpdatePropertyFlag();
}

void CItemContainer::ContentAdd(CItem *pItem)
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	if ( !pItem )
		return;
	if ( pItem->GetParent() == this )
		return;		// already here

	CPointMap pt;
	if ( g_Serv.IsLoading() )
		pt = pItem->GetTopPoint();

	ContentAdd(pItem, pt);
}

bool CItemContainer::IsItemInside(const CItem *pItem) const
{
	ADDTOCALLSTACK("CItemContainer::IsItemInside");
	// Checks if a particular item is in a container or one of it's subcontainers
	for (;;)
	{
		if ( !pItem )
			return false;
		if ( pItem == this )
			return true;
		pItem = dynamic_cast<CItemContainer *>(pItem->GetParent());
	}
}

void CItemContainer::OnRemoveOb(CGObListRec *pObRec)	// override this = called when removed from list
{
	ADDTOCALLSTACK("CItemContainer::OnRemoveOb");
	// Remove this object from the container list
	CItem *pItem = static_cast<CItem *>(pObRec);
	ASSERT(pItem);
	if ( IsType(IT_EQ_TRADE_WINDOW) )
	{
		Trade_Status(false);
		m_itEqTradeWindow.m_iWaitTime = g_World.GetCurrentTime().GetTimeRaw() + (10 * TICK_PER_SEC);
	}
	if ( IsType(IT_EQ_VENDOR_BOX) && IsItemEquipped() )		// vendor boxes should ALWAYS be equipped
	{
		CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
		if ( pItemVend )
			pItemVend->SetBasePrice(0);
	}

	CContainer::OnRemoveOb(pObRec);
	UpdatePropertyFlag();
	if ( IsType(IT_KEYRING) )
		SetKeyRing();
}

void CItemContainer::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemContainer::DupeCopy");
	// Copy the contents of this item

	CItemVendable::DupeCopy(pItem);

	const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
	if ( !pContItem )
		return;

	for ( CItem *pContent = pContItem->GetContentHead(); pContent != NULL; pContent = pContent->GetNext() )
		ContentAdd(CreateDupeItem(pContent), pContent->GetContainedPoint());
}

void CItemContainer::MakeKey()
{
	ADDTOCALLSTACK("CItemContainer::MakeKey");
	SetType(IT_CONTAINER);
	m_itContainer.m_lockUID = GetUID();
	m_itContainer.m_lock_complexity = 500 + Calc_GetRandVal(600);

	CItem *pKey = CreateScript(ITEMID_KEY_COPPER);
	ASSERT(pKey);
	pKey->m_itKey.m_lockUID = GetUID();
	ContentAdd(pKey);
}

void CItemContainer::SetKeyRing()
{
	ADDTOCALLSTACK("CItemContainer::SetKeyRing");
	// Look of a key ring depends on how many keys it has in it
	static const ITEMID_TYPE sm_Item_Keyrings[] =
	{
		ITEMID_KEY_RING0,
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING5
	};

	size_t iQty = GetCount();
	if ( iQty >= COUNTOF(sm_Item_Keyrings) )
		iQty = COUNTOF(sm_Item_Keyrings) - 1;

	ITEMID_TYPE id = sm_Item_Keyrings[iQty];
	if ( id != GetID() )
	{
		SetID(id);
		Update();
	}
}

bool CItemContainer::CanContainerHold(const CItem *pItem, const CChar *pCharMsg)
{
	ADDTOCALLSTACK("CItemContainer::CanContainerHold");
	if ( !pCharMsg || !pItem )
		return false;
	if ( pCharMsg->IsPriv(PRIV_GM) )
		return true;

	if ( IsAttr(ATTR_MAGIC) )
	{
		pCharMsg->SysMessageDefault(DEFMSG_CONT_MAGIC);
		return false;
	}

	CVarDefCont *pVar = GetKey("OVERRIDE.MAXITEMS", false);
	size_t tMaxItemsCont = pVar ? static_cast<size_t>(pVar->GetValNum()) : MAX_ITEMS_CONT;
	if ( GetCount() >= tMaxItemsCont )
	{
		pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL);
		return false;
	}

	int iMaxWeight = (GetContainedLayer() == LAYER_PACK) ? g_Cfg.m_iBackpackMaxWeight + m_ModMaxWeight : m_ModMaxWeight;
	if ( iMaxWeight && (GetTotalWeight() + pItem->GetWeight() > iMaxWeight) )
	{
		pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL_WEIGHT);
		return false;
	}

	switch ( GetType() )
	{
		case IT_EQ_BANK_BOX:
		{
			// Check if the bankbox will allow this item to be dropped into it
			const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
			DWORD dwItemsInContainer = pContItem ? pContItem->ContentCountAll() : 0;
			DWORD dwBankIMax = pVar ? static_cast<DWORD>(pVar->GetValNum()) : g_Cfg.m_iBankIMax;

			if ( ContentCountAll() + dwItemsInContainer > dwBankIMax )
			{
				pCharMsg->SysMessageDefault(DEFMSG_BVBOX_FULL_ITEMS);
				return false;
			}

			int iBankWMax = g_Cfg.m_iBankWMax + (m_ModMaxWeight * WEIGHT_UNITS);
			if ( iBankWMax >= 0 )
			{
				if ( GetWeight() + pItem->GetWeight() > iBankWMax )
				{
					pCharMsg->SysMessageDefault(DEFMSG_BVBOX_FULL_WEIGHT);
					return false;
				}
			}
			break;
		}
		case IT_GAME_BOARD:
		{
			if ( !pItem->IsType(IT_GAME_PIECE) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOTGAMEPIECE);
				return false;
			}
			break;
		}
		case IT_BBOARD:		// bulletin boards are containers but we can't put stuff in them this way
			return false;
		case IT_EQ_TRADE_WINDOW:
			return true;
		case IT_KEYRING:
		{
			if ( !pItem->IsType(IT_KEY) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOTKEY);
				return false;
			}
			if ( !pItem->m_itKey.m_lockUID )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOBLANKRING);
				return false;
			}
			break;
		}
		case IT_EQ_VENDOR_BOX:
		{
			if ( pItem->IsTimerSet() && !pItem->IsAttr(ATTR_DECAY) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOT4SALE);
				return false;
			}

			// Fix players losing their items when attempting to sell them with no VALUE property scripted
			if ( !pItem->Item_GetDef()->GetMakeValue(0) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOT4SALE);
				return false;
			}
			break;
		}
		case IT_TRASH_CAN:
		{
			Sound(0x235);
			pCharMsg->SysMessageDefault(DEFMSG_ITEMUSE_TRASHCAN);
			SetTimeout(15 * TICK_PER_SEC);
			break;
		}
		default:
			break;
	}

	return true;
}

void CItemContainer::Restock()
{
	ADDTOCALLSTACK("CItemContainer::Restock");
	// Check for vendor type containers

	if ( IsItemEquipped() )
	{
		// Part of a vendor
		CChar *pChar = dynamic_cast<CChar *>(GetParent());
		if ( pChar && !pChar->IsStatFlag(STATF_Pet) )
		{
			switch ( GetEquipLayer() )
			{
				case LAYER_VENDOR_STOCK:
				{
					// Reset vendor stock
					for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
					{
						CItemVendable *pVendItem = dynamic_cast<CItemVendable *>(pItem);
						if ( pVendItem )
							pVendItem->Restock(true);
					}
					break;
				}
				case LAYER_VENDOR_EXTRA:
				{
					// Clear all this junk periodically
					DeleteAll();
					break;
				}
				case LAYER_VENDOR_BUYS:
				{
					// Reset what we bought from players
					for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
					{
						CItemVendable *pVendItem = dynamic_cast<CItemVendable *>(pItem);
						if ( pVendItem )
							pVendItem->Restock(false);
					}
					break;
				}
				case LAYER_BANKBOX:
				{
					// Restock cash
					if ( !m_itEqBankBox.m_Check_Restock )
						m_itEqBankBox.m_Check_Restock = 10000;
					if ( m_itEqBankBox.m_Check_Amount < m_itEqBankBox.m_Check_Restock )
						m_itEqBankBox.m_Check_Amount = m_itEqBankBox.m_Check_Restock;
					break;
				}
				default:
					break;
			}
		}
	}
}

void CItemContainer::OnOpenEvent(CChar *pCharOpener, const CObjBaseTemplate *pObjTop)
{
	ADDTOCALLSTACK("CItemContainer::OnOpenEvent");
	// The container is being opened

	if ( !pCharOpener )
		return;

	if ( IsType(IT_EQ_VENDOR_BOX) || IsType(IT_EQ_BANK_BOX) )
	{
		const CChar *pCharTop = dynamic_cast<const CChar *>(pObjTop);
		if ( !pCharTop )
			return;

		if ( pCharTop == pCharOpener )
			pCharOpener->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_SELF), GetWeight() / WEIGHT_UNITS, GetName());
		else
			pCharOpener->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_OTHER), pCharTop->GetPronoun(), GetWeight() / WEIGHT_UNITS, pCharTop->GetPossessPronoun(), GetName());

		// To avoid exploits, bank box should store char P when opened and only allow access from this P (see CanTouch)
		m_itEqBankBox.m_ptOpen = pCharOpener->GetTopPoint();
	}
}

void CItemContainer::Game_Create()
{
	ADDTOCALLSTACK("CItemContainer::Game_Create");
	ASSERT(IsType(IT_GAME_BOARD));

	if ( GetCount() > 0 )
		return;		// already here

	static const ITEMID_TYPE sm_Item_ChessPieces[] =
	{
		ITEMID_GAME1_ROOK,		// 42,4
		ITEMID_GAME1_KNIGHT,
		ITEMID_GAME1_BISHOP,
		ITEMID_GAME1_QUEEN,
		ITEMID_GAME1_KING,
		ITEMID_GAME1_BISHOP,
		ITEMID_GAME1_KNIGHT,
		ITEMID_GAME1_ROOK,		// 218,4

		ITEMID_GAME1_PAWN,		// 42,41
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,		// 218, 41

		ITEMID_GAME2_PAWN,		// 44, 167
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,		// 218, 164

		ITEMID_GAME2_ROOK,		// 42, 185
		ITEMID_GAME2_KNIGHT,
		ITEMID_GAME2_BISHOP,
		ITEMID_GAME2_QUEEN,
		ITEMID_GAME2_KING,
		ITEMID_GAME2_BISHOP,
		ITEMID_GAME2_KNIGHT,
		ITEMID_GAME2_ROOK		// 218, 183
	};

	// Chess has 2 rows of 8 pieces on each side
	static const WORD sm_ChessRow[] =
	{
		5,
		40,
		160,
		184
	};

	// Checkers has 3 rows of 4 pieces on each side
	static const WORD sm_CheckerRow[] =
	{
		30,
		55,
		80,
		155,
		180,
		205
	};

	// Backgammon has an odd setup
	static const WORD sm_BackgammonRow[] =
	{
		// White Side
		// Position 13
		8,
		23,
		38,
		53,
		68,
		// Position 6
		128,
		143,
		158,
		173,
		188,
		// Position 24
		8,
		23,
		// Position 8
		158,
		173,
		188,

		// Black Side
		// Position 12
		128,
		143,
		158,
		173,
		188,
		// Position 19
		8,
		23,
		38,
		53,
		68,
		// Position 1
		173,
		188,
		// Position 17
		8,
		23,
		38
	};

	CPointMap pt;

	if ( m_itGameBoard.m_GameType == 0 )	// chess
	{
		for ( size_t i = 0; i < COUNTOF(sm_Item_ChessPieces); ++i )
		{
			// Add all it's pieces (if not already added)
			CItem *pPiece = CItem::CreateBase(sm_Item_ChessPieces[i]);
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Move to the next row after 8 pieces
			if ( (i & 7) == 0 )
			{
				pt.m_x = 42;
				pt.m_y = sm_ChessRow[i / 8];
			}
			else
			{
				// Skip white squares
				pt.m_x += 25;
			}
			ContentAdd(pPiece, pt);
		}
	}
	else if ( m_itGameBoard.m_GameType == 1 )	// checkers
	{
		for ( size_t i = 0; i < 24; ++i )
		{
			// Add all it's pieces (if not already added)
			CItem *pPiece = CItem::CreateBase((i >= 12) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER);
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Move to the next row after 4 pieces (in checkers the pieces are placed on the black squares)
			if ( (i & 3) == 0 )
			{
				pt.m_x = ((i / 4) & 1) ? 67 : 42;
				pt.m_y = sm_CheckerRow[i / 4];
			}
			else
			{
				// Skip white squares
				pt.m_x += 50;
			}
			ContentAdd(pPiece, pt);
		}
	}
	else if ( m_itGameBoard.m_GameType == 2 )	// backgammon
	{
		for ( size_t i = 0; i < 30; ++i )
		{
			// Add all it's pieces (if not already added)
			CItem *pPiece = CItem::CreateBase((i >= 15) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER);
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Backgammon has a strange setup (someone more familiar with the game may be able to improve the code for this)
			if ( (i == 12) || (i == 27) )
				pt.m_x = 107;
			else if ( (i == 10) || (i == 25) )
				pt.m_x = 224;
			else if ( (i == 5) || (i == 20) )
				pt.m_x = 141;
			else if ( (i == 0) || (i == 15) )
				pt.m_x = 41;
			pt.m_y = sm_BackgammonRow[i];
			ContentAdd(pPiece, pt);
		}
	}
}

enum ICV_TYPE
{
	ICV_CLOSE,
	ICV_DELETE,
	ICV_EMPTY,
	ICV_FIXWEIGHT,
	ICV_OPEN,
	ICV_QTY
};

LPCTSTR const CItemContainer::sm_szVerbKeys[ICV_QTY + 1] =
{
	"CLOSE",
	"DELETE",
	"EMPTY",
	"FIXWEIGHT",
	"OPEN",
	NULL
};

bool CItemContainer::r_Verb(CScript &s, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	switch ( FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1) )
	{
		case ICV_CLOSE:
		{
			if ( pSrc->GetChar() )
			{
				CChar *pChar = pSrc->GetChar();
				if ( pChar->m_pClient )
					pChar->m_pClient->closeUIWindow(PacketCloseUIWindow::Container, this);
			}
			return true;
		}
		case ICV_DELETE:
		{
			if ( s.HasArgs() )
			{
				// 1 based pages
				size_t index = s.GetArgVal();
				if ( (index > 0) && (index <= GetCount()) )
				{
					delete GetAt(index - 1);
					return true;
				}
			}
			return false;
		}
		case ICV_EMPTY:
		{
			DeleteAll();
			return true;
		}
		case ICV_FIXWEIGHT:
		{
			FixWeight();
			return true;
		}
		case ICV_OPEN:
		{
			if ( pSrc->GetChar() )
			{
				CChar *pChar = pSrc->GetChar();
				if ( pChar->m_pClient )
				{
					if ( s.HasArgs() )
					{
						pChar->m_pClient->addItem(this);
						pChar->m_pClient->addCustomSpellbookOpen(this, static_cast<GUMP_TYPE>(s.GetArgVal()));
						return true;
					}
					pChar->m_pClient->addItem(this);
					pChar->m_pClient->addContainerSetup(this);
					OnOpenEvent(pChar, GetTopLevelObj());
				}
			}
			return true;
		}
	}
	return CItemVendable::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CItemContainer::OnTick()
{
	ADDTOCALLSTACK("CItemContainer::OnTick");
	// Item is equipped or not
	switch ( GetType() )
	{
		case IT_CONTAINER:
		{
			if ( IsAttr(ATTR_MAGIC) )
			{
				Restock();
				return true;
			}
			break;
		}
		case IT_TRASH_CAN:
		{
			DeleteAll();
			return true;
		}
		case IT_EQ_VENDOR_BOX:
		case IT_EQ_BANK_BOX:
		{
			Restock();
			SetTimeout(-1);
			return true;
		}
		default:
			break;
	}
	return CItemVendable::OnTick();
}
