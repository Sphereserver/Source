//
// CContain.CPP
//
#include "graysvr.h"	// predef header.
#include "../network/send.h"

//***************************************************************************
// -CContainer

void CContainer::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CContainer::OnWeightChange");
	// Propagate the weight change up the stack if there is one.
	m_totalweight += iChange;
}

int CContainer::FixWeight()
{
	ADDTOCALLSTACK("CContainer::FixWeight");
	// If there is some sort of ASSERT during item add then this is used to fix it.
	m_totalweight = 0;

	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItem->GetNext())
	{
		CItemContainer * pCont = dynamic_cast <CItemContainer *> (pItem);
		if ( pCont )
		{
			pCont->FixWeight();
			if ( ! pCont->IsWeighed())
				continue;	// Bank box doesn't count for wieght.
		}
		m_totalweight += pItem->GetWeight();
	}
	return( m_totalweight );
}

void CContainer::ContentAddPrivate( CItem * pItem )
{
	ADDTOCALLSTACK("CContainer::ContentAddPrivate");
	// We are adding to a CChar or a CItemContainer
	ASSERT( pItem != NULL );
	ASSERT( pItem->IsValidUID());	// it should be valid at this point.
	if ( pItem->GetParent() == this )
		return;

	CGObList::InsertHead( pItem );
	OnWeightChange( pItem->GetWeight());
}

void CContainer::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CContainer::OnRemoveOb");
	// remove this object from the container list.
	// Overload the RemoveAt for general lists to come here.
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	ASSERT(pItem);
	CGObList::OnRemoveOb( pItem );
	ASSERT( pItem->GetParent() == NULL );
	pItem->SetContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
	OnWeightChange( -pItem->GetWeight());
}

void CContainer::r_WriteContent( CScript & s ) const
{
	ADDTOCALLSTACK("CContainer::r_WriteContent");
	ASSERT(dynamic_cast<const CGObList *>(this) != NULL);

	// Write out all the items in me.
	CItem* pItemNext;
	for ( CItem* pItem = GetContentHead(); pItem != NULL; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();
		ASSERT( pItem->GetParent() == this );
		pItem->r_WriteSafe(s);
	}
}

CItem * CContainer::ContentFind( RESOURCE_ID_BASE rid, DWORD dwArg, int iDecendLevels ) const
{
	ADDTOCALLSTACK("CContainer::ContentFind");
	// send all the items in the container.

	if ( rid.GetResIndex() == 0 )
		return( NULL );

	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItem->GetNext())
	{
		if ( pItem->IsResourceMatch( rid, dwArg ))
			break;
		if ( iDecendLevels <= 0 )
			continue;
		CItemContainer * pCont = dynamic_cast <CItemContainer*>(pItem);
		if ( pCont != NULL )
		{
			if ( ! pCont->IsSearchable())
				continue;
			CItem * pItemInCont = pCont->ContentFind( rid, dwArg, iDecendLevels-1 );
			if ( pItemInCont )
				return( pItemInCont );
		}
	}
	return( pItem );
}

TRIGRET_TYPE CContainer::OnContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, RESOURCE_ID_BASE rid, DWORD dwArg, int iDecendLevels )
{
	ADDTOCALLSTACK("CContainer::OnContTriggerForLoop");
	if ( rid.GetResIndex() != 0 )
	{
		CItem* pItem = GetContentHead();
		CItem * pItemNext;
		for ( ; pItem!=NULL; pItem=pItemNext)
		{
			pItemNext = pItem->GetNext();
			
			if ( pItem->IsResourceMatch( rid, dwArg ))
			{
				s.SeekContext( StartContext );
				TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
 				if ( iRet != TRIGRET_ENDIF )
				{
					return( iRet );
				}
				EndContext = s.GetContext();
			}
			if ( iDecendLevels <= 0 )
				continue;
			CItemContainer * pCont = dynamic_cast <CItemContainer*>(pItem);
			if ( pCont != NULL )
			{
				if ( pCont->IsSearchable())
				{
					CContainer * pContBase = dynamic_cast <CContainer *> (pCont);
					TRIGRET_TYPE iRet = pContBase->OnContTriggerForLoop( s, pSrc, pArgs, pResult, StartContext, EndContext, rid, dwArg, iDecendLevels-1 );
					if ( iRet != TRIGRET_ENDIF )
					{
						return( iRet );
					}

					// Since the previous call has already found the EndContext, set it.
					EndContext = s.GetContext();
				}
			}
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		CScriptObj * pScript = dynamic_cast <CScriptObj *> (this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
		{
			return( iRet );
		}
	}
	else
	{
		s.SeekContext( EndContext );
	}
	return( TRIGRET_ENDIF );
}

TRIGRET_TYPE CContainer::OnGenericContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, int iDecendLevels )
{
	ADDTOCALLSTACK("CContainer::OnGenericContTriggerForLoop");
	CItem* pItem = GetContentHead();
	CItem * pItemNext;
	for ( ; pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();
		
		s.SeekContext( StartContext );
		TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
 		if ( iRet != TRIGRET_ENDIF )
		{
			return( iRet );
		}
		EndContext = s.GetContext();
		if ( iDecendLevels <= 0 )
			continue;
		CItemContainer * pCont = dynamic_cast <CItemContainer*>(pItem);
		if ( pCont != NULL )
		{
			if ( pCont->IsSearchable())
			{
				CContainer * pContBase = dynamic_cast <CContainer *> (pCont);
				TRIGRET_TYPE iRet = pContBase->OnGenericContTriggerForLoop( s, pSrc, pArgs, pResult, StartContext, EndContext, iDecendLevels-1 );
				if ( iRet != TRIGRET_ENDIF )
				{
					return( iRet );
				}

				// Since the previous call has already found the EndContext, set it.
				EndContext = s.GetContext();
			}
		}
	}
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		CScriptObj * pScript = dynamic_cast <CScriptObj *> (this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
		{
			return( iRet );
		}
	}
	else
	{
		s.SeekContext( EndContext );
	}
	return( TRIGRET_ENDIF );
}

bool CContainer::ContentFindKeyFor( CItem * pLocked ) const
{
	ADDTOCALLSTACK("CContainer::ContentFindKeyFor");
	// Look for the key that fits this in my possesion.
	// TODO: Double check for pLocked->m_itContainer.m_lockUID working
	return( pLocked->m_itContainer.m_lockUID && (ContentFind( RESOURCE_ID( RES_TYPEDEF, IT_KEY ), pLocked->m_itContainer.m_lockUID ) != NULL) );
}

CItem* CContainer::ContentFindRandom() const
{
	ADDTOCALLSTACK("CContainer::ContentFindRandom");
	// returns Pointer of random item, NULL if player carrying none
	return( dynamic_cast <CItem*> ( GetAt( Calc_GetRandVal( GetCount()))));
}

int CContainer::ContentConsume( RESOURCE_ID_BASE rid, int amount, bool fTest, DWORD dwArg )
{
	ADDTOCALLSTACK("CContainer::ContentConsume");
	// ARGS:
	//  dwArg = a hack for ores.
	// RETURN:
	//  0 = all consumed ok.
	//  # = number left to be consumed. (still required)

	if ( rid.GetResIndex() == 0 )
		return( amount );	// from skills menus.

	CItem * pItemNext;
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();

		if ( pItem->IsResourceMatch( rid, dwArg ))
		{
			amount -= pItem->ConsumeAmount( amount, fTest );
			if ( amount <= 0 )
				break;
		}

		CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem);
		if ( pCont != NULL )	// this is a sub-container.
		{
			if ( rid == RESOURCE_ID(RES_TYPEDEF,IT_GOLD))
			{
				if ( pCont->IsType(IT_CONTAINER_LOCKED))
					continue;
			}
			else
			{
				if ( ! pCont->IsSearchable())
					continue;
			}
			amount = pCont->ContentConsume( rid, amount, fTest, dwArg );
			if ( amount <= 0 )
				break;
		}
	}
	return( amount );
}

