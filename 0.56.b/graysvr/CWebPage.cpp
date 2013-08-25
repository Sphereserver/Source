//
// CWebPage.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version
#include "../common/CFileList.h"
#include "../network/network.h"
#include "../network/send.h"

enum WV_TYPE
{
	WV_CLIENTLIST,
	WV_GMPAGELIST,
	WV_GUILDLIST,
	WV_TOWNLIST,
	WV_WEBPAGE,
	WV_QTY
};

LPCTSTR const CWebPageDef::sm_szVerbKeys[WV_QTY+1] =
{
	"CLIENTLIST",	// make a table of all the clients.
	"GMPAGELIST",	// make a table of the gm pages.
	"GUILDLIST",	// make a table of the guilds.
	"TOWNLIST",		// make a table of the towns.
	"WEBPAGE",		// feed a web page to the source caller
	NULL
};

//********************************************************
// -CFileConsole

class CFileConsole : public CTextConsole
{
public:
	static const char *m_sClassName;
	CFileText m_FileOut;

public:
	virtual PLEVEL_TYPE GetPrivLevel() const
	{
		return PLEVEL_Admin;
	}
	virtual LPCTSTR GetName() const
	{
		return "WebFile";
	}
	virtual void SysMessage( LPCTSTR pszMessage ) const
	{
		if ( pszMessage == NULL || ISINTRESOURCE(pszMessage))
			return;
		(const_cast <CFileConsole*>(this))->m_FileOut.WriteString(pszMessage);
	}

public:
	CFileConsole() { };

private:
	CFileConsole(const CFileConsole& copy);
	CFileConsole& operator=(const CFileConsole& other);
};

//********************************************************
// -CWebPageDef

int CWebPageDef::sm_iListIndex;

CWebPageDef::CWebPageDef( RESOURCE_ID rid ) :
	CResourceLink( rid )
{
	// Web page m_sWebPageFilePath
	m_type = WEBPAGE_TEMPLATE;
	m_privlevel=PLEVEL_Guest;

	m_timeNextUpdate.Init();
	m_iUpdatePeriod = 2*60*TICK_PER_SEC;
	m_iUpdateLog = 0;

	// default source name
	if ( rid.GetResIndex())
	{
		m_sSrcFilePath.Format( GRAY_FILE "statusbase%d.htm", rid.GetResIndex());
	}
	else
	{
		m_sSrcFilePath = GRAY_FILE "statusbase.htm";
	}
}

enum WC_TYPE
{
	WC_PLEVEL,
	WC_WEBPAGEFILE,
	WC_WEBPAGELOG,
	WC_WEBPAGESRC,
	WC_WEBPAGEUPDATE,
	WC_QTY
};

LPCTSTR const CWebPageDef::sm_szLoadKeys[WC_QTY+1] =
{
	"PLEVEL",				// What priv level does one need to be to view this page.
	"WEBPAGEFILE",			// For periodic generated pages.
	"WEBPAGELOG",			// daily log a copy of this page.
	"WEBPAGESRC",			// the source name of the web page.
	"WEBPAGEUPDATE",		// how often to generate a page ? (seconds)
	NULL
};

