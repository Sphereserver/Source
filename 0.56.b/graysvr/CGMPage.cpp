//
// CGMPage.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

//////////////////////////////////////////////////////////////////
// -CGMPage

CGMPage::CGMPage( LPCTSTR pszAccount ) :
	m_sAccount( pszAccount )
{
	m_pGMClient = NULL;
	m_timePage = CServTime::GetCurrentTime();
	// Put at the end of the list.
	g_World.m_GMPages.InsertTail( this );
}

CGMPage::~CGMPage()
{
	ClearGMHandler();
}

INT64 CGMPage::GetAge() const
{
	ADDTOCALLSTACK("CGMPage::GetAge");
	// How old in seconds.
	return( (-g_World.GetTimeDiff( m_timePage )) / TICK_PER_SEC );
}

void CGMPage::ClearGMHandler()
{
	if ( m_pGMClient != NULL)	// break the link to the client.
	{
		ASSERT( m_pGMClient->m_pGMPage == this );
		m_pGMClient->m_pGMPage = NULL;
	}

	m_pGMClient = NULL;
}

void CGMPage::SetGMHandler(CClient* pClient)
{
	if ( m_pGMClient == pClient )
		return;

	if ( m_pGMClient != NULL)	// break the link to the previous client.
		ClearGMHandler();

	m_pGMClient = pClient;
	if ( m_pGMClient != NULL )
		m_pGMClient->m_pGMPage = this;
}

void CGMPage::r_Write( CScript & s ) const
{
	ADDTOCALLSTACK_INTENSIVE("CGMPage::r_Write");
	s.WriteSection( "GMPAGE %s", GetName());
	s.WriteKey( "REASON", GetReason());
	s.WriteKeyHex( "TIME", GetAge());
	s.WriteKey( "P", m_ptOrigin.WriteUsed());
}

enum GC_TYPE
{
	GC_ACCOUNT,
	GC_P,
	GC_REASON,
	GC_STATUS,
	GC_TIME,
	GC_QTY
};

LPCTSTR const CGMPage::sm_szLoadKeys[GC_QTY+1] =
{
	"ACCOUNT",
	"P",
	"REASON",
	"STATUS",
	"TIME",
	NULL
};

bool CGMPage::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CGMPage::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
	case GC_ACCOUNT:
		sVal = GetName();
		break;
	case GC_P:	// "P"
		sVal = m_ptOrigin.WriteUsed();
		break;
	case GC_REASON:	// "REASON"
		sVal = GetReason();
		break;
	case GC_STATUS:
		sVal = GetAccountStatus();
		break;
	case GC_TIME:	// "TIME"
		sVal.FormatLLHex( GetAge() );
		break;
	default:
		return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CGMPage::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CGMPage::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
	case GC_P:	// "P"
		m_ptOrigin.Read( s.GetArgStr());
		break;
	case GC_REASON:	// "REASON"
		SetReason( s.GetArgStr());
		break;
	case GC_TIME:	// "TIME"
		m_timePage = CServTime::GetCurrentTime() - ( s.GetArgVal() * TICK_PER_SEC );
		break;
	default:
		return( CScriptObj::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

CAccountRef CGMPage::FindAccount() const
{
	ADDTOCALLSTACK("CGMPage::FindAccount");
	return( g_Accounts.Account_Find( m_sAccount ));
}

LPCTSTR CGMPage::GetAccountStatus() const
{
	ADDTOCALLSTACK("CGMPage::GetAccountStatus");
	CClient * pClient = FindAccount()->FindClient();
	if ( pClient==NULL )
		return "OFFLINE";
	else if ( pClient->GetChar() == NULL )
		return "LOGIN";
	else
		return pClient->GetChar()->GetName();
}

