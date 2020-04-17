#include "graysvr.h"	// predef header

CGMPage::CGMPage(LPCTSTR pszAccount)
{
	m_pClientHandling = NULL;
	m_sAccount = pszAccount;
	m_uidChar = UID_UNUSED;
	m_pt.InitPoint();
	m_sReason = NULL;
	m_time = CServTime::GetCurrentTime();

	g_World.m_GMPages.InsertTail(this);		// put it at the end of the list
}

CGMPage::~CGMPage()
{
	ClearHandler();
}

void CGMPage::SetHandler(CClient *pClient)
{
	ADDTOCALLSTACK("CGMPage::SetHandler");
	if ( !pClient || (pClient == m_pClientHandling) )
		return;

	if ( m_pClientHandling )
		ClearHandler();

	m_pClientHandling = pClient;
	m_pClientHandling->m_pGMPage = this;
}

void CGMPage::ClearHandler()
{
	ADDTOCALLSTACK("CGMPage::ClearHandler");
	if ( m_pClientHandling )
	{
		m_pClientHandling->m_pGMPage = NULL;
		m_pClientHandling = NULL;
	}
}

void CGMPage::r_Write(CScript &s) const
{
	ADDTOCALLSTACK_INTENSIVE("CGMPage::r_Write");
	s.WriteSection("GMPAGE %s", GetName());
	s.WriteKeyHex("CHARUID", m_uidChar);
	s.WriteKey("P", m_pt.WriteUsed());
	s.WriteKey("REASON", m_sReason);
	s.WriteKeyVal("TIME", -g_World.GetTimeDiff(m_time) / TICK_PER_SEC);
}

enum GC_TYPE
{
	GC_ACCOUNT,
	GC_CHARUID,
	GC_DELETE,
	GC_HANDLED,
	GC_P,
	GC_REASON,
	GC_TIME,
	GC_QTY
};

const LPCTSTR CGMPage::sm_szLoadKeys[GC_QTY + 1] =
{
	"ACCOUNT",
	"CHARUID",
	"DELETE",
	"HANDLED",
	"P",
	"REASON",
	"TIME",
	NULL
};

bool CGMPage::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CGMPage::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case GC_ACCOUNT:
			sVal = GetName();
			break;
		case GC_CHARUID:
			sVal.FormatHex(m_uidChar);
			break;
		case GC_HANDLED:
			sVal.FormatHex((m_pClientHandling && m_pClientHandling->GetChar()) ? m_pClientHandling->GetChar()->GetUID() : static_cast<CGrayUID>(UID_CLEAR));
			break;
		case GC_P:
			sVal = m_pt.WriteUsed();
			break;
		case GC_REASON:
			sVal = m_sReason;
			break;
		case GC_TIME:
			sVal.FormatLLVal(-g_World.GetTimeDiff(m_time) / TICK_PER_SEC);
			break;
		default:
			return CScriptObj::r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CGMPage::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CGMPage::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case GC_CHARUID:
			m_uidChar = s.GetArgVal();
			break;
		case GC_DELETE:
			delete this;
			break;
		case GC_HANDLED:
		{
			CChar *pChar = static_cast<CGrayUID>(s.GetArgVal()).CharFind();
			if ( pChar && pChar->m_pClient )
				SetHandler(pChar->m_pClient);
			else
				ClearHandler();
			break;
		}
		case GC_P:
			m_pt.Read(s.GetArgStr());
			break;
		case GC_REASON:
			m_sReason = s.GetArgStr();
			break;
		case GC_TIME:
			m_time = CServTime::GetCurrentTime() - (s.GetArgLLVal() * TICK_PER_SEC);
			break;
		default:
			return CScriptObj::r_LoadVal(s);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}