bool CWebPageDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CWebPageDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case WC_PLEVEL:
			sVal.FormatVal( m_privlevel );
			break;
		case WC_WEBPAGEFILE:
			sVal = m_sDstFilePath;
			break;
		case WC_WEBPAGELOG:
			sVal.FormatVal( m_iUpdateLog );
			break;
		case WC_WEBPAGESRC:
			sVal = m_sSrcFilePath;
			break;
		case WC_WEBPAGEUPDATE:	// (seconds)
			sVal.FormatVal( m_iUpdatePeriod / TICK_PER_SEC );
			break;
		default:
			return( g_Serv.r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CWebPageDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case WC_PLEVEL:
			m_privlevel = static_cast<PLEVEL_TYPE>(s.GetArgVal());
			break;
		case WC_WEBPAGEFILE:
			m_sDstFilePath = s.GetArgStr();
			break;
		case WC_WEBPAGELOG:
			m_iUpdateLog = s.GetArgVal();
			break;
		case WC_WEBPAGESRC:
			return SetSourceFile( s.GetArgStr(), NULL );
		case WC_WEBPAGEUPDATE:	// (seconds)
			m_iUpdatePeriod = s.GetArgVal() * TICK_PER_SEC;
			if ( m_iUpdatePeriod && m_type == WEBPAGE_TEXT )
			{
				m_type = WEBPAGE_TEMPLATE;
			}
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

bool CWebPageDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
	ADDTOCALLSTACK("CWebPageDef::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	sm_iListIndex = 0;
	TCHAR *pszTmp2 = Str_GetTemp();

	WV_TYPE iHeadKey = (WV_TYPE) FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	switch ( iHeadKey )
	{
		case WV_WEBPAGE:
			{
			// serv a web page to the pSrc
				CClient * pClient = dynamic_cast <CClient *>(pSrc);
				if ( pClient == NULL )
					return( false );
				return ServPage( pClient, s.GetArgStr(), NULL );
			}
	
		case WV_CLIENTLIST:
			{
				ClientIterator it;
				for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
				{
					CChar * pChar = pClient->GetChar();
					if ( pChar == NULL )
						continue;
					if (( pChar->IsStatFlag(STATF_Insubstantial) ) && (!pChar->IsStatFlag(STATF_DEAD)))
						continue;
	
					sm_iListIndex++;
	
					LPCTSTR pszArgs = s.GetArgStr();
					if ( pszArgs[0] == '\0' )
						pszArgs = "<tr><td>%NAME%</td><td>%REGION.NAME%</td></tr>\n";
					strcpy( pszTmp2, pszArgs );
					pChar->ParseText( Str_MakeFiltered( pszTmp2 ), &g_Serv, 1 );
					pSrc->SysMessage( pszTmp2 );
				}
			}
			break;
	
		case WV_GUILDLIST:
		case WV_TOWNLIST:
			{
				if ( !s.HasArgs() )
					return false;

				IT_TYPE	needtype = ( iHeadKey == WV_GUILDLIST ) ? IT_STONE_GUILD : IT_STONE_TOWN;

				for ( size_t i = 0; i < g_World.m_Stones.GetCount(); i++ )
				{
					CItemStone * pStone = g_World.m_Stones[i];
					if ( !pStone || !pStone->IsType(needtype) )
						continue;
	
					sm_iListIndex++;
	
					strcpy(pszTmp2, s.GetArgStr());
					pStone->ParseText(Str_MakeFiltered(pszTmp2), &g_Serv, 1);
					pSrc->SysMessage(pszTmp2);
				}
			}
			break;

		case WV_GMPAGELIST:
			{
				if ( ! s.HasArgs())
					return( false );
				CGMPage * pPage = STATIC_CAST <CGMPage*>( g_World.m_GMPages.GetHead());
				for ( ; pPage!=NULL; pPage = pPage->GetNext())
				{
					sm_iListIndex++;
					strcpy( pszTmp2, s.GetArgStr() );
					pPage->ParseText( Str_MakeFiltered( pszTmp2 ), &g_Serv, 1 );
					pSrc->SysMessage( pszTmp2 );
				}
			}
			break;
	
		default:
			return( CResourceLink::r_Verb(s,pSrc));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::WebPageUpdate( bool fNow, LPCTSTR pszDstName, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CWebPageDef::WebPageUpdate");
	// Generate the status web pages.
	// Read in the Base status page "*STATUSBASE.HTM"
	// Filter the XML type tags.
	// Output the status page "*.HTM"
	// Server name
	// Server email
	// Number of clients, items, NPC's

	if ( ! fNow )
	{
		if ( m_iUpdatePeriod <= 0 )
			return false;
		if ( CServTime::GetCurrentTime() < m_timeNextUpdate )
			return true;	// should stilll be valid
	}

	ASSERT(pSrc);
	m_timeNextUpdate = CServTime::GetCurrentTime() + m_iUpdatePeriod;
	if ( pszDstName == NULL )
	{
		pszDstName = m_sDstFilePath;
	}

	if ( m_type != WEBPAGE_TEMPLATE ||
		*pszDstName == '\0' ||
		m_sSrcFilePath.IsEmpty())
		return false;

	CScript FileRead;
	if ( ! FileRead.Open( m_sSrcFilePath, OF_READ|OF_TEXT|OF_DEFAULTMODE ))
	{
		return false;
	}

	CScriptFileContext context( &FileRead );	// set this as the context.

	CFileConsole FileOut;
	if ( ! FileOut.m_FileOut.Open( pszDstName, OF_WRITE|OF_TEXT ))
	{
		DEBUG_ERR(( "Can't open web page output '%s'\n", static_cast<LPCTSTR>(pszDstName) ));
		return false;
	}

	bool fScriptMode = false;

	while ( FileRead.ReadTextLine( false ))
	{
		TCHAR *pszTmp = Str_GetTemp();
		strcpy( pszTmp, FileRead.GetKey());

		TCHAR * pszHead = strstr( pszTmp, "<script language=\"Sphere\">" );
		if ( pszHead != NULL )
		{
			// Deal with the stuff preceding the scripts.
			*pszHead = '\0';
			pszHead += 26;
			ParseText( pszTmp, pSrc, 1 );
			FileOut.SysMessage( pszTmp );
			fScriptMode = true;
		}
		else
		{
			pszHead = pszTmp;
		}

		// Look for the end of </script>
		if ( fScriptMode )
		{
			GETNONWHITESPACE(pszHead);
			TCHAR * pszFormat = pszHead;

			pszHead = strstr( pszFormat, "</script>" );
			if ( pszHead != NULL )
			{
				*pszHead = '\0';
				pszHead += 9;
				fScriptMode = false;
			}

			if ( pszFormat[0] != '\0' )
			{
				// Allow if/then logic ??? OnTriggerRun( CScript &s, TRIGRUN_SINGLE_EXEC, &FileOut )
				CScript script( pszFormat );
				if ( ! r_Verb( script, &FileOut ))
				{
					DEBUG_ERR(( "Web page source format error '%s'\n", static_cast<LPCTSTR>(pszTmp) ));
					continue;
				}
			}

			if ( fScriptMode )
				continue;
		}

		// Look for stuff we can displace here. %STUFF%
		ParseText( pszHead, pSrc, 1 );
		FileOut.SysMessage( pszHead );
	}

	return( true );
}

void CWebPageDef::WebPageLog()
{
	ADDTOCALLSTACK("CWebPageDef::WebPageLog");
	if ( ! m_iUpdateLog || ! m_iUpdatePeriod )
		return;
	if ( m_type != WEBPAGE_TEMPLATE )
		return;

	CFileText FileRead;
	if ( ! FileRead.Open( m_sDstFilePath, OF_READ|OF_TEXT ))
		return;

	LPCTSTR pszExt = FileRead.GetFileExt();

	TCHAR szName[ _MAX_PATH ];
	strcpy( szName, m_sDstFilePath );
	szName[ m_sDstFilePath.GetLength() - strlen(pszExt) ] = '\0';

	CGTime datetime = CGTime::GetCurrentTime();

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s%d%02d%02d%s", szName, datetime.GetYear()%100, datetime.GetMonth(), datetime.GetDay(), pszExt);

	CFileText FileTest;
	if ( FileTest.Open(pszTemp, OF_READ|OF_TEXT) )
		return;

	// Copy it.
	WebPageUpdate(true, pszTemp, &g_Serv);
}

LPCTSTR const CWebPageDef::sm_szPageExt[] =
{
	".bmp",
	".gif",
	".htm",
	".html",
	".jpeg",
	".jpg",
	".js",
	".txt",
};

bool CWebPageDef::SetSourceFile( LPCTSTR pszName, CClient * pClient )
{
	ADDTOCALLSTACK("CWebPageDef::SetSourceFile");
	static WEBPAGE_TYPE const sm_szPageExtType[] =
	{
		WEBPAGE_BMP,
		WEBPAGE_GIF,
		WEBPAGE_TEMPLATE,
		WEBPAGE_TEXT,
		WEBPAGE_JPG,
		WEBPAGE_JPG,
		WEBPAGE_TEXT,
		WEBPAGE_TEXT
	};

	// attempt to set this to a source file.
	// test if it exists.
	size_t iLen = strlen( pszName );
	if ( iLen <= 3 )
		return( false );

	LPCTSTR pszExt = CGFile::GetFilesExt( pszName );
	if ( pszExt == NULL || pszExt[0] == '\0' )
		return( false );

	int iType = FindTableSorted( pszExt, sm_szPageExt, COUNTOF( sm_szPageExt ));
	if ( iType < 0 )
		return false;
	m_type = sm_szPageExtType[iType];

	if ( pClient == NULL )
	{
		// this is being set via the Script files.
		// make sure the file is valid
		CScript FileRead;
		if ( ! g_Cfg.OpenResourceFind( FileRead, pszName ))
		{
			DEBUG_ERR(( "Can't open web page input '%s'\n", static_cast<LPCTSTR>(m_sSrcFilePath) ));
			return( false );
		}
		m_sSrcFilePath = FileRead.GetFilePath();
	}
	else
	{
		if ( *pszName == '/' )
			pszName ++;
		if ( strstr( pszName, ".." ))	// this sort of access is not allowed.
			return( false );
		if ( strstr( pszName, "\\\\" ))	// this sort of access is not allowed.
			return( false );
		if ( strstr( pszName, "//" ))	// this sort of access is not allowed.
			return( false );
		m_sSrcFilePath = CGFile::GetMergedFileName( g_Cfg.m_sSCPBaseDir, pszName );
	}

	return( true );
}

bool CWebPageDef::IsMatch( LPCTSTR pszMatch ) const
{
	ADDTOCALLSTACK("CWebPageDef::IsMatch");
	if ( pszMatch == NULL )	// match all.
		return( true );

	LPCTSTR pszDstName = GetDstName();
	LPCTSTR pszTry;

	if ( pszDstName[0] )
	{
		// Page is generated periodically. match the output name
		pszTry = CScript::GetFilesTitle( pszDstName );
	}
	else
	{
		// match the source name
		pszTry = CScript::GetFilesTitle( GetName());
	}

	return( ! strcmpi( pszTry, pszMatch ));
}

LPCTSTR const CWebPageDef::sm_szPageType[WEBPAGE_QTY+1] =
{
	"text/html",		// WEBPAGE_TEMPLATE
	"text/html",		// WEBPAGE_TEXT
	"image/x-xbitmap",	// WEBPAGE_BMP,
	"image/gif",		// WEBPAGE_GIF,
	"image/jpeg",		// WEBPAGE_JPG,
	NULL,				// WEBPAGE_QTY
};

LPCTSTR const CWebPageDef::sm_szTrigName[WTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@Load",
	NULL,
};

int CWebPageDef::ServPageRequest( CClient * pClient, LPCTSTR pszURLArgs, CGTime * pdateIfModifiedSince )
{
	ADDTOCALLSTACK("CWebPageDef::ServPageRequest");
	UNREFERENCED_PARAMETER(pszURLArgs);
	// Got a web page request from the client.
	// ARGS:
	//  pszURLArgs = args on the URL line ex. http://www.hostname.com/dir?args
	// RETURN:
	//  HTTP error code = 0=200 page was served.

	ASSERT(pClient);

	if ( HasTrigger(WTRIG_Load))
	{
		CResourceLock s;
		if ( ResourceLock(s))
		{
			if (CScriptObj::OnTriggerScript( s, sm_szTrigName[WTRIG_Load], pClient, NULL ) == TRIGRET_RET_TRUE)
				return( 0 );	// Block further action.
		}
	}

	if ( m_privlevel )
	{
		if ( pClient->GetAccount() == NULL )
			return 401;	// Authorization required
		if ( pClient->GetPrivLevel() < m_privlevel )
			return 403;	// Forbidden
	}

	CGTime datetime = CGTime::GetCurrentTime();

	LPCTSTR pszName;
	bool fGenerate = false;

	if ( m_type == WEBPAGE_TEMPLATE ) // my version of cgi
	{
		pszName = GetDstName();
		if ( pszName[0] == '\0' )
		{
			pszName = "temppage.htm";
			fGenerate = true;
		}
		else
		{
			fGenerate = ! m_iUpdatePeriod;
		}

		// The page must be generated on demand.
		if ( ! WebPageUpdate( fGenerate, pszName, pClient ))
			return 500;
	}
	else
	{
		pszName = GetName();
	}

	// Get proper Last-Modified: time.
	time_t dateChange;
	DWORD dwSize;
	if ( ! CFileList::ReadFileInfo( pszName, dateChange, dwSize ))
	{
		return 500;
	}

	const char *sDate = datetime.FormatGmt(NULL);	// current date.

	if ( ! fGenerate &&
		! pdateIfModifiedSince &&
		pdateIfModifiedSince->IsTimeValid() &&
		dateChange <= pdateIfModifiedSince->GetTime() )
	{
		TCHAR *pszTemp = Str_GetTemp();
		sprintf(pszTemp, 
			"HTTP/1.1 304 Not Modified\r\nDate: %s\r\nServer: " GRAY_TITLE " V " GRAY_VERSION "\r\nContent-Length: 0\r\n\r\n", sDate);

		new PacketWeb(pClient, (BYTE*)pszTemp, strlen(pszTemp));
		return(0);
	}

	// Now serve up the page.
	CGFile FileRead;
	if ( ! FileRead.Open( pszName, OF_READ|OF_BINARY ))
		return 500;

	// Send the header first.
	TCHAR szTmp[8*1024];
	size_t iLen = sprintf(szTmp,
		"HTTP/1.1 200 OK\r\n" // 100 Continue
		"Date: %s\r\n"
		"Server: " GRAY_TITLE " V " GRAY_VERSION "\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Type: %s\r\n",
		static_cast<LPCTSTR>(sDate),
		static_cast<LPCTSTR>(sm_szPageType[m_type]) // type of the file. image/gif, image/x-xbitmap, image/jpeg
		);

	if ( m_type == WEBPAGE_TEMPLATE )
		iLen += sprintf(szTmp + iLen, "Expires: 0\r\n");
	else
		iLen += sprintf(szTmp + iLen, "Last-Modified: %s\r\n",  CGTime(dateChange).FormatGmt(NULL));

	iLen += sprintf( szTmp + iLen,
		"Content-Length: %lu\r\n"
		"\r\n",
		dwSize
		);

	PacketWeb packet;
	packet.setData((BYTE*)szTmp, iLen);
	packet.send(pClient);

	for (;;)
	{
		iLen = FileRead.Read( szTmp, sizeof( szTmp ) );
		if ( iLen <= 0 )
			break;
		packet.setData((BYTE*)szTmp, iLen);
		packet.send(pClient);
		dwSize -= iLen;
		if ( iLen < sizeof( szTmp ) )
		{
			// memset( szTmp+iLen, 0, sizeof(szTmp)-iLen );
			break;
		}
	}
	return 0;
}

static int GetHexDigit( TCHAR ch )
{
	if ( IsDigit( ch ))
		return( ch - '0' );

	ch = toupper(ch);
	if ( ch > 'F' || ch <'A' )
		return( -1 );

	return( ch - ( 'A' - 10 ));
}

static int HtmlDeCode( TCHAR * pszDst, LPCTSTR pszSrc )
{
	int i = 0;
	for (;;)
	{
		TCHAR ch = *pszSrc++;
		if ( ch == '+' )
		{
			ch = ' ';
		}
		else if ( ch == '%' )
		{
			ch = *pszSrc++;
			int iVal = GetHexDigit(ch);
			if ( ch )
			{
				ch = *pszSrc++;
				if ( ch )
				{
					ch = iVal*0x10 + GetHexDigit(ch);
					if (static_cast<unsigned char>(ch) == 0xa0)
						ch = '\0';
				}
			}
		}
		*pszDst++ = ch;
		if ( ch == 0 )
			break;
		i++;
	}
	return( i );
}

bool CWebPageDef::ServPagePost( CClient * pClient, LPCTSTR pszURLArgs, TCHAR * pContentData, int iContentLength )
{
	ADDTOCALLSTACK("CWebPageDef::ServPagePost");
	UNREFERENCED_PARAMETER(pszURLArgs);
	// RETURN: true = this was the page of interest.

	ASSERT(pClient);

	if ( pContentData == NULL || iContentLength <= 0 )
		return( false );
	if ( ! HasTrigger(XTRIG_UNKNOWN))	// this form has no triggers.
		return( false );

	// Parse the data.
	pContentData[iContentLength] = 0;
	TCHAR * ppArgs[64];
	size_t iArgs = Str_ParseCmds(pContentData, ppArgs, COUNTOF(ppArgs), "&");
	if (( iArgs <= 0 ) || ( iArgs >= 63 ))
		return false;

	// T or TXT or TEXT = the text fields.
	// B or BTN or BUTTON = the buttons
	// C or CHK or CHECK = the check boxes

	CDialogResponseArgs resp;
	DWORD dwButtonID = ULONG_MAX;
	for ( size_t i = 0; i < iArgs; i++ )
	{
		TCHAR * pszNum = ppArgs[i];
		while ( IsAlpha(*pszNum) )
			pszNum++;

		int iNum = ATOI(pszNum);
		while ( *pszNum )
		{
			if ( *pszNum == '=' )
			{
				pszNum++;
				break;
			}
			pszNum++;
		}
		switch ( toupper(ppArgs[i][0]) )
		{
			case 'B':
				dwButtonID = iNum;
				break;
			case 'C':
				if ( !iNum )
					continue;
				if ( ATOI(pszNum) )
				{
					resp.m_CheckArray.Add( iNum );
				}
				break;
			case 'T':
				if ( iNum > 0 )
				{
					TCHAR *pszData = Str_GetTemp();
					HtmlDeCode( pszData, pszNum );
					resp.AddText( iNum, pszData );
				}
				break;
		}
	}

	// Use the data in RES_WEBPAGE block.
	CResourceLock s;
	if ( !ResourceLock(s) )
		return false;

	// Find the correct entry point.
	while ( s.ReadKeyParse())
	{
		if ( !s.IsKeyHead("ON", 2) || ( (DWORD)s.GetArgVal() != dwButtonID ))
			continue;
		OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pClient, &resp);
		return true;
	}

	// Put up some sort of failure page ?

	return( false );
}

bool CWebPageDef::ServPage( CClient * pClient, TCHAR * pszPage, CGTime * pdateIfModifiedSince )	// static
{
	ADDTOCALLSTACK("CWebPageDef::ServPage");
	// make sure this is a valid format for the request.

	TCHAR szPageName[_MAX_PATH];
	Str_GetBare( szPageName, pszPage, sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`" );

	int iError = 404;
	CWebPageDef * pWebPage = g_Cfg.FindWebPage(szPageName);
	if ( pWebPage )
	{
		iError = pWebPage->ServPageRequest(pClient, szPageName, pdateIfModifiedSince);
		if ( ! iError )
			return true;
	}

	// Is it a file in the Script directory ?
	if ( iError == 404 )
	{
		const RESOURCE_ID ridjunk( RES_UNKNOWN, 1 );
		CWebPageDef tmppage( ridjunk );
		if ( tmppage.SetSourceFile( szPageName, pClient ))
		{
			if ( !tmppage.ServPageRequest(pClient, szPageName, pdateIfModifiedSince) )
				return true;
		}
	}

	// Can't find it !?
	// just take the default page. or have a custom 404 page ?

	pClient->m_Targ_Text = pszPage;

	TCHAR	*pszTemp = Str_GetTemp();
	sprintf(pszTemp, GRAY_FILE "%d.htm", iError);
	pWebPage = g_Cfg.FindWebPage(pszTemp);
	if ( pWebPage )
	{
		if ( ! pWebPage->ServPageRequest( pClient, pszPage, NULL ))
			return true;
	}

	// Hmm we should do something !!!?
	// Try to give a reasonable default error msg.

	LPCTSTR pszErrText;
	switch (iError)
	{
		case 401: pszErrText = "Authorization Required"; break;
		case 403: pszErrText = "Forbidden"; break;
		case 404: pszErrText = "Object Not Found"; break;
		case 500: pszErrText = "Internal Server Error"; break;
		default: pszErrText = "Unknown Error"; break;
	}

	CGTime datetime = CGTime::GetCurrentTime();
	const char *sDate = datetime.FormatGmt(NULL);
	CGString sMsgHead;
	CGString sText;

	sText.Format(
		"<html><head><title>Error %d</title>"
		"<meta name=robots content=noindex>"
		"</head><body>"
		"<h2>HTTP Error %d</h2><p><strong>%d %s</strong></p>"
		"<p>The " GRAY_TITLE " server cannot deliver the file or script you asked for.</p>"
		"<p>Please contact the server's administrator if this problem persists.</p>"
		"</body></html>",
		iError,
		iError,
		iError,
		static_cast<LPCTSTR>(pszErrText));

	sMsgHead.Format(
		"HTTP/1.1 %d %s\r\n"
		"Date: %s\r\n"
		"Server: " GRAY_TITLE " V " GRAY_VERSION "\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n%s",
		iError, static_cast<LPCTSTR>(pszErrText),
		static_cast<LPCTSTR>(sDate),
		sText.GetLength(),
		static_cast<LPCTSTR>(sText));

	new PacketWeb(pClient, reinterpret_cast<const BYTE *>(sMsgHead.GetPtr()), sMsgHead.GetLength());
	return false;
}

