#include "graysvr.h"	// predef header
#include "../common/CFileList.h"
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

const LPCTSTR CWebPageDef::sm_szVerbKeys[WV_QTY + 1] =
{
	"CLIENTLIST",		// Generate HTML table of clients
	"GMPAGELIST",		// Generate HTML table of GM pages
	"GUILDLIST",		// Generate HTML table of guildstones
	"TOWNLIST",			// Generate HTML table of townstones
	"WEBPAGE",			// Feed a webpage to the source caller
	NULL
};

///////////////////////////////////////////////////////////
// CWebPageDef

int CWebPageDef::sm_iListIndex;

CWebPageDef::CWebPageDef(RESOURCE_ID rid) : CResourceLink(rid)
{
	// Web page m_sWebPageFilePath
	m_type = WEBPAGE_TEMPLATE;
	m_privlevel = PLEVEL_Player;
	m_timeNextUpdate.Init();
	m_iUpdatePeriod = 2 * 60 * TICK_PER_SEC;
	m_iUpdateLog = 0;

	// Default source name
	if ( rid.GetResIndex() )
		m_sSrcFilePath.Format(SPHERE_FILE "statusbase%d.htm", rid.GetResIndex());
	else
		m_sSrcFilePath = SPHERE_FILE "statusbase.htm";
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

const LPCTSTR CWebPageDef::sm_szLoadKeys[WC_QTY + 1] =
{
	"PLEVEL",				// Required plevel to view this page
	"WEBPAGEFILE",			// Determines where the page is saved
	"WEBPAGELOG",			// Create a daily copy of the page
	"WEBPAGESRC",			// Determines what page is used as base
	"WEBPAGEUPDATE",		// How often in seconds the page is updated (0 = never)
	NULL
};

bool CWebPageDef::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CWebPageDef::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case WC_PLEVEL:
			sVal.FormatVal(m_privlevel);
			break;
		case WC_WEBPAGEFILE:
			sVal = m_sDstFilePath;
			break;
		case WC_WEBPAGELOG:
			sVal.FormatVal(m_iUpdateLog);
			break;
		case WC_WEBPAGESRC:
			sVal = m_sSrcFilePath;
			break;
		case WC_WEBPAGEUPDATE:
			sVal.FormatVal(m_iUpdatePeriod / TICK_PER_SEC);
			break;
		default:
			return g_Serv.r_WriteVal(pszKey, sVal, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::r_LoadVal(CScript &s) // Load an item Script
{
	ADDTOCALLSTACK("CWebPageDef::r_LoadVal");
	EXC_TRY("LoadVal");
	switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
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
			return SetSourceFile(s.GetArgStr(), NULL);
		case WC_WEBPAGEUPDATE:
			m_iUpdatePeriod = s.GetArgVal() * TICK_PER_SEC;
			if ( m_iUpdatePeriod && (m_type == WEBPAGE_TEXT) )
				m_type = WEBPAGE_TEMPLATE;
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

bool CWebPageDef::r_Verb(CScript &s, CTextConsole *pSrc)	// some command on this object as a target
{
	ADDTOCALLSTACK("CWebPageDef::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);

	sm_iListIndex = 0;
	TCHAR *pszTemp = Str_GetTemp();

	WV_TYPE index = static_cast<WV_TYPE>(FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1));
	switch ( index )
	{
		case WV_CLIENTLIST:
		{
			if ( !s.HasArgs() )
				return false;

			ClientIterator it;
			for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
			{
				CChar *pChar = pClient->GetChar();
				if ( !pChar || (pChar->IsStatFlag(STATF_Insubstantial) && (pClient->GetPrivLevel() > PLEVEL_Player)) )
					continue;

				strncpy(pszTemp, s.GetArgStr(), MAX_NAME_SIZE - 1);
				pChar->ParseText(Str_MakeFiltered(pszTemp), &g_Serv, 1);
				pSrc->SysMessage(pszTemp);
				++sm_iListIndex;
			}
			break;
		}
		case WV_GMPAGELIST:
		{
			if ( !s.HasArgs() )
				return false;

			for ( CGMPage *pGMPage = dynamic_cast<CGMPage *>(g_World.m_GMPages.GetHead()); pGMPage != NULL; pGMPage = pGMPage->GetNext() )
			{
				strncpy(pszTemp, s.GetArgStr(), THREAD_TSTRING_STORAGE - 1);
				pGMPage->ParseText(Str_MakeFiltered(pszTemp), &g_Serv, 1);
				pSrc->SysMessage(pszTemp);
				++sm_iListIndex;
			}
			break;
		}
		case WV_GUILDLIST:
		case WV_TOWNLIST:
		{
			if ( !s.HasArgs() )
				return false;

			IT_TYPE	type = (index == WV_GUILDLIST) ? IT_STONE_GUILD : IT_STONE_TOWN;
			for ( size_t i = 0; i < g_World.m_Stones.GetCount(); ++i )
			{
				CItemStone *pStone = g_World.m_Stones[i];
				if ( !pStone || !pStone->IsType(type) )
					continue;

				strncpy(pszTemp, s.GetArgStr(), MAX_ITEM_NAME_SIZE - 1);
				pStone->ParseText(Str_MakeFiltered(pszTemp), &g_Serv, 1);
				pSrc->SysMessage(pszTemp);
				++sm_iListIndex;
			}
			break;
		}
		case WV_WEBPAGE:
		{
			CClient *pClient = dynamic_cast<CClient *>(pSrc);
			if ( !pClient )
				return false;

			return ServPage(pClient, s.GetArgStr(), NULL);
		}
		default:
			return CResourceLink::r_Verb(s, pSrc);
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CWebPageDef::WebPageUpdate(bool fNow, LPCTSTR pszDstName, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CWebPageDef::WebPageUpdate");
	// Read the webpage source and generate the final webpage

	if ( !fNow )
	{
		if ( m_iUpdatePeriod <= 0 )
			return false;
		if ( CServTime::GetCurrentTime() < m_timeNextUpdate )
			return true;
	}

	ASSERT(pSrc);
	m_timeNextUpdate = CServTime::GetCurrentTime() + m_iUpdatePeriod;
	if ( !pszDstName )
		pszDstName = m_sDstFilePath;

	if ( (m_type != WEBPAGE_TEMPLATE) || !pszDstName || m_sSrcFilePath.IsEmpty() )
		return false;

	CScript FileRead;
	if ( !FileRead.Open(m_sSrcFilePath, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		return false;

	CScriptFileContext context(&FileRead);	// set this as the context

	CFileConsole FileOut;
	if ( !FileOut.m_FileOut.Open(pszDstName, OF_WRITE|OF_TEXT) )
	{
		DEBUG_ERR(("Can't open web page output '%s'\n", pszDstName));
		return false;
	}

	bool fScriptMode = false;
	while ( FileRead.ReadTextLine(false) )
	{
		TCHAR *pszTemp = Str_GetTemp();
		strcpy(pszTemp, FileRead.GetKey());

		TCHAR *pszHead = strstr(pszTemp, "<script language=\"Sphere\">");
		if ( pszHead )
		{
			// Deal with the stuff preceding the scripts
			*pszHead = '\0';
			pszHead += 26;
			ParseText(pszTemp, pSrc, 1);
			FileOut.SysMessage(pszTemp);
			fScriptMode = true;
		}
		else
			pszHead = pszTemp;

		// Look for the end of </script>
		if ( fScriptMode )
		{
			GETNONWHITESPACE(pszHead);
			TCHAR *pszFormat = pszHead;

			pszHead = strstr(pszFormat, "</script>");
			if ( pszHead )
			{
				*pszHead = '\0';
				pszHead += 9;
				fScriptMode = false;
			}

			if ( pszFormat[0] != '\0' )
			{
				// Allow if/else logic? OnTriggerRun(CScript &s, TRIGRUN_SINGLE_EXEC, &FileOut)
				CScript script(pszFormat);
				if ( !r_Verb(script, &FileOut) )
				{
					DEBUG_ERR(("Web page source format error '%s'\n", static_cast<LPCTSTR>(pszTemp)));
					continue;
				}
			}

			if ( fScriptMode )
				continue;
		}

		// Parse the text looking for %ARGS% stuff
		ParseText(pszHead, pSrc, 1);
		FileOut.SysMessage(pszHead);
	}
	return true;
}

void CWebPageDef::WebPageLog()
{
	ADDTOCALLSTACK("CWebPageDef::WebPageLog");
	// Periodically generate the page log
	if ( !m_iUpdateLog || !m_iUpdatePeriod || (m_type != WEBPAGE_TEMPLATE) )
		return;

	CFileText FileRead;
	if ( !FileRead.Open(m_sDstFilePath, OF_READ|OF_TEXT) )
		return;

	LPCTSTR pszExt = FileRead.GetFileExt();

	TCHAR szName[_MAX_PATH];
	strncpy(szName, m_sDstFilePath, sizeof(szName) - 1);
	szName[m_sDstFilePath.GetLength() - strlen(pszExt)] = '\0';

	CGTime timeCurrent = CGTime::GetCurrentTime();

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s%d-%02d-%02d%s", szName, timeCurrent.GetYear(), timeCurrent.GetMonth(), timeCurrent.GetDay(), pszExt);

	CFileText FileTest;
	if ( FileTest.Open(pszTemp, OF_READ|OF_TEXT) )
		return;

	WebPageUpdate(true, pszTemp, &g_Serv);
}

const LPCTSTR CWebPageDef::sm_szPageExt[] =
{
	".bmp",
	".gif",
	".htm",
	".html",
	".jpeg",
	".jpg",
	".js",
	".png",
	".txt"
};

const LPCTSTR CWebPageDef::sm_szPageType[WEBPAGE_QTY + 1] =
{
	"text/html",		// WEBPAGE_TEMPLATE
	"text/html",		// WEBPAGE_TEXT
	"image/bmp",		// WEBPAGE_BMP
	"image/gif",		// WEBPAGE_GIF
	"image/jpeg",		// WEBPAGE_JPG
	"image/png",		// WEBPAGE_PNG
	NULL				// WEBPAGE_QTY
};

bool CWebPageDef::SetSourceFile(LPCTSTR pszName, CClient *pClient)
{
	ADDTOCALLSTACK("CWebPageDef::SetSourceFile");
	// Set source file for this webpage

	LPCTSTR pszExt = CGFile::GetFilesExt(pszName);
	if ( !pszExt )
		return false;

	int index = FindTableSorted(pszExt, sm_szPageExt, COUNTOF(sm_szPageExt));
	if ( index < 0 )
		return false;

	static const WEBPAGE_TYPE sm_szPageExtType[] =
	{
		WEBPAGE_BMP,
		WEBPAGE_GIF,
		WEBPAGE_TEMPLATE,
		WEBPAGE_TEXT,
		WEBPAGE_JPG,
		WEBPAGE_JPG,
		WEBPAGE_TEXT,
		WEBPAGE_PNG,
		WEBPAGE_TEXT
	};
	m_type = sm_szPageExtType[index];

	if ( pClient )
	{
		if ( *pszName == '/' )
			++pszName;
		if ( strstr(pszName, "..") || strstr(pszName, "//") || strstr(pszName, "\\\\") )	// this sort of access is not allowed
			return false;
		m_sSrcFilePath = CGFile::GetMergedFileName(g_Cfg.m_sSCPBaseDir, pszName);
	}
	else
	{
		CScript FileRead;
		if ( !g_Cfg.OpenResourceFind(FileRead, pszName) )
		{
			DEBUG_ERR(("Can't open web page input '%s'\n", static_cast<LPCTSTR>(m_sSrcFilePath)));
			return false;
		}
		m_sSrcFilePath = FileRead.GetFilePath();
	}
	return true;
}

bool CWebPageDef::IsMatch(LPCTSTR pszMatch) const
{
	ADDTOCALLSTACK("CWebPageDef::IsMatch");
	if ( !pszMatch )	// match all
		return true;

	LPCTSTR pszDstName = GetDstName();
	LPCTSTR pszCompare = CScript::GetFilesTitle(pszDstName ? pszDstName : GetName());
	return !strcmpi(pszCompare, pszMatch);
}

const LPCTSTR CWebPageDef::sm_szTrigName[WTRIG_QTY + 1] =	// static
{
	"@AAAUNUSED",
	"@Load",
	NULL
};

int CWebPageDef::ServPageRequest(CClient *pClient, LPCTSTR pszURLArgs, CGTime *pTimeLastModified)
{
	ADDTOCALLSTACK("CWebPageDef::ServPageRequest");
	UNREFERENCED_PARAMETER(pszURLArgs);
	// Client is requesting an webpage
	// ARGS:
	//   pszURLArgs = args on the URL (eg http://www.hostname.com/dir?args)
	// RETURN:
	//   HTML status code
	//     0			= action blocked by script
	//     100 ~ 102	= information
	//     200 ~ 226	= success
	//     300 ~ 308	= redirection
	//     400 ~ 451	= client error
	//     500 ~ 511	= server error
	ASSERT(pClient);

	if ( HasTrigger(WTRIG_Load) )
	{
		CResourceLock s;
		if ( ResourceLock(s) )
		{
			if ( CScriptObj::OnTriggerScript(s, sm_szTrigName[WTRIG_Load], pClient) == TRIGRET_RET_TRUE )
				return 0;	// action blocked by script
		}
	}

	if ( m_privlevel )
	{
		if ( !pClient->m_pAccount )
			return 401;		// Unauthorized
		if ( pClient->GetPrivLevel() < m_privlevel )
			return 403;		// Forbidden
	}

	LPCTSTR pszName;
	bool fGenerate = false;

	if ( m_type == WEBPAGE_TEMPLATE )	// my version of CGI
	{
		pszName = GetDstName();
		if ( !pszName )
		{
			pszName = "temppage.htm";
			fGenerate = true;
		}
		else
			fGenerate = !m_iUpdatePeriod;

		if ( !WebPageUpdate(fGenerate, pszName, pClient) )
			return 500;		// Internal server error
	}
	else
		pszName = GetName();

	CGTime timeCurrent = CGTime::GetCurrentTime();
	time_t timeFileLastModified;

	DWORD dwSize;
	if ( !CFileList::ReadFileInfo(pszName, timeFileLastModified, dwSize) )
		return 500;		// Internal server error

	if ( !fGenerate && pTimeLastModified->IsTimeValid() && (pTimeLastModified->GetTime() <= timeFileLastModified) )
	{
		TCHAR *pszTemp = Str_GetTemp();
		sprintf(pszTemp,
			"HTTP/1.1 304 Not Modified\r\n"
			"Date: %s\r\n"
			"Server: " SPHERE_TITLE_VER "\r\n"
			"Content-Length: 0\r\n"
			"\r\n",
			timeCurrent.FormatGmt(NULL)
		);
		new PacketWeb(pClient, reinterpret_cast<const BYTE *>(pszTemp), strlen(pszTemp));
		return 304;		// Not modified
	}

	CGFile FileRead;
	if ( !FileRead.Open(pszName, OF_READ|OF_BINARY) )
		return 500;		// Internal server error

	// Send HTTP header
	TCHAR szTemp[8 * 1024];
	size_t iLen = sprintf(szTemp,
		"HTTP/1.1 200 OK\r\n"
		"Date: %s\r\n"
		"Server: " SPHERE_TITLE_VER "\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Type: %s\r\n",
		timeCurrent.FormatGmt(NULL),
		sm_szPageType[m_type]
	);

	if ( m_type == WEBPAGE_TEMPLATE )
		iLen += sprintf(szTemp + iLen, "Expires: 0\r\n");
	else
		iLen += sprintf(szTemp + iLen, "Last-Modified: %s\r\n", CGTime(timeFileLastModified).FormatGmt(NULL));

	iLen += sprintf(szTemp + iLen, "Content-Length: %" FMTDWORD "\r\n\r\n", dwSize);

	PacketWeb packet;
	packet.setData(reinterpret_cast<const BYTE *>(szTemp), iLen);
	packet.send(pClient);

	// Send webpage content
	for (;;)
	{
		iLen = FileRead.Read(szTemp, sizeof(szTemp));
		if ( iLen <= 0 )
			break;

		packet.setData(reinterpret_cast<const BYTE *>(szTemp), iLen);
		packet.send(pClient);
		if ( iLen < sizeof(szTemp) )
			break;
	}
	return 200;		// OK
}

static int GetHexDigit(TCHAR ch)
{
	if ( IsDigit(ch) )
		return ch - '0';

	ch = static_cast<TCHAR>(toupper(ch));
	if ( (ch > 'F') || (ch < 'A') )
		return -1;

	return ch - ('A' - 10);
}

static void HtmlDecode(TCHAR *pszDst, LPCTSTR pszSrc)
{
	for (;;)
	{
		TCHAR ch = *pszSrc++;
		if ( ch == '+' )
			ch = ' ';
		else if ( ch == '%' )
		{
			ch = *pszSrc++;
			int iVal = GetHexDigit(ch);
			if ( ch )
			{
				ch = *pszSrc++;
				if ( ch )
				{
					ch = static_cast<TCHAR>((iVal * 0x10) + GetHexDigit(ch));
					if ( static_cast<int>(ch) == 0xA0 )
						ch = '\0';
				}
			}
		}

		*pszDst++ = ch;
		if ( ch == 0 )
			return;
	}
}

bool CWebPageDef::ServPagePost(CClient *pClient, TCHAR *pszContent, int iContentLength)
{
	ADDTOCALLSTACK("CWebPageDef::ServPagePost");
	// Client sent POST data to server
	// RETURN:
	//  true = this was the page of interest
	ASSERT(pClient);

	if ( !pszContent || (iContentLength <= 0) || !HasTrigger(XTRIG_UNKNOWN) )
		return false;

	// Parse the data
	pszContent[iContentLength] = 0;
	TCHAR *ppArgs[64];
	size_t iArgQty = Str_ParseCmds(pszContent, ppArgs, COUNTOF(ppArgs), "&");
	if ( (iArgQty <= 0) || (iArgQty >= 63) )
		return false;

	CDialogResponseArgs resp;
	DWORD dwButtonID = DWORD_MAX;

	for ( size_t i = 0; i < iArgQty; ++i )
	{
		TCHAR *pszNum = ppArgs[i];
		while ( IsAlpha(*pszNum) )
			++pszNum;

		WORD wID = static_cast<WORD>(ATOI(pszNum));
		while ( *pszNum )
		{
			if ( *pszNum == '=' )
			{
				++pszNum;
				break;
			}
			++pszNum;
		}

		switch ( toupper(ppArgs[i][0]) )
		{
			case 'B':	// button
			{
				dwButtonID = wID;
				break;
			}
			case 'C':	// checkbox
			{
				if ( !wID )
					continue;
				if ( ATOI(pszNum) )
					resp.m_CheckArray.Add(wID);
				break;
			}
			case 'T':	// text field
			{
				if ( wID > 0 )
				{
					TCHAR *pszText = Str_GetTemp();
					HtmlDecode(pszText, pszNum);
					resp.AddText(wID, pszText);
				}
				break;
			}
		}
	}

	// Use the data in RES_WEBPAGE block
	CResourceLock s;
	if ( !ResourceLock(s) )
		return false;

	// Find the correct entry point
	while ( s.ReadKeyParse() )
	{
		if ( !s.IsKeyHead("ON", 2) || (static_cast<DWORD>(s.GetArgVal()) != dwButtonID) )
			continue;

		OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pClient, &resp);
		return true;
	}
	return false;
}

bool CWebPageDef::ServPage(CClient *pClient, TCHAR *pszPageName, CGTime *pTimeLastModified)	// static
{
	ADDTOCALLSTACK("CWebPageDef::ServPage");
	// Serve the requested webpage to client

	TCHAR szPageName[_MAX_PATH];
	Str_GetBare(szPageName, pszPageName, sizeof(szPageName), "!\"#$%&()*,:;<=>?[]^{|}-+'`");

	int iStatusCode = 404;
	CWebPageDef *pWebPage = g_Cfg.FindWebPage(szPageName);
	if ( pWebPage )
	{
		iStatusCode = pWebPage->ServPageRequest(pClient, szPageName, pTimeLastModified);
		if ( iStatusCode < 400 )
			return true;
	}

	// Webpage not found, try one of the options below
	// 1) Check if it is a file in the scripts folder
	if ( iStatusCode == 404 )
	{
		const RESOURCE_ID rid(RES_UNKNOWN, 1);
		CWebPageDef webpageTemp(rid);
		if ( webpageTemp.SetSourceFile(szPageName, pClient) )
		{
			if ( webpageTemp.ServPageRequest(pClient, szPageName, pTimeLastModified) < 400 )
				return true;
		}
	}

	// 2) Check if there's a custom webpage for this specific error
	pClient->m_Targ_Text = pszPageName;
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, SPHERE_FILE "%d.htm", iStatusCode);
	pWebPage = g_Cfg.FindWebPage(pszTemp);
	if ( pWebPage && (pWebPage->ServPageRequest(pClient, pszPageName, NULL) < 400) )
		return true;

	// 3) Use generic error page
	LPCTSTR pszError;
	switch ( iStatusCode )
	{
		case 401:	pszError = "Unauthorized";			break;
		case 403:	pszError = "Forbidden";				break;
		case 404:	pszError = "Not Found";				break;
		case 500:	pszError = "Internal Server Error";	break;
		default:	pszError = "Unknown Error";			break;
	}

	CGString sContent;
	sContent.Format(
		"<html>"
		"<head><title>Error %d</title><meta name=robots content=noindex></head>"
		"<body><h2>Error %d: %s</h2><p>The server can't deliver the requested file or script</p></body>"
		"</html>",
		iStatusCode,
		iStatusCode,
		pszError);

	CGTime timeCurrent = CGTime::GetCurrentTime();
	CGString sHeader;
	sHeader.Format(
		"HTTP/1.1 %d %s\r\n"
		"Date: %s\r\n"
		"Server: " SPHERE_TITLE_VER "\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %" FMTSIZE_T "\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		iStatusCode,
		pszError,
		timeCurrent.FormatGmt(NULL),
		sContent.GetLength(),
		static_cast<LPCTSTR>(sContent)
	);

	new PacketWeb(pClient, reinterpret_cast<const BYTE *>(sHeader.GetPtr()), sHeader.GetLength());
	return false;
}
