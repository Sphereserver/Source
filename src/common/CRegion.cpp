#include "../network/network.h"

///////////////////////////////////////////////////////////
// CGRegion

CGRegion::CGRegion()
{
	m_rectUnion.SetRectEmpty();
}

bool CGRegion::AddRegionRect(const CGRect &rect)
{
	ADDTOCALLSTACK("CGRegion::AddRegionRect");
	if ( rect.IsRectEmpty() )
		return false;

	size_t iCount = m_Rects.GetCount();
	if ( (iCount == 0) && IsRegionEmpty() )
	{
		m_rectUnion = rect;
		return true;
	}

	if ( rect.m_map != m_rectUnion.m_map )
		g_Log.EventError("Adding rect %d,%d,%d,%d,%d to region on different map %d\n", rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, rect.m_map, m_rectUnion.m_map);

	// Make sure it is not inside or equal to a previous rect
	for ( size_t i = 0; i < iCount; ++i )
	{
		if ( rect.IsInside(m_Rects[i]) )
			return true;
	}

	if ( iCount == 0 )
	{
		if ( rect.IsInside(m_rectUnion) )
			return true;
		m_Rects.Add(m_rectUnion);
	}

	m_Rects.Add(rect);
	m_rectUnion.UnionRect(rect);
	return true;
}

CGRect &CGRegion::GetRegionRect(size_t i)
{
	ADDTOCALLSTACK("CGRegion::GetRegionRect");
	return (m_Rects.GetCount() == 0) ? m_rectUnion : m_Rects.ElementAt(i);
}

const CGRect &CGRegion::GetRegionRect(size_t i) const
{
	ADDTOCALLSTACK("CGRegion::GetRegionRect");
	return (m_Rects.GetCount() == 0) ? m_rectUnion : m_Rects.ElementAt(i);
}

size_t CGRegion::GetRegionRectCount() const
{
	ADDTOCALLSTACK("CGRegion::GetRegionRectCount");
	size_t iCount = m_Rects.GetCount();
	return ((iCount == 0) && !IsRegionEmpty()) ? 1 : iCount;
}

CPointBase CGRegion::GetRegionCorner(DIR_TYPE dir) const
{
	ADDTOCALLSTACK("CGRegion::GetRegionCorner");
	// NOTE: DIR_QTY = center
	return m_rectUnion.GetRectCorner(dir);
}

bool CGRegion::IsInside(const CGRect &rect) const
{
	ADDTOCALLSTACK("CGRegion::IsInside");
	if ( !m_rectUnion.IsInside(rect) )
		return false;

	size_t iCount = m_Rects.GetCount();
	if ( iCount == 0 )
		return true;

	for ( size_t i = 0; i < iCount; ++i )
	{
		if ( m_Rects[i].IsInside(rect) )
			return true;
	}
	return false;
}

bool CGRegion::IsInside(const CGRegion *pRegion) const
{
	ADDTOCALLSTACK("CGRegion::IsInside");
	if ( !m_rectUnion.IsInside(pRegion->m_rectUnion) )
		return false;

	size_t iCount = pRegion->m_Rects.GetCount();
	for ( size_t i = 0; i < iCount; ++i )
	{
		if ( !IsInside(pRegion->m_Rects[i]) )
			return false;
	}
	return true;
}

bool CGRegion::IsInside2d(const CPointBase &pt) const
{
	ADDTOCALLSTACK("CGRegion::IsInside2d");
	if ( !m_rectUnion.IsInside2d(pt) )
		return false;

	size_t iCount = m_Rects.GetCount();
	if ( iCount > 0 )
	{
		for ( size_t i = 0; i < iCount; ++i )
		{
			if ( m_Rects[i].IsInside2d(pt) )
				return true;
		}
		return false;
	}
	return true;
}

bool CGRegion::IsOverlapped(const CGRect &rect) const
{
	ADDTOCALLSTACK("CGRegion::IsOverlapped");
	if ( !m_rectUnion.IsOverlapped(rect) )
		return false;

	size_t iCount = m_Rects.GetCount();
	if ( iCount == 0 )
		return true;

	for ( size_t i = 0; i < iCount; ++i )
	{
		if ( rect.IsOverlapped(m_Rects[i]) )
			return true;
	}
	return false;
}