int CContainer::ContentCount( RESOURCE_ID_BASE rid, DWORD dwArg )
{
	ADDTOCALLSTACK("CContainer::ContentCount");
	// Calculate total (gold or other items) in this recursed container
	return( INT_MAX - ContentConsume( rid, INT_MAX, true, dwArg ));
}

void CContainer::ContentAttrMod( WORD wAttr, bool fSet )
{
	ADDTOCALLSTACK("CContainer::ContentAttrMod");
	// Mark the attr
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
	{
		if ( fSet )
		{
			pItem->SetAttr( wAttr );
		}
		else
		{
			pItem->ClrAttr( wAttr );
		}
		CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem);
		if ( pCont != NULL )	// this is a sub-container.
		{
			pCont->ContentAttrMod( wAttr, fSet );
		}
	}
}

void CContainer::ContentNotifyDelete()
{
	ADDTOCALLSTACK("CContainer::ContentNotifyDelete");
	if ( IsSetEF(EF_Minimize_Triggers) ) // no point entering this loop if the trigger is disabled
		return;

	// trigger @Destroy on contained items
	CItem *pItemNext = NULL;
	for (CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();

		if ( pItem->NotifyDelete() == false )
		{
			// item shouldn't be destroyed and so cannot remain in this container,
			// drop it to the ground if it hasn't been moved already
			if (pItem->GetParent() == this)
				pItem->MoveToCheck( pItem->GetTopLevelObj()->GetTopPoint() );
		}
	}
}

void CContainer::ContentsDump( const CPointMap & pt, WORD wAttrLeave )
{
	ADDTOCALLSTACK("CContainer::ContentsDump");
	// Just dump the contents onto the ground.
	wAttrLeave |= ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_CURSED2|ATTR_BLESSED2;
	CItem * pItemNext;
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsAttr(wAttrLeave))
			continue;	// hair and newbie stuff.
		// ??? scatter a little ?
		pItem->MoveToCheck( pt );
	}
}

void CContainer::ContentsTransfer( CItemContainer * pCont, bool fNoNewbie )
{
	ADDTOCALLSTACK("CContainer::ContentsTransfer");
	// Move all contents to another container. (pCont)
	if ( pCont == NULL )
		return;

	CItem* pItemNext;
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();
		if ( fNoNewbie && pItem->IsAttr( ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_CURSED2|ATTR_BLESSED2 ))	// keep newbie stuff.
			continue;
		pCont->ContentAdd( pItem );	// add content
	}
}

size_t CContainer::ResourceConsumePart( const CResourceQtyArray * pResources, int iReplicationQty, int iDamagePercent, bool fTest, DWORD dwArg )
{
	ADDTOCALLSTACK("CContainer::ResourceConsumePart");
	// Consume just some of the resources.
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product.
	//  iDamagePercent = 0-100
	// RETURN:
	//  BadIndex = all needed items where present.
	// index of the item we did not have.

	if ( iDamagePercent <= 0 )
		return pResources->BadIndex();

	size_t iMissing = pResources->BadIndex();

	size_t iQtyRes = pResources->GetCount();
	for ( size_t i = 0; i < iQtyRes; i++ )
	{
		int iResQty = pResources->GetAt(i).GetResQty();
		if (iResQty <= 0) // not sure why this would be true
			continue;

		int iQtyTotal = ( iResQty * iReplicationQty );
		if ( iQtyTotal <= 0 )
			continue;
		iQtyTotal = IMULDIV( iQtyTotal, iDamagePercent, 100 );
		if ( iQtyTotal <= 0 )
			continue;

		RESOURCE_ID rid = pResources->GetAt(i).GetResourceID();
		int iRet = ContentConsume( rid, iQtyTotal, fTest, dwArg );
		if ( iRet )
		{
			iMissing = i;
		}
	}

	return( iMissing );
}

