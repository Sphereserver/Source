#include "graysvr.h"

CItemVendable::CItemVendable(ITEMID_TYPE id, CItemBase *pItemDef) : CItem(id, pItemDef)
{
	m_price = 0;
	m_quality = 0;
}

CItemVendable::~CItemVendable()
{
	// Nothing really to do, no dynamic memory has been allocated
	DeletePrepare();	// must remove early because virtuals will fail in child destructor
}

void CItemVendable::DupeCopy(const CItem *pItem)
{
	ADDTOCALLSTACK("CItemVendable::DupeCopy");
	CItem::DupeCopy(pItem);

	const CItemVendable *pItemVend = dynamic_cast<const CItemVendable *>(pItem);
	if ( !pItemVend )
		return;

	m_price = pItemVend->m_price;
	m_quality = pItemVend->m_quality;
}

DWORD CItemVendable::GetVendorPrice(int iConvertFactor)
{
	ADDTOCALLSTACK("CItemVendable::GetVendorPrice");
	// Player is buying/selling from a vendor
	// Assume this item is on the vendor!
	// ARGS:
	//  iConvertFactor = percent to apply over base price (-100% to +100%)

	DWORD dwPrice = m_price;
	if ( dwPrice <= 0 )
	{
		CItemBase *pItemDef;
		if ( IsType(IT_DEED) )
		{
			// Deeds just represent the item they are deeding
			pItemDef = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itDeed.m_Type)));
			if ( !pItemDef )
				return 1;
		}
		else
		{
			pItemDef = Item_GetDef();
		}
		dwPrice = pItemDef->GetMakeValue(m_quality);
	}
	return dwPrice + IMULDIV(dwPrice, maximum(iConvertFactor, -100), 100);
}

void CItemVendable::Restock(bool fSellToPlayers)
{
	ADDTOCALLSTACK("CItemVendable::Restock");
	// Assume this is on a non-pet vendor

	ASSERT(IsItemInContainer());
	if ( fSellToPlayers )
	{
		// Restock to full amount
		WORD wAmountRestock = GetContainedLayer();
		if ( !wAmountRestock )
		{
			SetContainedLayer(1);
			wAmountRestock = 1;
		}
		if ( GetAmount() < wAmountRestock )
			SetAmount(wAmountRestock);
	}
	else
	{
		// Clear the amount I have bought from players
		SetContainedLayer(0);
	}
}

bool CItemVendable::IsValidSaleItem(bool fBuyFromVendor) const
{
	ADDTOCALLSTACK("CItemVendable::IsValidSaleItem");
	if ( !IsMovableType() )
	{
		if ( fBuyFromVendor )
			DEBUG_ERR(("Vendor uid=0%lx selling unmovable item '%s'\n", static_cast<DWORD>(GetTopLevelObj()->GetUID()), GetResourceName()));
		return false;
	}
	if ( !fBuyFromVendor && IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_BLESSED) )
		return false;
	if ( IsType(IT_COIN) )
		return false;

	return true;
}

bool CItemVendable::IsValidNPCSaleItem() const
{
	ADDTOCALLSTACK("CItemVendable::IsValidNPCSaleItem");
	if ( (m_price <= 0) && (Item_GetDef()->GetMakeValue(0) <= 0) )
	{
		DEBUG_ERR(("Vendor uid=0%lx selling unpriced item '%s'\n", static_cast<DWORD>(GetTopLevelObj()->GetUID()), GetResourceName()));
		return false;
	}
	if ( !IsValidSaleItem(true) )
	{
		DEBUG_ERR(("Vendor uid=0%lx selling bad item '%s'\n", static_cast<DWORD>(GetTopLevelObj()->GetUID()), GetResourceName()));
		return false;
	}
	return true;
}

enum IVC_TYPE
{
	IVC_PRICE,
	IVC_QUALITY,
	IVC_QTY
};

LPCTSTR const CItemVendable::sm_szLoadKeys[IVC_QTY + 1] =
{
	"PRICE",
	"QUALITY",
	NULL
};

void CItemVendable::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CItemVendable::r_Write");
	CItem::r_Write(s);
	if ( m_price > 0 )
		s.WriteKeyVal("PRICE", m_price);
	if ( m_quality > 0 )
		s.WriteKeyVal("QUALITY", m_quality);
}

bool CItemVendable::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CItemVendable::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case IVC_PRICE:
			sVal.FormatUVal(m_price);
			return true;
		case IVC_QUALITY:
			sVal.FormatUVal(m_quality);
			return true;
		default:
			return CItem::r_WriteVal(pszKey, sVal, pSrc);
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemVendable::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CItemVendable::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case IVC_PRICE:
			m_price = static_cast<DWORD>(s.GetArgVal());
			return true;
		case IVC_QUALITY:
			m_quality = static_cast<WORD>(s.GetArgVal());
			return true;
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