bool CGRegion::IsOverlapped(const CGRegion *pRegion) const
{
	ADDTOCALLSTACK("CGRegion::IsOverlapped");
	if ( !m_rectUnion.IsOverlapped(pRegion->m_rectUnion) )
		return false;

	size_t iCount = m_Rects.GetCount();
	size_t iCountRegion = pRegion->m_Rects.GetCount();
	if ( iCount == 0 )
	{
		if ( iCountRegion == 0 )
			return true;
		return pRegion->IsOverlapped(m_rectUnion);
	}

	if ( iCountRegion == 0 )
		return IsOverlapped(pRegion->m_rectUnion);

	for ( size_t i = 0; i < iCount; ++i )
	{
		for ( size_t j = 0; j < iCountRegion; ++j )
		{
			if ( m_Rects[i].IsOverlapped(pRegion->m_Rects[j]) )
				return true;
		}
	}
	return false;
}

bool CGRegion::IsEqualRegion(const CGRegion *pRegion) const
{
	ADDTOCALLSTACK("CGRegion::IsEqualRegion");
	if ( !m_rectUnion.IsEqual(pRegion->m_rectUnion) )
		return false;

	size_t iCount = m_Rects.GetCount();
	size_t iCountRegion = pRegion->m_Rects.GetCount();
	if ( iCount != iCountRegion )
		return false;

	for ( size_t i = 0; i < iCount; ++i )
	{
		for ( size_t j = 0; ; ++j )
		{
			if ( j >= iCountRegion )
				return false;
			if ( m_Rects[i].IsEqual(pRegion->m_Rects[j]) )
				break;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////
// CRegionBase

CRegionBase::CRegionBase(RESOURCE_ID rid, LPCTSTR pszName) : CResourceDef(rid)
{
	if ( pszName )
		SetName(pszName);

	m_dwFlags = 0;
	m_iModified = 0;
	m_iLinkedSectors = 0;
}

CRegionBase::~CRegionBase()
{
	//RemoveSelf();
	UnRealizeRegion();
}

bool CRegionBase::RealizeRegion()
{
	ADDTOCALLSTACK("CRegionBase::RealizeRegion");
	// Link the region to world
	if ( IsRegionEmpty() )
		return false;

	if ( !m_pt.IsValidPoint() )
		m_pt = GetRegionCorner(DIR_QTY);	// center

	// Link to all sectors that it overlaps
	ASSERT(!m_iLinkedSectors);
	int iSectorQty = g_MapList.GetSectorQty(m_pt.m_map);
	for ( int i = 0; i < iSectorQty; ++i )
	{
		CSector *pSector = g_World.GetSector(m_pt.m_map, i);
		if ( pSector && IsOverlapped(pSector->GetRect()) )
		{
			if ( !pSector->LinkRegion(this) )
			{
				g_Log.EventError("Linking sector #%d for region %s on map %hhu failed (fatal for this region)\n", i, GetName(), m_pt.m_map);
				return false;
			}
			++m_iLinkedSectors;
		}
	}
	return true;
}

void CRegionBase::UnRealizeRegion()
{
	ADDTOCALLSTACK("CRegionBase::UnRealizeRegion");
	// Unlink the region from world
	for ( int i = 0; ; ++i )
	{
		CSector *pSector = GetSector(i);
		if ( !pSector )
			return;
		if ( !IsOverlapped(pSector->GetRect()) )
			continue;
		if ( pSector->UnLinkRegion(this) )
			--m_iLinkedSectors;
	}
}

bool CRegionBase::MakeRegionName()
{
	ADDTOCALLSTACK("CRegionBase::MakeRegionName");
	if ( m_pDefName )
		return true;

	LPCTSTR pszName = GetName();
	GETNONWHITESPACE(pszName);
	if ( !strnicmp("the ", pszName, 4) )
		pszName += 4;
	else if ( !strnicmp("a ", pszName, 2) )
		pszName += 2;
	else if ( !strnicmp("an ", pszName, 3) )
		pszName += 3;
	else if ( !strnicmp("ye ", pszName, 3) )
		pszName += 3;

	TCHAR *pszTemp = Str_GetTemp();
	strcpy(pszTemp, "a_");

	TCHAR ch;
	TCHAR *pszDef = pszTemp + 2;
	for ( ; *pszName; ++pszName )
	{
		if ( !strnicmp(" of ", pszName, 4) || !strnicmp(" in ", pszName, 4) )
		{
			pszName += 4;
			continue;
		}
		else if ( !strnicmp(" the ", pszName, 5) )
		{
			pszName += 5;
			continue;
		}

		ch = *pszName;
		if ( (ch == ' ') || (ch == '\t') || (ch == '-') )
			ch = '_';
		else if ( !isalnum(ch) )
			continue;

		// Collapse multiple spaces together
		if ( (ch == '_') && (*(pszDef - 1) == '_') )
			continue;

		*pszDef = static_cast<TCHAR>(tolower(ch));
		++pszDef;
	}
	*pszDef = '_';
	*++pszDef = '\0';

	size_t iMax = g_Cfg.m_RegionDefs.GetCount();
	size_t iLen = strlen(pszTemp);
	int iVar = 1;
	for ( size_t i = 0; i < iMax; ++i )
	{
		CRegionBase *pRegion = g_Cfg.m_RegionDefs.GetAt(i);
		if ( !pRegion )
			continue;

		LPCTSTR pszKey = pRegion->GetResourceName();
		if ( !pszKey )
			continue;

		// Is this a similar key?
		if ( strnicmp(pszTemp, pszKey, iLen) != 0 )
			continue;

		// Skip underscores
		pszKey = pszKey + iLen;
		while ( *pszKey == '_' )
			++pszKey;

		// Is this a subsequent key with a number? Get the highest + 1
		if ( IsStrNumericDec(pszKey) )
		{
			int iVarThis = ATOI(pszKey);
			if ( iVarThis >= iVar )
				iVar = iVarThis + 1;
		}
		else
			++iVar;
	}

	snprintf(pszDef, THREAD_STRING_LENGTH, "%d", iVar);
	SetResourceName(pszTemp);
	return true;
}

bool CRegionBase::AddRegionRect(const CRectMap &rect)
{
	ADDTOCALLSTACK("CRegionBase::AddRegionRect");
	if ( !rect.IsValid() )
		return false;

	return CGRegion::AddRegionRect(rect);
}

void CRegionBase::SetName(LPCTSTR pszName)
{
	ADDTOCALLSTACK("CRegionBase::SetName");
	m_sName = ((pszName == NULL) || (pszName[0] == '%')) ? g_Serv.GetName() : pszName;
}

void CRegionBase::SetModified(int iModFlag)
{
	ADDTOCALLSTACK("CRegionBase::SetModified");
	if ( m_iLinkedSectors )
		m_iModified |= iModFlag;
}

bool CRegionBase::IsGuarded() const
{
	ADDTOCALLSTACK("CRegionBase::IsGuarded");
	return (IsFlag(REGION_FLAG_GUARDED) && !IsFlag(REGION_FLAG_SAFE));
}

bool CRegionBase::CheckAntiMagic(SPELL_TYPE spell) const
{
	ADDTOCALLSTACK("CRegionBase::CheckAntiMagic");
	if ( !IsFlag(REGION_ANTIMAGIC_ALL|REGION_ANTIMAGIC_RECALL_IN|REGION_ANTIMAGIC_RECALL_OUT|REGION_ANTIMAGIC_GATE|REGION_ANTIMAGIC_TELEPORT|REGION_ANTIMAGIC_DAMAGE|REGION_FLAG_SHIP) )
		return false;
	if ( IsFlag(REGION_ANTIMAGIC_ALL) )
		return true;
	if ( IsFlag(REGION_ANTIMAGIC_RECALL_IN|REGION_FLAG_SHIP) && ((spell == SPELL_Mark) || (spell == SPELL_Gate_Travel)) )
		return true;
	if ( IsFlag(REGION_ANTIMAGIC_RECALL_OUT) && ((spell == SPELL_Recall) || (spell == SPELL_Gate_Travel) || (spell == SPELL_Mark)) )
		return true;
	if ( IsFlag(REGION_ANTIMAGIC_GATE) && (spell == SPELL_Gate_Travel) )
		return true;
	if ( IsFlag(REGION_ANTIMAGIC_TELEPORT) && (spell == SPELL_Teleport) )
		return true;
	if ( IsFlag(REGION_ANTIMAGIC_DAMAGE) )
	{
		const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
		if ( pSpellDef && pSpellDef->IsSpellType(SPELLFLAG_HARM) )
			return true;
	}
	return false;
}

const LPCTSTR CRegionBase::sm_szTrigName[RTRIG_QTY + 1] =	// static
{
	"@AAAUNUSED",
	"@CLIPERIODIC",
	"@ENTER",
	"@EXIT",
	"@REGPERIODIC",
	"@STEP",
	NULL
};

TRIGRET_TYPE CRegionBase::OnRegionTrigger(CTextConsole *pSrc, RTRIG_TYPE iAction)
{
	ADDTOCALLSTACK("CRegionBase::OnRegionTrigger");

	for ( size_t i = 0; i < m_Events.GetCount(); ++i )
	{
		CResourceLink *pLink = m_Events[i];
		if ( !pLink || (pLink->GetResType() != RES_REGIONTYPE) || !pLink->HasTrigger(iAction) )
			continue;

		CResourceLock s;
		if ( pLink->ResourceLock(s) )
		{
			if ( CScriptObj::OnTriggerScript(s, sm_szTrigName[iAction], pSrc) == TRIGRET_RET_TRUE )
				return TRIGRET_RET_TRUE;
		}
	}

	// EVENTSREGION triggers (constant events of regions set from sphere.ini)
	TRIGRET_TYPE tr;
	for ( size_t i = 0; i < g_Cfg.m_pEventsRegionLink.GetCount(); ++i )
	{
		CResourceLink *pLink = g_Cfg.m_pEventsRegionLink[i];
		if ( !pLink || (pLink->GetResType() != RES_REGIONTYPE) || !pLink->HasTrigger(iAction) )
			continue;

		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;

		tr = CScriptObj::OnTriggerScript(s, sm_szTrigName[iAction], pSrc);
		if ( (tr != TRIGRET_RET_FALSE) && (tr != TRIGRET_RET_DEFAULT) )
			return tr;
	}

	return TRIGRET_RET_DEFAULT;
}

void CRegionBase::r_WriteBase(CScript &s)
{
	ADDTOCALLSTACK("CRegionBase::r_WriteBase");
	if ( GetName() && GetName()[0] )
		s.WriteKey("NAME", GetName());

	if ( !m_sGroup.IsEmpty() )
		s.WriteKey("GROUP", static_cast<LPCTSTR>(m_sGroup));

	CRegionBase::r_WriteBody(s, "");

	if ( m_pt.IsValidPoint() )
		s.WriteKey("P", m_pt.WriteUsed());
	else if ( m_pt.m_map )
		s.WriteKeyVal("MAP", m_pt.m_map);

	size_t iCount = GetRegionRectCount();
	for ( size_t i = 0; i < iCount; ++i )
		s.WriteKey("RECT", GetRegionRect(i).Write());
}

void CRegionBase::r_WriteBody(CScript &s, LPCTSTR pszPrefix)
{
	ADDTOCALLSTACK("CRegionBase::r_WriteBody");
	TemporaryString pszBuffer;
	if ( GetRegionFlags() )
	{
		snprintf(pszBuffer, THREAD_STRING_LENGTH, "%sFLAGS", pszPrefix);
		s.WriteKeyHex(pszBuffer, GetRegionFlags());
	}

	if ( m_Events.GetCount() > 0 )
	{
		CGString sVal;
		m_Events.WriteResourceRefList(sVal);
		snprintf(pszBuffer, THREAD_STRING_LENGTH, "%sEVENTS", pszPrefix);
		s.WriteKey(pszBuffer, sVal);
	}

	m_BaseDefs.r_WritePrefix(s);

	snprintf(pszBuffer, THREAD_STRING_LENGTH, "%sTAG", pszPrefix);
	m_TagDefs.r_WritePrefix(s, pszBuffer);
}

void CRegionBase::r_WriteModified(CScript &s)
{
	ADDTOCALLSTACK("CRegionBase::r_WriteModified");
	if ( m_iModified & REGMOD_NAME )
		s.WriteKey("NAME", GetName());

	if ( m_iModified & REGMOD_GROUP )
		s.WriteKey("GROUP", m_sGroup);

	if ( m_iModified & REGMOD_FLAGS )
		s.WriteKeyHex("FLAGS", GetRegionFlags());

	if ( m_iModified & REGMOD_EVENTS )
	{
		CGString sVal;
		m_Events.WriteResourceRefList(sVal);
		s.WriteKey("EVENTS", sVal);
	}
}

void CRegionBase::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CRegionBase::r_Write");
	s.WriteSection("ROOMDEF %s", GetResourceName());
	r_WriteBase(s);
}

enum RC_TYPE
{
	RC_ANNOUNCE,
	RC_ARENA,
	RC_BUILDABLE,
	RC_CLIENTS,
	RC_COLDCHANCE,
	RC_EVENTS,
	RC_FLAGS,
	RC_GATE,
	RC_GROUP,
	RC_GUARDED,
	RC_ISEVENT,
	RC_MAGIC,
	RC_MAP,
	RC_MARK,
	RC_NAME,
	RC_NOBUILD,
	RC_NODECAY,
	RC_NOPVP,
	RC_P,
	RC_RAINCHANCE,
	RC_RECALL,
	RC_RECALLIN,
	RC_RECALLOUT,
	RC_RECT,
	RC_SAFE,
	RC_TAG,
	RC_TAG0,
	RC_TAGAT,
	RC_TAGCOUNT,
	RC_TYPE,
	RC_UID,
	RC_UNDERGROUND,
	RC_QTY
};

const LPCTSTR CRegionBase::sm_szLoadKeys[RC_QTY + 1] =	// static
{
	"ANNOUNCE",
	"ARENA",
	"BUILDABLE",
	"CLIENTS",
	"COLDCHANCE",
	"EVENTS",
	"FLAGS",
	"GATE",
	"GROUP",
	"GUARDED",
	"ISEVENT",
	"MAGIC",
	"MAP",
	"MARK",
	"NAME",
	"NOBUILD",
	"NODECAY",
	"NOPVP",
	"P",
	"RAINCHANCE",
	"RECALL",
	"RECALLIN",
	"RECALLOUT",
	"RECT",
	"SAFE",
	"TAG",
	"TAG0",
	"TAGAT",
	"TAGCOUNT",
	"TYPE",
	"UID",
	"UNDERGROUND",
	NULL
};

bool CRegionBase::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CRegionBase::r_WriteVal");
	EXC_TRY("WriteVal");

	int index = FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
		return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);

	switch ( index )
	{
		case RC_ANNOUNCE:
			sVal.FormatVal(IsFlag(REGION_FLAG_ANNOUNCE));
			break;
		case RC_ARENA:
			sVal.FormatVal(IsFlag(REGION_FLAG_ARENA));
			break;
		case RC_BUILDABLE:
			sVal.FormatVal(!IsFlag(REGION_FLAG_NOBUILDING));
			break;
		case RC_CLIENTS:
		{
			size_t iClients = 0;
			for ( int i = 0; ; ++i )
			{
				CSector *pSector = GetSector(i);
				if ( !pSector )
					break;
				iClients += pSector->m_Chars_Active.HasClients();
			}
			sVal.FormatUVal(iClients);
			break;
		}
		case RC_EVENTS:
			m_Events.WriteResourceRefList(sVal);
			break;
		case RC_FLAGS:
			sVal.FormatHex(GetRegionFlags());
			break;
		case RC_GATE:
			sVal.FormatVal(!IsFlag(REGION_ANTIMAGIC_GATE));
			break;
		case RC_GROUP:
			sVal = m_sGroup;
			break;
		case RC_GUARDED:
			sVal.FormatVal(IsFlag(REGION_FLAG_GUARDED));
			break;
		case RC_ISEVENT:
			if ( pszKey[7] != '.' )
				return false;
			pszKey += 8;
			sVal.FormatVal(m_Events.ContainsResourceName(RES_EVENTS, pszKey));
			return true;
		case RC_MAGIC:
			sVal.FormatVal(!IsFlag(REGION_ANTIMAGIC_ALL));
			break;
		case RC_MAP:
			sVal.FormatVal(m_pt.m_map);
			break;
		case RC_MARK:
		case RC_RECALLIN:
			sVal.FormatVal(!IsFlag(REGION_ANTIMAGIC_RECALL_IN));
			break;
		case RC_NAME:
			// The previous name was really the DEFNAME?
			sVal = GetName();
			break;
		case RC_NOBUILD:
			sVal.FormatVal(IsFlag(REGION_FLAG_NOBUILDING));
			break;
		case RC_NODECAY:
			sVal.FormatVal(IsFlag(REGION_FLAG_NODECAY));
			break;
		case RC_NOPVP:
			sVal.FormatVal(IsFlag(REGION_FLAG_NO_PVP));
			break;
		case RC_P:
			sVal = m_pt.WriteUsed();
			break;
		case RC_RECALL:
		case RC_RECALLOUT:
			sVal.FormatVal(!IsFlag(REGION_ANTIMAGIC_RECALL_OUT));
			break;
		case RC_RECT:
		{
			pszKey += 4;
			if ( *pszKey == '\0' )
			{
				sVal.FormatVal(static_cast<long>(m_Rects.GetCount() + 1));
				return true;
			}

			SKIP_SEPARATORS(pszKey);
			size_t iRect = Exp_GetVal(pszKey);
			if ( iRect == 0 )
			{
				sVal = m_rectUnion.Write();
				return true;
			}

			--iRect;
			sVal = m_Rects.IsValidIndex(iRect) ? m_Rects[iRect].Write() : "0";
			return true;
		}
		case RC_SAFE:
			sVal.FormatVal(IsFlag(REGION_FLAG_SAFE));
			break;
		case RC_TAG:
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, false);
			return true;
		case RC_TAG0:
			if ( pszKey[4] != '.' )
				return false;
			pszKey += 5;
			sVal = m_TagDefs.GetKeyStr(pszKey, true);
			return true;
		case RC_TAGAT:
		{
			pszKey += 5;
			if ( *pszKey == '.' )
			{
				SKIP_SEPARATORS(pszKey);
				size_t iCount = static_cast<size_t>(Exp_GetVal(pszKey));
				if ( iCount >= m_TagDefs.GetCount() )
					return false;

				CVarDefCont *pTagAt = m_TagDefs.GetAt(iCount);
				if ( !pTagAt )
					return false;

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
		case RC_TAGCOUNT:
			sVal.FormatVal(static_cast<long>(m_TagDefs.GetCount()));
			break;
		case RC_TYPE:
		{
			const CItemBase *pItemBase = NULL;
			const CItem *pItem = GetResourceID().ItemFind();
			if ( pItem )
				pItemBase = pItem->Item_GetDef();

			sVal = pItemBase ? pItemBase->GetResourceName() : "";
			break;
		}
		case RC_UID:
			if ( pszKey[3] == '.' )		// allow use of UID.x.KEY on the REGION object
				return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);

			sVal.FormatHex(GetResourceID());
			break;
		case RC_UNDERGROUND:
			sVal.FormatVal(IsFlag(REGION_FLAG_UNDERGROUND));
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<LPCTSTR>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegionBase::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CRegionBase::r_LoadVal");
	EXC_TRY("LoadVal");

	if ( s.IsKeyHead("TAG.", 4) )
	{
		SetModified(REGMOD_TAGS);
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey() + 4, fQuoted, s.GetArgStr(&fQuoted), false);
		return true;
	}
	else if ( s.IsKeyHead("TAG0.", 5) )
	{
		SetModified(REGMOD_TAGS);
		bool fQuoted = false;
		m_TagDefs.SetStr(s.GetKey() + 5, fQuoted, s.GetArgStr(&fQuoted), true);
		return true;
	}

	int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case RC_ANNOUNCE:
			ToggleRegionFlags(REGION_FLAG_ANNOUNCE, (s.GetArgVal() != 0));
			break;
		case RC_ARENA:
			ToggleRegionFlags(REGION_FLAG_ARENA, (s.GetArgVal() != 0));
			break;
		case RC_BUILDABLE:
			ToggleRegionFlags(REGION_FLAG_NOBUILDING, (s.GetArgVal() != 0));
			break;
		case RC_COLDCHANCE:
			SendSectorsVerb(s.GetKey(), s.GetArgStr(), &g_Serv);
			break;
		case RC_EVENTS:
			SetModified(REGMOD_EVENTS);
			return m_Events.r_LoadVal(s, RES_REGIONTYPE);
		case RC_FLAGS:
			m_dwFlags = (s.GetArgVal() & ~REGION_FLAG_SHIP)|(m_dwFlags & REGION_FLAG_SHIP);
			SetModified(REGMOD_FLAGS);
			break;
		case RC_GATE:
			ToggleRegionFlags(REGION_ANTIMAGIC_GATE, (s.GetArgVal() != 0));
			break;
		case RC_GROUP:
			m_sGroup = s.GetArgStr();
			SetModified(REGMOD_GROUP);
			break;
		case RC_GUARDED:
			ToggleRegionFlags(REGION_FLAG_GUARDED, (s.GetArgVal() != 0));
			break;
		case RC_MAGIC:
			ToggleRegionFlags(REGION_ANTIMAGIC_ALL, (s.GetArgVal() != 0));
			break;
		case RC_MAP:
			m_pt.m_map = static_cast<BYTE>(s.GetArgVal());
			break;
		case RC_MARK:
		case RC_RECALLIN:
			ToggleRegionFlags(REGION_ANTIMAGIC_RECALL_IN, (s.GetArgVal() != 0));
			break;
		case RC_NAME:
			SetName(s.GetArgStr());
			SetModified(REGMOD_NAME);
			break;
		case RC_NOBUILD:
			ToggleRegionFlags(REGION_FLAG_NOBUILDING, (s.GetArgVal() != 0));
			break;
		case RC_NODECAY:
			ToggleRegionFlags(REGION_FLAG_NODECAY, (s.GetArgVal() != 0));
			break;
		case RC_NOPVP:
			ToggleRegionFlags(REGION_FLAG_NO_PVP, (s.GetArgVal() != 0));
			break;
		case RC_P:
			m_pt.Read(s.GetArgStr());
			break;
		case RC_RAINCHANCE:
			SendSectorsVerb(s.GetKey(), s.GetArgStr(), &g_Serv);
			break;
		case RC_RECALL:
		case RC_RECALLOUT:
			ToggleRegionFlags(REGION_ANTIMAGIC_RECALL_OUT, (s.GetArgVal() != 0));
			break;
		case RC_RECT:
		{
			if ( m_iLinkedSectors )
				return false;

			CRectMap rect;
			rect.Read(s.GetArgStr());
			return AddRegionRect(rect);
		}
		case RC_SAFE:
			ToggleRegionFlags(REGION_FLAG_SAFE, (s.GetArgVal() != 0));
			break;
		case RC_UNDERGROUND:
			ToggleRegionFlags(REGION_FLAG_UNDERGROUND, (s.GetArgVal() != 0));
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

enum RV_TYPE
{
	RV_ALLCLIENTS,
	RV_TAGLIST,
	RV_QTY
};

const LPCTSTR CRegionBase::sm_szVerbKeys[RV_QTY + 1] =
{
	"ALLCLIENTS",
	"TAGLIST",
	NULL
};

enum SEV_TYPE
{
	#define ADD(a,b) SEV_##a,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	SEV_QTY
};

bool CRegionBase::r_Verb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CRegionBase::r_Verb");
	EXC_TRY("Verb");

	LPCTSTR pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	int index = FindTableSorted(pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1);
	if ( index < 0 )
	{
		index = FindTableSorted(s.GetKey(), CSector::sm_szVerbKeys, SEV_QTY);
		if ( index >= 0 )
			return SendSectorsVerb(s.GetKey(), s.GetArgRaw(), pSrc);
	}

	switch ( index )
	{
		case RV_ALLCLIENTS:
		{
			ClientIterator it;
			for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
			{
				CChar *pChar = pClient->GetChar();
				if ( !pChar || (pChar->m_pArea != this) )
					continue;

				CScript script(s.GetArgStr());
				pChar->r_Verb(script, pSrc);
			}
			return true;
		}
		case RV_TAGLIST:
		{
			m_TagDefs.DumpKeys(pSrc, "TAG.");
			return true;
		}
		default:
			break;
	}

	return CScriptObj::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegionBase::SendSectorsVerb(LPCTSTR pszVerb, LPCTSTR pszArgs, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CRegionBase::SendSectorsVerb");
	// Send a command to all CSectors in this region

	bool fRet = false;
	for ( int i = 0; ; ++i )
	{
		CSector *pSector = GetSector(i);
		if ( !pSector )
			break;

		if ( IsOverlapped(pSector->GetRect()) )
		{
			CScript script(pszVerb, pszArgs);
			fRet |= pSector->r_Verb(script, pSrc);
		}
	}
	return fRet;
}

///////////////////////////////////////////////////////////
// CRegionWorld

CRegionWorld::CRegionWorld(RESOURCE_ID rid, LPCTSTR pszName) : CRegionBase(rid, pszName)
{
}

const CRandGroupDef *CRegionWorld::FindNaturalResource(/*IT_TYPE*/ int iType) const
{
	ADDTOCALLSTACK("CRegionWorld::FindNaturalResource");
	// Find natural resources assinged to this region

	for ( size_t i = 0; i < m_Events.GetCount(); ++i )
	{
		CResourceLink *pLink = m_Events[i];
		if ( !pLink || (pLink->GetResType() != RES_REGIONTYPE) )
			continue;

		if ( pLink->GetResPage() == iType )
			return dynamic_cast<const CRandGroupDef *>(pLink);
	}
	return NULL;
}

bool CRegionWorld::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CRegionWorld::r_GetRef");
	return CRegionBase::r_GetRef(pszKey, pRef);
}