int CContainer::ResourceConsume( const CResourceQtyArray * pResources, int iReplicationQty, bool fTest, DWORD dwArg )
{
	ADDTOCALLSTACK("CContainer::ResourceConsume");
	// Consume or test all the required resources.
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product.
	// RETURN:
	//  how many whole objects can be made. <= iReplicationQty

	if ( iReplicationQty <= 0 )
		iReplicationQty = 1;
	if ( ! fTest && iReplicationQty > 1 )
	{
		// Test what the max number we can really make is first !
		// All resources must be consumed with the same number.
		iReplicationQty = ResourceConsume( pResources, iReplicationQty, true, dwArg );
	}

	int iQtyMin = INT_MAX;
	for ( size_t i = 0; i < pResources->GetCount(); i++ )
	{
		int iResQty = pResources->GetAt(i).GetResQty();
		if (iResQty <= 0) // not sure why this would be true
			continue;

		int iQtyTotal = ( iResQty * iReplicationQty );
		RESOURCE_ID rid = pResources->GetAt(i).GetResourceID();
		if ( rid.GetResType() == RES_SKILL )
		{
			CChar *	pChar	= dynamic_cast <CChar *> (this);
			if ( !pChar )
				continue;
			if ( pChar->Skill_GetBase( (SKILL_TYPE) rid.GetResIndex() ) < iResQty )
				return 0;
			continue;
		}

		int iQtyCur = iQtyTotal - ContentConsume( rid, iQtyTotal, fTest, dwArg );
		iQtyCur /= iResQty;
		if ( iQtyCur < iQtyMin )
			iQtyMin = iQtyCur;
	}

	if ( iQtyMin == INT_MAX )	// it has no resources ? So i guess we can make it from nothing ?
		return( iReplicationQty );

	return( iQtyMin );
}

int CContainer::ContentCountAll() const
{
	ADDTOCALLSTACK("CContainer::ContentCountAll");
	// RETURN:
	//  A count of all the items in this container and sub contianers.
	int iTotal = 0;
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
	{
		iTotal++;
		const CItemContainer * pCont = dynamic_cast <const CItemContainer*>(pItem);
		if ( pCont == NULL )
			continue;
		// if ( ! pCont->IsSearchable()) continue; // found a sub
		iTotal += pCont->ContentCountAll();
	}
	return( iTotal );
}