void CRegionWorld::r_WriteBody(CScript &s, LPCTSTR pszPrefix)
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteBody");
	CRegionBase::r_WriteBody(s, pszPrefix);
}

void CRegionWorld::r_WriteModified(CScript &s)
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteModified");
	CRegionBase::r_WriteModified(s);

	if ( m_iModified & REGMOD_TAGS )
		m_TagDefs.r_WritePrefix(s, "TAG", "GUARDOWNER");
}

void CRegionWorld::r_Write(CScript &s)
{
	ADDTOCALLSTACK_INTENSIVE("CRegionWorld::r_Write");
	s.WriteSection("AREADEF %s", GetResourceName());
	r_WriteBase(s);
}

enum RWC_TYPE
{
	RWC_DEFNAME,
	RWC_REGION,
	RWC_RESOURCES,
	RWC_QTY
};

const LPCTSTR CRegionWorld::sm_szLoadKeys[RWC_QTY + 1] =	// static
{
	"DEFNAME",
	"REGION",
	"RESOURCES",
	NULL
};

bool CRegionWorld::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteVal");
	EXC_TRY("WriteVal");

	switch ( FindTableHeadSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case RWC_DEFNAME:
			sVal = GetResourceName();
			break;
		case RWC_RESOURCES:
			m_Events.WriteResourceRefList(sVal);
			break;
		case RWC_REGION:
		{
			if ( pszKey[6] && (pszKey[6] != '.') )
				return false;

			CRegionWorld *pRegion = dynamic_cast<CRegionWorld *>(m_pt.GetRegion(REGION_TYPE_AREA));
			if ( !pszKey[6] )
			{
				sVal.FormatVal(pRegion ? 1 : 0);
				return true;
			}

			pszKey += 7;
			if ( pRegion && m_pt.GetRegion(REGION_TYPE_MULTI) )
				return pRegion->r_WriteVal(pszKey, sVal, pSrc);

			return r_WriteVal(pszKey, sVal, pSrc);
		}
		default:
			return CRegionBase::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<LPCTSTR>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegionWorld::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CRegionWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	// Load region values from scripts
	switch ( FindTableHeadSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case RWC_DEFNAME:
			return SetResourceName(s.GetArgStr());
		case RWC_RESOURCES:
			SetModified(REGMOD_EVENTS);
			return m_Events.r_LoadVal(s, RES_REGIONTYPE);
		default:
			break;
	}
	return CRegionBase::r_LoadVal(s);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CRegionWorld::r_Verb(CScript &s, CTextConsole *pSrc)	// execute command from script
{
	ADDTOCALLSTACK("CRegionWorld::r_Verb");
	EXC_TRY("Verb");

	return CRegionBase::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

///////////////////////////////////////////////////////////
// CTeleport

CTeleport::CTeleport(TCHAR *pszArgs)
{
	TCHAR *ppArgs[4];
	size_t iArgQty = Str_ParseCmds(pszArgs, ppArgs, COUNTOF(ppArgs), "=");
	if ( iArgQty < 2 )
		g_Log.EventError("Bad teleport def\n");

	if ( iArgQty >= 1 )
		Read(ppArgs[0]);
	else
		InitPoint();

	if ( iArgQty >= 2 )
		m_ptDst.Read(ppArgs[1]);
	else
		m_ptDst.InitPoint();

	m_fNPC = (iArgQty >= 4) ? (ATOI(ppArgs[3]) != 0) : false;
}

bool CTeleport::RealizeTeleport()
{
	ADDTOCALLSTACK("CTeleport::RealizeTeleport");
	if ( !IsValidPoint() || !m_ptDst.IsValidPoint() )
	{
		g_Log.EventError("Bad teleport coords %s\n", WriteUsed());
		return false;
	}
	CSector *pSector = GetSector();
	return pSector ? pSector->AddTeleport(this) : false;
}