bool CContainer::r_GetRefContainer( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CContainer::r_GetRefContainer");
	if ( !strnicmp(pszKey, "FIND", 4) )				// find*
	{
		pszKey += 4;
		if ( !strnicmp(pszKey, "ID", 2) )			// findid
		{
			pszKey += 2;
			SKIP_SEPARATORS(pszKey);
			pRef = ContentFind(g_Cfg.ResourceGetID(RES_ITEMDEF, pszKey));
			SKIP_SEPARATORS(pszKey);
			return true;
		}
		else if ( !strnicmp(pszKey, "CONT", 4) )	// findcont
		{
			pszKey += 4;
			SKIP_SEPARATORS(pszKey);
			pRef = GetAt(Exp_GetSingle(pszKey));
			SKIP_SEPARATORS(pszKey);
			return true;
		}
		else if ( !strnicmp(pszKey, "TYPE", 4) )	// findtype
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

bool CContainer::r_WriteValContainer(LPCTSTR pszKey, CGString & sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CContainer::r_WriteValContainer");
	EXC_TRY("WriteVal");

	static LPCTSTR const sm_szParams[] =
	{
		"count",
		"fcount",
		"rescount",
		"restest",
	};

	int i = FindTableHeadSorted(pszKey, sm_szParams, COUNTOF(sm_szParams));
	if ( i < 0 )
		return false;

	LPCTSTR	pKey = pszKey + strlen(sm_szParams[i]);
	SKIP_SEPARATORS(pKey);
	switch ( i )
	{
		case 0:			//	count
		{
			int iTotal = 0;

			for ( CItem* pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
				iTotal++;

			sVal.FormatVal(iTotal);
		} break;

		case 1:			//	fcount
			sVal.FormatVal(ContentCountAll());
			break;

		case 2:			//	rescount
			sVal.FormatVal(*pKey ? ContentCount(g_Cfg.ResourceGetID(RES_ITEMDEF, pKey)) : GetCount());
			break;

		case 3:			//	restest
		{
			CResourceQtyArray Resources;
			sVal.FormatVal(Resources.Load(pKey) ? ResourceConsume(&Resources, 1, true) : 0);
		} break;

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

//----------------------------------------------------
// -CItemContainer

CItemContainer::CItemContainer( ITEMID_TYPE id, CItemBase * pItemDef ) :
	CItemVendable( id, pItemDef )
{
	// m_fTinkerTrapped = false;
}

bool CItemContainer::NotifyDelete()
{
	// notify destruction of the container before its contents
	if ( CItem::NotifyDelete() == false )
		return false;

	// ensure trade contents are moved out
	if ( IsType(IT_EQ_TRADE_WINDOW) )
		Trade_Delete();

	ContentNotifyDelete();
	return true;
}

void CItemContainer::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemContainer::r_Write");
	CItemVendable::r_Write(s);
	r_WriteContent(s);
}

bool CItemContainer::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItemContainer::r_GetRef");
	if ( r_GetRefContainer( pszKey, pRef ))
	{
		return( true );
	}
	return( CItemVendable::r_GetRef( pszKey, pRef ));
}

bool CItemContainer::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemContainer::r_WriteVal");
	if ( r_WriteValContainer(pszKey, sVal, pSrc) )
		return true;
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

void CItemContainer::Trade_Status( bool fCheck )
{
	ADDTOCALLSTACK("CItemContainer::Trade_Status");
	// Update trade status check boxes to both sides.
	CItemContainer * pPartner = dynamic_cast <CItemContainer *> ( m_uidLink.ItemFind());
	if ( pPartner == NULL )
		return;

	CChar * pChar1 = dynamic_cast <CChar*> (GetParent());
	if ( pChar1 == NULL )
		return;
	CChar * pChar2 = dynamic_cast <CChar*> (pPartner->GetParent());
	if ( pChar2 == NULL )
		return;

	m_itEqTradeWindow.m_fCheck = fCheck;
	if ( ! fCheck )
	{
		pPartner->m_itEqTradeWindow.m_fCheck = false;
	}

	PacketTradeAction cmd(SECURE_TRADE_CHANGE);

	if ( pChar1->IsClient())
	{
		cmd.prepareReadyChange(this, pPartner);
		cmd.send(pChar1->GetClient());
	}
	if ( pChar2->IsClient())
	{
		cmd.prepareReadyChange(pPartner, this);
		cmd.send(pChar2->GetClient());
	}

	// if both checked then done.
	if ( ! pPartner->m_itEqTradeWindow.m_fCheck || ! m_itEqTradeWindow.m_fCheck )
		return;

	CItem * pItem;
	CItem * pItemNext;
	int iCont1, iCont2;
	unsigned short i;

	CScriptTriggerArgs Args1(pChar1);
	pItem = pPartner->GetContentHead();
	for ( i = 1; pItem != NULL; pItem = pItemNext, ++i )
	{
		pItemNext = pItem->GetNext();
		Args1.m_VarObjs.Insert( i, pItem, true );
	}

	Args1.m_iN1 = iCont1 = --i;
	pItemNext = NULL;
	//DEBUG_ERR(("Args1.m_iN1(%d) = iCont1(%d) = i(%d)\n",Args1.m_iN1, iCont1, i));

	CScriptTriggerArgs Args2(pChar2);
	pItem = GetContentHead();
	for ( i = 1; pItem != NULL; pItem = pItemNext, ++i )
	{
		pItemNext = pItem->GetNext();
		Args2.m_VarObjs.Insert( i, pItem, true );
	}

	Args2.m_iN1 = iCont2 = --i;
	pItemNext = NULL;
	//DEBUG_ERR(("Args2.m_iN1(%d) = iCont2(%d) = i(%d)\n",Args2.m_iN1, iCont2, i));

	Args1.m_iN2 = iCont2;
	Args2.m_iN2 = iCont1;
	if (( pChar1->OnTrigger(CTRIG_TradeAccepted, pChar2, &Args1) == TRIGRET_RET_TRUE ) || ( pChar2->OnTrigger(CTRIG_TradeAccepted, pChar1, &Args2) == TRIGRET_RET_TRUE ))
		Delete(); //Return 1 in one of the triggers

	pItem = GetContentHead();
	for ( ; pItem != NULL; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();
		pChar2->ItemBounce( pItem );
	}

	pItemNext = NULL;
	pItem = pPartner->GetContentHead();	
	for ( ; pItem != NULL; pItem = pItemNext)
	{
		pItemNext = pItem->GetNext();
		pChar1->ItemBounce( pItem );
	}

	// done with trade.
	Delete();
}

void CItemContainer::Trade_Delete()
{
	ADDTOCALLSTACK("CItemContainer::Trade_Delete");
	// Called when object deleted.

	ASSERT( IsType(IT_EQ_TRADE_WINDOW) );

	CChar * pChar = dynamic_cast <CChar*> (GetParent());
	if ( pChar == NULL )
		return;

	if ( pChar->IsClient())
	{
		// Send the cancel trade message.
		PacketTradeAction cmd(SECURE_TRADE_CLOSE);
		cmd.prepareClose(this);
		cmd.send(pChar->GetClient());
	}

	// Drop items back in my pack.
	CItem * pItemNext;
	for ( CItem* pItem = GetContentHead(); pItem!=NULL; pItem=pItemNext)
	{
		pItemNext = pItem->GetNext();
		pChar->ItemBounce( pItem );
	}

	// Kill my trading partner.
	CItemContainer * pPartner = dynamic_cast <CItemContainer *> ( m_uidLink.ItemFind());
	if ( pPartner == NULL )
		return;

	m_uidLink.InitUID();	// unlink.
	pPartner->m_uidLink.InitUID();
	pPartner->Delete();
}

void CItemContainer::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CItemContainer::OnWeightChange");
	CContainer::OnWeightChange( iChange );
	UpdatePropertyFlag(AUTOTOOLTIP_FLAG_WEIGHT);

	if ( iChange == 0 )
		return;	// no change

	// some containers do not add weight to you.
	if ( ! IsWeighed())
		return;

	// Propagate the weight change up the stack if there is one.
	CContainer * pCont = dynamic_cast <CContainer*>(GetParent());
	if ( pCont == NULL )
		return;	// on ground.
	pCont->OnWeightChange( iChange );
}

CPointMap CItemContainer::GetRandContainerLoc() const
{
	ADDTOCALLSTACK("CItemContainer::GetRandContainerLoc");
	// Max/Min Container Sizes.

	static const struct // we can probably get this from MUL file some place.
	{
		GUMP_TYPE m_gump;
		WORD m_minx;
		WORD m_miny;
		WORD m_maxx;
		WORD m_maxy;
	} sm_ContSize[] =
	{
		{ GUMP_RESERVED, 40, 50, 100, 100 },		// default.
		{ GUMP_SECURE_TRADE, 1, 1, 66, 26 },
		{ GUMP_CORPSE, 20, 85, 80, 185 },
		{ GUMP_PACK, 44, 65, 142, 150 },	// Open backpack
		{ GUMP_BAG, 29, 34, 93, 119 },		// Leather Bag
		{ GUMP_CHEST_GO_SI, 18, 105, 118, 169 },	// Gold and Silver Chest.
		{ GUMP_CHEST_WO_GO, 18, 105, 118, 169 },	// Wood with gold trim.
		{ GUMP_CHEST_SI, 18, 105, 118, 169 },	// silver chest.
		{ GUMP_BARREL, 33, 36, 98, 139 },		// Barrel
		{ GUMP_CRATE, 20, 10, 126, 91 },	// Wood Crate
		{ GUMP_BASKET_SQ, 19, 47, 138, 114 },	// Square picknick Basket
		{ GUMP_BASKET_RO, 33, 36,  98, 134 }, // Round Basket
		{ GUMP_CABINET_DK, 24, 96, 91, 143 },
		{ GUMP_CABINET_LT, 24, 96, 91, 143 },
		{ GUMP_BOOK_SHELF, 76, 12, 96, 59 },
		{ GUMP_BOX_WOOD, 16, 51, 140, 115 },	// small wood box with a lock
		{ GUMP_BOX_WOOD_OR, 16, 51, 140, 115 }, // Small wood box (ornate)(no lock)
		{ GUMP_BOX_GO_LO, 16, 51, 140, 115 }, // Gold/Brass box with a lock.
		{ GUMP_DRAWER_DK, 16, 17, 110, 85 },
		{ GUMP_DRAWER_LT, 16, 17, 110, 85 },
		{ GUMP_SHIP_HOLD, 46, 74, 152, 175 },

		{ GUMP_GAME_BOARD,	 4, 10, 220, 185 },	// Chess or checker board.
		{ GUMP_GAME_BACKGAM, 4, 10, 220, 185 },
	};

	// ??? pItemDef->m_ttContainer.m_dwMinXY to m_dwMaxXY
	// Get a random location in the container.

	CItemBase * pItemDef = Item_GetDef();
	GUMP_TYPE gump = pItemDef->IsTypeContainer();

	// check for custom values in TDATA3/TDATA4
	if ( pItemDef->m_ttContainer.m_dwMaxXY )
	{
		int tmp_MinX = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF0000) >> 16;
		int tmp_MinY = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF);
		int tmp_MaxX = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF0000) >> 16;
		int tmp_MaxY = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF);
		DEBUG_WARN(("Custom container gump id %d for 0%x\n", gump, GetDispID()));
		return( CPointMap(
			tmp_MinX + Calc_GetRandVal( tmp_MaxX - tmp_MinX ),
			tmp_MinY + Calc_GetRandVal( tmp_MaxY - tmp_MinY ),
			0 ));
	}

	unsigned int i = 0;
	for ( ; ; i++ )
	{
		if (i >= COUNTOF(sm_ContSize))
		{
			i = 0;	// set to default
			DEBUG_WARN(("Unknown container gump id %d for 0%x\n", gump, GetDispID()));
			break;
		}
		if ( sm_ContSize[i].m_gump == gump )
			break;
	}

	return( CPointMap(
		sm_ContSize[i].m_minx + Calc_GetRandVal( sm_ContSize[i].m_maxx - sm_ContSize[i].m_minx ),
		sm_ContSize[i].m_miny + Calc_GetRandVal( sm_ContSize[i].m_maxy - sm_ContSize[i].m_miny ),
		0 ));
}

void CItemContainer::ContentAdd( CItem * pItem, CPointMap pt, unsigned char gridIndex )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	// Add to CItemContainer
	if ( pItem == NULL )
		return;
	if ( pItem == this )
		return;	// infinite loop.

	if ( ! g_Serv.IsLoading())
	{
		if ( IsType(IT_EQ_TRADE_WINDOW) )
		{
			// Drop into a trade window.
			Trade_Status( false );
		}
		switch ( pItem->GetType())
		{
			case IT_LIGHT_LIT:
				// Douse the light in a pack ! (if possible)
				pItem->Use_Light();
				break;
			case IT_GAME_BOARD:
			{
				// Can't be put into any sort of a container.
				// delete all it's pieces.
				CItemContainer* pCont = dynamic_cast <CItemContainer*> (pItem);
				ASSERT(pCont);
				pCont->DeleteAll();
				break;
			}
			default:
				break;
		}
	}

	if ( pt.m_x <= 0 || pt.m_y <= 0 || pt.m_x > 512 || pt.m_y > 512 )	// invalid container location ?
	{
		bool fInsert = false;
		// Try to stack it.
		if ( ! g_Serv.IsLoading() && pItem->Item_GetDef()->IsStackableType())
		{
			CItem * pItemNext;
			for ( CItem* pTry=GetContentHead(); pTry!=NULL; pTry=pItemNext)
			{
				pItemNext = pTry->GetNext();
				pt = pTry->GetContainedPoint();
				if ( pItem->Stack( pTry ))
				{
					fInsert = true;
					break;
				}
			}
		}
		if ( ! fInsert )
			pt = GetRandContainerLoc();
	}

	bool bValidGrid = true;
	{	
		// check that the grid index isn't already in use
		CItem * pItemNext;
		for ( CItem * pTry = GetContentHead(); pTry != NULL; pTry = pItemNext )
		{
			pItemNext = pTry->GetNext();
			if ( pTry->GetContainedGridIndex() == gridIndex )
			{
				bValidGrid = false;
				break;
			}
		}
	}

	if ( !bValidGrid )
	{
		// the grid index we've been given is already in use, so find the
		// first unused grid index
		for ( gridIndex = 0; (gridIndex < 255 && !bValidGrid); gridIndex++ )
		{
			bValidGrid = true;

			CItem * pItemNext;
			for ( CItem * pTry = GetContentHead(); pTry != NULL; pTry = pItemNext )
			{
				pItemNext = pTry->GetNext();
				if ( pTry->GetContainedGridIndex() == gridIndex )
				{
					bValidGrid = false;
					break;
				}
			}

			if ( bValidGrid )
				break;
		}
	}

	CContainer::ContentAddPrivate( pItem );
	pItem->SetContainedPoint( pt );
	pItem->SetContainedGridIndex( gridIndex );

	// if an item needs OnTickStatusUpdate called on the next tick, it needs
	// to be added to a separate list since it won't receive ticks whilst in
	// this container
	if (pItem->m_fStatusUpdate != 0 && g_World.m_ObjStatusUpdates.ContainsPtr(pItem) == false)
		g_World.m_ObjStatusUpdates.Add(pItem);

	switch ( GetType())
	{
	case IT_KEYRING: // empty key ring.
		SetKeyRing();
		break;
	case IT_EQ_VENDOR_BOX:
		if ( ! IsItemEquipped())	// vendor boxes should ALWAYS be equipped !
		{
			DEBUG_ERR(("Un-equipped vendor box uid=0%lx is bad\n", (DWORD) GetUID()));
			break;
		}
		{
			CItemVendable * pItemVend = dynamic_cast <CItemVendable *>(pItem);
			if ( pItemVend == NULL )
			{
				g_Log.Event( LOGL_WARN, "Vendor non-vendable item: %s uid=0%lx, vendor: %s uid=0%lx\n", (LPCTSTR) pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetContainer()->GetName(), GetContainer()->GetUID().GetObjUID());
				pItem->Delete();
				break;
			}

			pItemVend->SetPlayerVendorPrice( 0 );	// unpriced yet.
			pItemVend->SetContainedLayer( pItem->GetAmount());
		}
		break;
	case IT_GAME_BOARD:
		// Can only place IT_GAME_PIECE inside here
		if ( pItem->IsType( IT_GAME_PIECE ))
			break;
		g_Log.Event( LOGL_WARN, "Game board contains invalid item: %s uid=0%lx, board: %s uid=0%lx\n", (LPCTSTR) pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetResourceName(), GetUID().GetObjUID());
		pItem->Delete();
		break;
	default:
		break;
	}

	switch( pItem->GetID())
	{
	case ITEMID_BEDROLL_O_EW:
	case ITEMID_BEDROLL_O_NS:
		// Close the bedroll
		pItem->SetDispID( ITEMID_BEDROLL_C );
		break;
	default:
		break;
	}

	pItem->Update();
}

void CItemContainer::ContentAdd( CItem * pItem )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	if ( pItem == NULL )
		return;
	if ( pItem->GetParent() == this )
		return;	// already here.
	CPointMap pt;	// invalid point.
	if ( g_Serv.IsLoading())
	{
		pt = pItem->GetUnkPoint();
	}
	ContentAdd( pItem, pt );
}

bool CItemContainer::IsItemInside(const CItem * pItem) const
{
	ADDTOCALLSTACK("CItemContainer::IsItemInside");
	// Checks if a particular item is in a container or one of
	// it's subcontainers.

	for (;;)
	{
		if ( pItem == NULL )
			return( false );
		if ( pItem == this )
			return( true );
		pItem = dynamic_cast <CItemContainer *>(pItem->GetParent());
	}
}

void CItemContainer::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CItemContainer::OnRemoveOb");
	// remove this object from the container list.
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	ASSERT(pItem);
	if ( IsType(IT_EQ_TRADE_WINDOW))
	{
		// Remove from a trade window.
		Trade_Status( false );
		m_TagDefs.SetNum("wait1sec", g_World.GetCurrentTime().GetTimeRaw() + 60*TICK_PER_SEC, false);
	}
	if ( IsType(IT_EQ_VENDOR_BOX) && IsItemEquipped())	// vendor boxes should ALWAYS be equipped !
	{
		CItemVendable * pItemVend = dynamic_cast <CItemVendable *>(pItem);
		if ( pItemVend != NULL )
		{
			pItemVend->SetPlayerVendorPrice( 0 );
		}
	}

	CContainer::OnRemoveOb(pObRec);

	if ( IsType(IT_KEYRING))	// key ring.
	{
		SetKeyRing();
	}
}

void CItemContainer::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItemContainer::DupeCopy");
	// Copy the contents of this item.

	CItemVendable::DupeCopy( pItem );

	const CItemContainer * pContItem = dynamic_cast <const CItemContainer *>(pItem);
	if ( pContItem == NULL )
		return;

	for ( CItem * pContent = pContItem->GetContentHead(); pContent != NULL; pContent = pContent->GetNext())
	{
		ContentAdd( CreateDupeItem( pContent ), pContent->GetContainedPoint());
	}
}

void CItemContainer::MakeKey()
{
	ADDTOCALLSTACK("CItemContainer::MakeKey");
	SetType(IT_CONTAINER);
	m_itContainer.m_lockUID = GetUID();
	m_itContainer.m_lock_complexity = 500 + Calc_GetRandVal( 600 );

	CItem * pKey = CreateScript( ITEMID_KEY_COPPER );
	ASSERT(pKey);
	pKey->m_itKey.m_lockUID = GetUID();
	ContentAdd( pKey );
}

void CItemContainer::SetKeyRing()
{
	ADDTOCALLSTACK("CItemContainer::SetKeyRing");
	// Look of a key ring depends on how many keys it has in it.
	static const ITEMID_TYPE sm_Item_Keyrings[] =
	{
		ITEMID_KEY_RING0, // empty key ring.
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING5,
	};

	size_t iQty = GetCount();
	if ( iQty >= COUNTOF(sm_Item_Keyrings))
		iQty = COUNTOF(sm_Item_Keyrings)-1;

	ITEMID_TYPE id = sm_Item_Keyrings[iQty];
	if ( id != GetID())
	{
		SetID(id);	// change the type as well.
		Update();
	}
}

bool CItemContainer::CanContainerHold( const CItem * pItem, const CChar * pCharMsg )
{
	ADDTOCALLSTACK("CItemContainer::CanContainerHold");
	if ( pCharMsg == NULL )
		return( false );
	if ( pCharMsg->IsPriv(PRIV_GM))	// a gm can doing anything.
		return( true );

	if ( IsAttr(ATTR_MAGIC))
	{
		// Put stuff in a magic box
		pCharMsg->SysMessageDefault( DEFMSG_CONT_MAGIC );
		return( false );
	}

	size_t tContMaxI = MAX_ITEMS_CONT;
	CVarDefCont * pTagTmp = GetKey("OVERRIDE.MAXITEMS", false);
	if ( pTagTmp )
	{
		tContMaxI = pTagTmp->GetValNum();
	}
	if ( GetCount() >= (tContMaxI - 1) )
	{
		pCharMsg->SysMessageDefault( DEFMSG_CONT_FULL );
		return( false );
	}

	pTagTmp = GetKey("OVERRIDE.MAXWEIGHT", false);
	if ( pTagTmp )	// weightcheck does ALSO apply on backpack if tag is set!
	{
		if (( GetWeight() + pItem->GetWeight()) > (pTagTmp->GetValNum() * WEIGHT_UNITS) )
		{
			pCharMsg->SysMessageDefault( DEFMSG_CONT_FULL_WEIGHT );
			return( false );
		}
	}

	if ( ! IsItemEquipped() &&	// does not apply to my pack.
		pItem->IsContainer() &&
		pItem->Item_GetDef()->GetVolume() >= Item_GetDef()->GetVolume())
	{
		// is the container too small ? can't put barrels in barrels.
		pCharMsg->SysMessageDefault( DEFMSG_CONT_TOOSMALL );
		return( false );
	}

	switch ( GetType())
	{
		case IT_EQ_BANK_BOX:
			{
				// Check if the bankbox will allow this item to be dropped into it.
				// Too many items or too much weight?
				int iBankIMax = g_Cfg.m_iBankIMax;
				int iBankWMax = g_Cfg.m_iBankWMax;

				CVarDefCont * pTagTemp = GetKey("OVERRIDE.MAXITEMS", false);
				if ( pTagTemp )
				{
					iBankIMax = pTagTemp->GetValNum();
				}

				pTagTemp = GetKey("OVERRIDE.MAXWEIGHT", false);
				if ( pTagTemp )
				{
					iBankWMax = pTagTemp->GetValNum() * WEIGHT_UNITS;
				}
		
				// Check if the item dropped in the bank is a container. If it is
				// we need to calculate the number of items in that too.
				int iItemsInContainer = 0;
				const CItemContainer * pContItem = dynamic_cast <const CItemContainer *>( pItem );
				if ( pContItem )
				{
					iItemsInContainer = pContItem->ContentCountAll();
				}
		
				// Check the total number of items in the bankbox and the ev.
				// container put into it.
				if (( ContentCountAll() + iItemsInContainer ) > iBankIMax )
				{
					pCharMsg->SysMessageDefault( DEFMSG_BVBOX_FULL_ITEMS );
					return( false );
				}
		
				// Check the weightlimit on bankboxes.
				if (( GetWeight() + pItem->GetWeight()) > iBankWMax )
				{
					pCharMsg->SysMessageDefault( DEFMSG_BVBOX_FULL_WEIGHT );
					return( false );
				}
			}
			break;
	
		case IT_GAME_BOARD:
			if ( ! pItem->IsType(IT_GAME_PIECE))
			{
				pCharMsg->SysMessageDefault( DEFMSG_ERR_NOTGAMEPIECE );
				return( false );
			}
			break;
		case IT_BBOARD:		// This is a trade window or a bboard.
			return( false );
		case IT_EQ_TRADE_WINDOW:
			// BBoards are containers but you can't put stuff in them this way.
			return( true );
	
		case IT_KEYRING: // empty key ring.
			if ( ! pItem->IsType(IT_KEY) )
			{
				pCharMsg->SysMessageDefault( DEFMSG_ERR_NOTKEY );
				return( false );
			}
			if ( ! pItem->m_itKey.m_lockUID )
			{
				pCharMsg->SysMessageDefault( DEFMSG_ERR_NOBLANKRING );
				return( false );
			}
			break;
	
		case IT_EQ_VENDOR_BOX:
			if ( pItem->IsTimerSet() && ! pItem->IsAttr(ATTR_DECAY))
			{
				pCharMsg->SysMessageDefault( DEFMSG_ERR_NOT4SALE );
				return( false );
			}

			// Fix players losing their items when attempting to sell them with no VALUE property scripted
			if ( !pItem->Item_GetDef()->GetMakeValue(0) )
			{
				pCharMsg->SysMessageDefault( DEFMSG_ERR_NOT4SALE );
				return ( false );
			}

			// Check that this vendor box hasn't already reached its content limit
			if ( GetCount() >= (MAX_ITEMS_VENDOR - 1) )
			{
				pCharMsg->SysMessageDefault( DEFMSG_CONT_FULL );
				return ( false );
			}
			break;
	
		case IT_TRASH_CAN:
			Sound( 0x235 ); // a little sound so we know it "ate" it.
			pCharMsg->SysMessageDefault( DEFMSG_ITEMUSE_TRASHCAN );
			SetTimeout( 15*TICK_PER_SEC );
			break;

		default:
			break;
	}

	return( true );
}

void CItemContainer::Restock()
{
	ADDTOCALLSTACK("CItemContainer::Restock");
	// Check for vendor type containers.

	if ( IsItemEquipped() )
	{
		// Part of a vendor.
		CChar * pChar = dynamic_cast <CChar*>(GetParent());
		if ( pChar && !pChar->IsStatFlag(STATF_Pet) )
		{
			switch ( GetEquipLayer())
			{
			case LAYER_VENDOR_STOCK:
				// Magic restock the vendors container.
				{
					CItem* pItemNext;
					CItem* pItem = GetContentHead();
					for ( ; pItem!=NULL; pItem=pItemNext)
					{
						pItemNext = pItem->GetNext();
						CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
						if ( pVendItem != NULL )
							pVendItem->Restock( true );
					}
				}
				break;

			case LAYER_VENDOR_EXTRA:
				// clear all this junk periodically.
				// sell it back for cash value ?
				DeleteAll();
				break;

			case LAYER_VENDOR_BUYS:
				{
					// Reset what we will buy from players.
					CItem* pItem= GetContentHead();
					for ( ; pItem!=NULL; pItem = pItem->GetNext())
					{
						CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
						if ( pVendItem != NULL )
							pVendItem->Restock( false );
					}
				}
				break;

			case LAYER_BANKBOX:
				// Restock petty cash.
				if ( ! m_itEqBankBox.m_Check_Restock )
					m_itEqBankBox.m_Check_Restock = 10000;
				if ( m_itEqBankBox.m_Check_Amount < m_itEqBankBox.m_Check_Restock )
					m_itEqBankBox.m_Check_Amount = m_itEqBankBox.m_Check_Restock;
				return;
				
			default:
				break;
			}
		}
	}
}

void CItemContainer::OnOpenEvent( CChar * pCharOpener, const CObjBaseTemplate * pObjTop )
{
	ADDTOCALLSTACK("CItemContainer::OnOpenEvent");
	// The container is being opened. Explode ? etc ?

	ASSERT(pCharOpener);

	if ( IsType(IT_EQ_BANK_BOX) ||
		IsType(IT_EQ_VENDOR_BOX))
	{
		const CChar * pCharTop = dynamic_cast <const CChar *>(pObjTop);
		if ( pCharTop == NULL )
			return;

		int iStones = GetWeight()/WEIGHT_UNITS;
		TCHAR *pszMsg = Str_GetTemp();
		if ( pCharTop == pCharOpener )
		{
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_BVBOX_OPEN_SELF ), iStones, (LPCTSTR) GetName());
		}
		else
		{
			sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_BVBOX_OPEN_OTHER ), pCharTop->GetPronoun(), iStones, (LPCTSTR) pCharTop->GetPossessPronoun(), (LPCTSTR) GetName());
		}
		pCharOpener->SysMessage(pszMsg);

		// these are special. they can only be opened near the designated opener.
		// see CanTouch
		m_itEqBankBox.m_pntOpen = pCharOpener->GetTopPoint();
	}
}

void CItemContainer::Game_Create()
{
	ADDTOCALLSTACK("CItemContainer::Game_Create");
	ASSERT( IsType(IT_GAME_BOARD) );
	
	if ( GetCount() > 0 )
		return;	// already here.

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
		ITEMID_GAME2_ROOK,		// 218, 183
	};

	CPointMap pt;

	// Chess has 2 rows of 8 pieces on each side
	static const WORD sm_ChessRow[] =
	{
		5,
		40,
		160,
		184,
	};

	// Checkers has 3 rows of 4 pieces on each side
	static const WORD sm_CheckerRow[] =
	{
		30,
		55,
		80,
		155,
		180,
		205,
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

	// Chess GameType
	if ( m_itGameBoard.m_GameType == 0 )
	{
		for ( size_t i = 0; i < COUNTOF(sm_Item_ChessPieces); i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem * pPiece = CItem::CreateBase( sm_Item_ChessPieces[i] );
			if ( pPiece == NULL )
				break;
			pPiece->SetType(IT_GAME_PIECE);
			
			// Move to the next row after 8 pieces
			if ( (i&7) == 0 )
			{
				pt.m_x = 42;
				pt.m_y = sm_ChessRow[ i/8 ];
			}
			else
			{
				pt.m_x += 25;
			}
			ContentAdd( pPiece, pt );
		}
	}
	// Checkers GameType
	else if ( m_itGameBoard.m_GameType == 1 )
	{
		for ( int i = 0; i < 24; i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem * pPiece = CItem::CreateBase( (( i >= (3*4)) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER ));
			if ( pPiece == NULL )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Move to the next row after 4 pieces
			// In checkers the pieces are placed on the black squares
			if ( (i&3) == 0 )
			{
				pt.m_x = (i / 4)&1? 67:42;
				pt.m_y = sm_CheckerRow[ i/4 ];
			}
			else
			{
				// Skip white squares
				pt.m_x += 50;
			}
			ContentAdd( pPiece, pt );
		}
	}
	// Backgammon GameType
	else if ( m_itGameBoard.m_GameType == 2 )
	{
		for ( int i = 0; i < 30; i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem * pPiece = CItem::CreateBase( (( i >= 15) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER ));
			if ( pPiece == NULL )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Backgammon has a strange setup.
			// Someone more familiar with the game may be able
			// to improve the code for this
			if ( (i == 12) || (i == 27) )
				pt.m_x = 107;
			else if ( (i == 10) || (i == 25) )
				pt.m_x = 224;
			else if ( (i == 5) || (i == 20) )
				pt.m_x = 141;
			else if ( (i == 0) || (i == 15) )
				pt.m_x = 41;
			pt.m_y = sm_BackgammonRow[ i ];
			ContentAdd( pPiece, pt );
		}
	}
}

enum ICV_TYPE
{
	ICV_DELETE,
	ICV_EMPTY,
	ICV_FIXWEIGHT,
	ICV_OPEN,
	ICV_QTY,
};

LPCTSTR const CItemContainer::sm_szVerbKeys[ICV_QTY+1] =
{
	"DELETE",
	"EMPTY",
	"FIXWEIGHT",
	"OPEN",
	NULL,
};

bool CItemContainer::r_Verb( CScript &s, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CItemContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	switch ( FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 ))
	{
		case ICV_DELETE: // "DELETE"
			if ( s.HasArgs())
			{
				// 1 based pages.
				size_t index = s.GetArgVal();
				if ( index > 0 && index <= GetCount())
				{
					delete GetAt( index - 1 );
					return( true );
				}
			}
			return( false );
		case ICV_EMPTY:	// "EMPTY"
			{
				DeleteAll();
				return( true );
			}
		case ICV_FIXWEIGHT:
			FixWeight();
			return( true );
		case ICV_OPEN: // "OPEN"
			if ( pSrc->GetChar())
			{
				CChar * pChar = pSrc->GetChar();
				if ( pChar->IsClient())
				{
					CClient * pClient = pChar->GetClient();
					ASSERT(pClient);
	
					if ( s.HasArgs() )
					{
						pClient->addItem( this );
						pClient->addCustomSpellbookOpen( this, s.GetArgVal() );
						return true;
					}
					pClient->addItem( this );	// may crash client if we dont do this.
					pClient->addContainerSetup( this );
					OnOpenEvent( pChar, GetTopLevelObj());
				}
			}
			return( true );
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
	// equipped or not.
	switch ( GetType())
	{
		case IT_TRASH_CAN:
			// Empty it !
			DeleteAll();
			return true;
		case IT_CONTAINER:
			if ( IsAttr(ATTR_MAGIC))
			{
				// Magic restocking container.
				Restock();
				return true;
			}
			break;
		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
			// Restock this box.
			// Magic restocking container.
			Restock();
			SetTimeout( -1 );
			return true;
		default:
			break;
	}
	return CItemVendable::OnTick();
}

////////////////////////////////////////////////////
// CItemCorpse

CChar * CItemCorpse::IsCorpseSleeping() const
{
	ADDTOCALLSTACK("CItemCorpse::IsCorpseSleeping");
	// Is this corpse really a sleeping person ?
	// CItemCorpse
	if ( !IsType(IT_CORPSE) )
	{
		DEBUG_ERR(("Corpse (0%lx) doesn't have type T_CORPSE!(it has %d)\n",(DWORD)GetUID(),this->GetType()));
		return NULL;
	}

	CChar * pCharCorpse = m_uidLink.CharFind();
	if ( pCharCorpse &&
		pCharCorpse->IsStatFlag( STATF_Sleeping ) &&
		! m_itCorpse.m_timeDeath.IsTimeValid() )
	{
		return( pCharCorpse );
	}
	return( NULL );
}

