#include "graysvr.h"	// predef header
#include "../network/send.h"
#include "../network/receive.h"

///////////////////////////////////////////////////////////
// CDialogDef

CDialogDef::CDialogDef(RESOURCE_ID rid) : CResourceLink(rid)
{
	m_iControlCount = 0;
	m_iTextCount = 0;
	m_pObj = NULL;
	m_x = 0;
	m_y = 0;
	m_iOriginX = 0;
	m_iOriginY = 0;
	m_wPage = 0;
	m_fNoDispose = false;
}

bool CDialogDef::GumpSetup(int iPage, CClient *pClientSrc, CObjBase *pObj, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CDialogDef::GumpSetup");
	m_iControlCount = 0;
	m_iTextCount = 0;
	m_pObj = pObj;
	m_iOriginX = 0;
	m_iOriginY = 0;
	m_wPage = static_cast<WORD>(iPage);
	m_fNoDispose = false;

	CScriptTriggerArgs Args(iPage, 0, pObj);
	Args.m_s1_raw = Args.m_s1 = pszArgs;

	// Read text first
	CResourceLock s;
	if ( g_Cfg.ResourceLock(s, RESOURCE_ID(RES_DIALOG, GetResourceID().GetResIndex(), RES_DIALOG_TEXT)) )
	{
		while ( s.ReadKey() )
		{
			if ( m_iTextCount >= COUNTOF(m_sText) - 1 )
				break;

			m_pObj->ParseText(s.GetKeyBuffer(), pClientSrc->GetChar());
			m_sText[m_iTextCount] = s.GetKey();
			++m_iTextCount;
		}
	}

	// Read the main dialog
	if ( !ResourceLock(s) )
		return false;

	// Read the size
	if ( !s.ReadKey() )
		return false;

	// Get X,Y starting position
	INT64 iPos[2];
	TCHAR *pszBuf = s.GetKeyBuffer();
	m_pObj->ParseText(pszBuf, pClientSrc->GetChar());

	Str_ParseCmds(pszBuf, iPos, COUNTOF(iPos));
	m_x = static_cast<int>(iPos[0]);
	m_y = static_cast<int>(iPos[1]);

	if ( OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pClientSrc->GetChar(), &Args) == TRIGRET_RET_TRUE )
		return false;

	return true;
}

size_t CDialogDef::GumpAddText(LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CDialogDef::GumpAddText");
	// Add pszArgs to text section and return insertion index

	m_sText[m_iTextCount] = pszArgs;
	++m_iTextCount;
	return m_iTextCount - 1;
}

enum GUMPCTL_TYPE
{
	GUMPCTL_BUTTON,				// Add button
	GUMPCTL_BUTTONTILEART,		// Add button with tilepic on top
	GUMPCTL_CHECKBOX,			// Add checkbox button
	GUMPCTL_CHECKERTRANS,		// Add transparency layer (required client >= 4.0.0)
	GUMPCTL_CROPPEDTEXT,		// Add text which will be cropped if it exceed given area (index-based)(OBSOLETE)
	GUMPCTL_DCROPPEDTEXT,		// Add text which will be cropped if it exceed given area
	GUMPCTL_DHTMLGUMP,			// Add text using HTML format
	GUMPCTL_DORIGIN,			// Set X,Y origin point to be able to use relative X,Y values on dialog objects
	GUMPCTL_DTEXT,				// Add text
	GUMPCTL_DTEXTENTRY,			// Add text entry area
	GUMPCTL_DTEXTENTRYLIMITED,	// Add text entry area limited to N chars (required client >= 5.0.0)
	GUMPCTL_GROUP,				// Group a bunch of radios/checkboxes
	GUMPCTL_GUMPPIC,			// Add gump picture
	GUMPCTL_GUMPPICTILED,		// Add repeated gump pictures to fill the given area
	GUMPCTL_HTMLGUMP,			// Add text using HTML format (index-based)(OBSOLETE)
	GUMPCTL_NOCLOSE,			// Dialog can't be closed with right click
	GUMPCTL_NODISPOSE,			// Dialog can't be closed with closegump macro
	GUMPCTL_NOMOVE,				// Dialog can't be moved
	GUMPCTL_PAGE,				// Set dialog page id to switch between pages
	GUMPCTL_PICINPIC,			// Add sprite picture (required client >= 7.0.80.0)
	GUMPCTL_RADIO,				// Add radio button
	GUMPCTL_RESIZEPIC,			// Add resized background gump
	GUMPCTL_TEXT,				// Add text (index-based)(OBSOLETE)
	GUMPCTL_TEXTENTRY,			// Add text entry area (index-based)(OBSOLETE)
	GUMPCTL_TEXTENTRYLIMITED,	// Add text entry area limited to N chars (required client >= 5.0.0)(index-based)(OBSOLETE)
	GUMPCTL_TILEPIC,			// Add item picture from tiledata.mul
	GUMPCTL_TILEPICHUE,			// Add hued item picture from tiledata.mul
	GUMPCTL_TOOLTIP,			// Add mouse-over tooltip on dialog object (required client >= 4.0.0)
	GUMPCTL_XMFHTMLGUMP,		// Add cliloc text
	GUMPCTL_XMFHTMLGUMPCOLOR,	// Add hued cliloc text
	GUMPCTL_XMFHTMLTOK,			// Add hued cliloc text with arguments
	GUMPCTL_QTY
};

const LPCTSTR CDialogDef::sm_szLoadKeys[GUMPCTL_QTY + 1] =
{
	"button",
	"buttontileart",
	"checkbox",
	"checkertrans",
	"croppedtext",
	"dcroppedtext",
	"dhtmlgump",
	"dorigin",
	"dtext",
	"dtextentry",
	"dtextentrylimited",
	"group",
	"gumppic",
	"gumppictiled",
	"htmlgump",
	"noclose",
	"nodispose",
	"nomove",
	"page",
	"picinpic",
	"radio",
	"resizepic",
	"text",
	"textentry",
	"textentrylimited",
	"tilepic",
	"tilepichue",
	"tooltip",
	"xmfhtmlgump",
	"xmfhtmlgumpcolor",
	"xmfhtmltok",
	NULL
};

bool CDialogDef::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CDialogDef::r_WriteVal");
	if ( m_pObj )
		return m_pObj->r_WriteVal(pszKey, sVal, pSrc);
	return false;
}

bool CDialogDef::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CDialogDef::r_LoadVal");
	if ( m_pObj )
		return m_pObj->r_LoadVal(s);
	return false;
}

#define SKIP_ALL(args)		SKIP_SEPARATORS(args);	GETNONWHITESPACE(args);
#define GET_ABSOLUTE(c)		SKIP_ALL(pszArgs);		int c = Exp_GetSingle(pszArgs);
#define GET_RELATIVE(c, origin)							\
	SKIP_ALL(pszArgs);									\
	int c;												\
	if ((*pszArgs == '-') && IsSpace(pszArgs[1]))		\
		c = origin, ++pszArgs;							\
	else if (*pszArgs == '+')							\
		c = origin + Exp_GetSingle(++pszArgs);			\
	else if (*pszArgs == '-')							\
		c = origin - Exp_GetSingle(++pszArgs);			\
	else if (*pszArgs == '*')							\
		origin = c = origin + Exp_GetSingle(++pszArgs);	\
	else												\
		c = Exp_GetSingle(pszArgs);

bool CDialogDef::r_Verb(CScript &s, CTextConsole *pSrc)		// some command on this object as a target
{
	ADDTOCALLSTACK("CDialogDef::r_Verb");
	EXC_TRY("Verb");

	// The first part of the key is GUMPCTL_TYPE
	LPCTSTR pszKey = s.GetKey();

	int index = FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1);
	if ( index < 0 )
	{
		CScriptTriggerArgs Args(s.GetArgRaw());
		CGString sVal;
		if ( r_Call(s.GetKey(), pSrc, &Args, &sVal) )
			return true;

		if ( !m_pObj )
			return CResourceLink::r_Verb(s, pSrc);

		return m_pObj->r_Verb(s, pSrc);
	}

	LPCTSTR pszArgs = s.GetArgStr();

	switch ( index )
	{
		case GUMPCTL_PAGE:
		{
			if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
				return false;

			GET_ABSOLUTE(page);

			if ( page <= 0 )
				return true;

			int iNewPage;
			if ( (m_wPage == 0) || (page > m_wPage) )
				iNewPage = page;
			else if ( page == m_wPage )
				iNewPage = 1;
			else
				iNewPage = page + 1;

			m_sControl[m_iControlCount].Format("page %d", iNewPage);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_BUTTON:
		case GUMPCTL_BUTTONTILEART:
		{
			if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gumpUnpressed);
			GET_ABSOLUTE(gumpPressed);
			GET_ABSOLUTE(ispressable);
			GET_ABSOLUTE(page);
			GET_ABSOLUTE(id);

			int iNewPage;
			if ( (m_wPage == 0) || (page > m_wPage) || (page == 0) )
				iNewPage = page;
			else if ( page == m_wPage )
				iNewPage = 1;
			else
				iNewPage = page + 1;

			if ( index == GUMPCTL_BUTTON )
				m_sControl[m_iControlCount].Format("button %d %d %d %d %d %d %d", x, y, gumpUnpressed, gumpPressed, ispressable, iNewPage, id);
			else
			{
				GET_ABSOLUTE(tileID);
				GET_ABSOLUTE(tileHue);
				GET_ABSOLUTE(tileX);
				GET_ABSOLUTE(tileY);

				m_sControl[m_iControlCount].Format("buttontileart %d %d %d %d %d %d %d %d %d %d %d", x, y, gumpUnpressed, gumpPressed, ispressable, iNewPage, id, tileID, tileHue, tileX, tileY);
			}

			++m_iControlCount;
			return true;
		}
		case GUMPCTL_GUMPPIC:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gump);
			SKIP_ALL(pszArgs);

			m_sControl[m_iControlCount].Format("gumppic %d %d %d%s%s", x, y, gump, *pszArgs ? " hue=" : "", *pszArgs ? pszArgs : "");
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_GUMPPICTILED:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(gump);

			m_sControl[m_iControlCount].Format("gumppictiled %d %d %d %d %d", x, y, width, height, gump);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_PICINPIC:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gump);
			GET_ABSOLUTE(spriteX);
			GET_ABSOLUTE(spriteY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);

			m_sControl[m_iControlCount].Format("picinpic %d %d %d %d %d %d %d", x, y, gump, spriteX, spriteY, width, height);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_RESIZEPIC:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gump);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);

			m_sControl[m_iControlCount].Format("resizepic %d %d %d %d %d", x, y, gump, width, height);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_TILEPIC:
		case GUMPCTL_TILEPICHUE:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(itemid);
			SKIP_ALL(pszArgs);

			if ( index == GUMPCTL_TILEPIC )
				m_sControl[m_iControlCount].Format("tilepic %d %d %d", x, y, itemid);
			else
				m_sControl[m_iControlCount].Format("tilepichue %d %d %d%s%s", x, y, itemid, *pszArgs ? " " : "", *pszArgs ? pszArgs : "");

			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DTEXT:
		{
			if ( (m_iControlCount >= COUNTOF(m_sControl) - 1) || (m_iTextCount >= COUNTOF(m_sText) - 1) )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(hue);
			SKIP_ALL(pszArgs);

			if ( *pszArgs == '.' )
				++pszArgs;

			size_t iText = GumpAddText(*pszArgs ? pszArgs : "");
			m_sControl[m_iControlCount].Format("text %d %d %d %" FMTSIZE_T, x, y, hue, iText);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DCROPPEDTEXT:
		{
			if ( (m_iControlCount >= COUNTOF(m_sControl) - 1) || (m_iTextCount >= COUNTOF(m_sText) - 1) )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(hue);
			SKIP_ALL(pszArgs);

			if ( *pszArgs == '.' )
				++pszArgs;

			size_t iText = GumpAddText(*pszArgs ? pszArgs : "");
			m_sControl[m_iControlCount].Format("croppedtext %d %d %d %d %d %" FMTSIZE_T, x, y, width, height, hue, iText);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DHTMLGUMP:
		{
			if ( (m_iControlCount >= COUNTOF(m_sControl) - 1) || (m_iTextCount >= COUNTOF(m_sText) - 1) )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(hasbackground);
			GET_ABSOLUTE(hasscrollbar);
			SKIP_ALL(pszArgs);

			size_t iText = GumpAddText(*pszArgs ? pszArgs : "");
			m_sControl[m_iControlCount].Format("htmlgump %d %d %d %d %" FMTSIZE_T " %d %d", x, y, width, height, iText, hasbackground, hasscrollbar);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DTEXTENTRY:
		{
			if ( (m_iControlCount >= COUNTOF(m_sControl) - 1) || (m_iTextCount >= COUNTOF(m_sText) - 1) )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(hue);
			GET_ABSOLUTE(id);
			SKIP_ALL(pszArgs);

			size_t iText = GumpAddText(*pszArgs ? pszArgs : "");
			m_sControl[m_iControlCount].Format("textentry %d %d %d %d %d %d %" FMTSIZE_T, x, y, width, height, hue, id, iText);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DTEXTENTRYLIMITED:
		{
			if ( (m_iControlCount >= COUNTOF(m_sControl) - 1) || (m_iTextCount >= COUNTOF(m_sText) - 1) )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(hue);
			GET_ABSOLUTE(id);
			GET_ABSOLUTE(limit);
			SKIP_ALL(pszArgs);

			size_t iText = GumpAddText(*pszArgs ? pszArgs : "");
			m_sControl[m_iControlCount].Format("textentrylimited %d %d %d %d %d %d %" FMTSIZE_T " %d", x, y, width, height, hue, id, iText, limit);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_CHECKBOX:
		{
			if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gumpUnchecked);
			GET_ABSOLUTE(gumpChecked);
			GET_ABSOLUTE(ischecked);
			GET_ABSOLUTE(id);

			m_sControl[m_iControlCount].Format("checkbox %d %d %d %d %d %d", x, y, gumpUnchecked, gumpChecked, ischecked, id);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_RADIO:
		{
			if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(gumpUnchecked);
			GET_ABSOLUTE(gumpChecked);
			GET_ABSOLUTE(ischecked);
			GET_ABSOLUTE(id);

			m_sControl[m_iControlCount].Format("radio %d %d %d %d %d %d", x, y, gumpUnchecked, gumpChecked, ischecked, id);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_CHECKERTRANS:
		{
			if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
				return false;

			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);

			m_sControl[m_iControlCount].Format("checkertrans %d %d %d %d", x, y, width, height);
			++m_iControlCount;
			return true;
		}
		case GUMPCTL_DORIGIN:
		{
			//GET_RELATIVE(x, m_iOriginX);
			//GET_RELATIVE(y, m_iOriginY);
			//m_iOriginX = x;
			//m_iOriginY = y;

			SKIP_ALL(pszArgs);
			if ( (*pszArgs == '-') && (IsSpace(pszArgs[1]) || !pszArgs[1]) )
				++pszArgs;
			else if ( *pszArgs == '*' )
				m_iOriginX += Exp_GetSingle(++pszArgs);
			else
				m_iOriginX = Exp_GetSingle(pszArgs);

			SKIP_ALL(pszArgs);
			if ( (*pszArgs == '-') && (IsSpace(pszArgs[1]) || !pszArgs[1]) )
				++pszArgs;
			else if ( *pszArgs == '*' )
				m_iOriginY += Exp_GetSingle(++pszArgs);
			else
				m_iOriginY = Exp_GetSingle(pszArgs);

			return true;
		}
		case GUMPCTL_NODISPOSE:
			m_fNoDispose = true;
			break;
		case GUMPCTL_CROPPEDTEXT:
		case GUMPCTL_TEXT:
		case GUMPCTL_TEXTENTRY:
		case GUMPCTL_TEXTENTRYLIMITED:
			break;
		case GUMPCTL_XMFHTMLGUMP:
		case GUMPCTL_XMFHTMLGUMPCOLOR:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(cliloc);
			GET_ABSOLUTE(hasbackground);
			GET_ABSOLUTE(hasscrollbar);
			//SKIP_ALL(pszArgs);

			if ( index == GUMPCTL_XMFHTMLGUMP )
				m_sControl[m_iControlCount].Format("xmfhtmlgump %d %d %d %d %d %d %d", x, y, width, height, cliloc, hasbackground, hasscrollbar);
			else
				m_sControl[m_iControlCount].Format("xmfhtmlgumpcolor %d %d %d %d %d %d %d%s%s", x, y, width, height, cliloc, hasbackground, hasscrollbar, *pszArgs ? " " : "", *pszArgs ? pszArgs : "");

			++m_iControlCount;
			return true;
		}
		case GUMPCTL_XMFHTMLTOK:
		{
			GET_RELATIVE(x, m_iOriginX);
			GET_RELATIVE(y, m_iOriginY);
			GET_ABSOLUTE(width);
			GET_ABSOLUTE(height);
			GET_ABSOLUTE(hasbackground);
			GET_ABSOLUTE(hasscrollbar);
			GET_ABSOLUTE(hue);
			GET_ABSOLUTE(cliloc);
			SKIP_ALL(pszArgs);

			m_sControl[m_iControlCount].Format("xmfhtmltok %d %d %d %d %d %d %d %d %s", x, y, width, height, hasbackground, hasscrollbar, hue, cliloc, *pszArgs ? pszArgs : "");
			++m_iControlCount;
			return true;
		}
		default:
			break;
	}

	if ( m_iControlCount >= COUNTOF(m_sControl) - 1 )
		return false;

	m_sControl[m_iControlCount].Format("%s %s", pszKey, pszArgs);
	++m_iControlCount;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

#undef SKIP_ALL
#undef GET_ABSOLUTE
#undef GET_RELATIVE

///////////////////////////////////////////////////////////
// CClient

void CClient::Dialog_Setup(CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase *pObj, LPCTSTR pszArgs)
{
	ADDTOCALLSTACK("CClient::Dialog_Setup");
	if ( !pObj )
		return;

	CResourceDef *pRes = g_Cfg.ResourceGetDef(rid);
	CDialogDef *pDlg = dynamic_cast<CDialogDef *>(pRes);
	if ( !pDlg )
	{
		DEBUG_ERR(("Invalid RES_DIALOG\n"));
		return;
	}

	if ( !pDlg->GumpSetup(iPage, this, pObj, pszArgs) )
		return;

	// Now pack it up to send
	//m_tmGumpDialog.m_ResourceID = rid;

	DWORD dwContext = static_cast<DWORD>(rid);
	if ( m_NetState->isClientKR() )
	{
		// KR enhanced clients can use only client internal dialogs, so check if the dialog can be translated into KR equivalent DialogID.
		// SA+ enhanced clients doesn't need this translation anymore because they support server dynamic dialogs again
		dwContext = g_Cfg.GetKRDialog(dwContext);
		if ( dwContext == 0 )
			g_Log.Event(LOGL_WARN, "A Kingdom Reborn equivalent of dialog '%s' has not been defined\n", pDlg->GetResourceName());
	}

	addGumpDialog(mode, pDlg->m_sControl, pDlg->m_iControlCount, pDlg->m_sText, pDlg->m_iTextCount, pDlg->m_x, pDlg->m_y, pObj, dwContext);
}

bool CClient::Dialog_Close(CObjBase *pObj, DWORD rid, DWORD dwButtonID)
{
	ADDTOCALLSTACK("CClient::Dialog_Close");
	// Dialog was closed

	const DWORD dwContext = (rid & 0xFFFFFF);
	new PacketGumpChange(this, dwContext, dwButtonID);

	if ( m_NetState->isClientVersion(MINCLIVER_CLOSEDIALOG) && pObj->IsChar() )
	{
		OpenedGumpsMap_t::iterator itGumpFound = m_mapOpenedGumps.find(dwContext);
		if ( (itGumpFound != m_mapOpenedGumps.end()) && ((*itGumpFound).second > 0) )
		{
			PacketGumpDialogRet packet;
			packet.writeByte(PACKET_GumpDialogRet);
			packet.writeInt16(27);
			packet.writeInt32(pObj->GetUID());
			packet.writeInt32(dwContext);
			packet.writeInt32(dwButtonID);
			packet.writeInt32(0);
			packet.writeInt32(0);
			packet.writeInt32(0);

			packet.seek(1);
			packet.onReceive(m_NetState);
		}
	}
	return true;
}

TRIGRET_TYPE CClient::Dialog_OnButton(RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase *pObj, CDialogResponseArgs *pArgs)
{
	ADDTOCALLSTACK("CClient::Dialog_OnButton");
	// Dialog button was pressed
	if ( !pObj )
		return TRIGRET_ENDIF;

	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, RESOURCE_ID(RES_DIALOG, rid.GetResIndex(), RES_DIALOG_BUTTON)) )
		return TRIGRET_ENDIF;

	INT64 piCmd[3];
	while ( s.ReadKeyParse() )
	{
		if ( !s.IsKeyHead("ON", 2) )
			continue;

		size_t iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, COUNTOF(piCmd));
		if ( iArgQty == 0 )
			continue;
		else if ( iArgQty == 1 )
		{
			// Single button value
			if ( dwButtonID != static_cast<DWORD>(piCmd[0]) )
				continue;
		}
		else
		{
			// Range of button values
			if ( (dwButtonID < static_cast<DWORD>(piCmd[0])) || (dwButtonID > static_cast<DWORD>(piCmd[1])) )
				continue;
		}

		pArgs->m_iN1 = dwButtonID;
		return pObj->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar, pArgs);
	}
	return TRIGRET_ENDIF;
}

void CClient::Menu_Setup(RESOURCE_ID_BASE rid, CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::Menu_Setup");
	// Open menu for general purpose
	// Client should return PacketMenuChoice::onReceive()

	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, rid) )
		return;

	if ( !pObj )
		pObj = m_pChar;

	static_cast<void>(s.ReadKey());		// get menu title
	pObj->ParseText(s.GetKeyBuffer(), m_pChar);

	CMenuItem item[MAX_MENU_ITEMS];
	item[0].m_sText = s.GetKey();
	//item[0].m_id = rid.m_internalrid;		// general context id

	size_t i = 0;
	while ( s.ReadKeyParse() )
	{
		if ( !s.IsKey("ON") )
			continue;

		++i;
		if ( !item[i].ParseLine(s.GetArgRaw(), pObj, m_pChar) )
			--i;

		if ( i >= COUNTOF(item) - 1 )
			break;
	}

	m_tmMenu.m_ResourceID = rid;

	ASSERT(i < COUNTOF(item));
	addItemMenu(CLIMODE_MENU, item, i, pObj);
}

TRIGRET_TYPE CClient::Menu_OnSelect(RESOURCE_ID_BASE rid, WORD wIndex, CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::Menu_OnSelect");
	// A selection was made on menu

	CResourceLock s;
	if ( !g_Cfg.ResourceLock(s, rid) )
		return TRIGRET_ENDIF;

	if ( !pObj )
		pObj = m_pChar;

	// Execute the menu script
	if ( wIndex == 0 )	// cancel button
	{
		while ( s.ReadKeyParse() )
		{
			if ( !s.IsKey("ON") || (*s.GetArgStr() != '@') || strcmpi(s.GetArgStr(), "@cancel") )
				continue;

			return pObj->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar);
		}
	}
	else
	{
		WORD i = 0;		// 1-based index
		while ( s.ReadKeyParse() )
		{
			if ( !s.IsKey("ON") || (*s.GetArgStr() == '@') )
				continue;

			++i;
			if ( i < wIndex )
				continue;
			if ( i > wIndex )
				break;

			return pObj->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar);
		}
	}

	// No selection
	return TRIGRET_ENDIF;
}

void CClient::addGumpInpVal(bool fCancel, INPVAL_TYPE type, DWORD dwMaxLength, LPCTSTR pszText, LPCTSTR pszCaption, CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addGumpInpVal");
	// CLIMODE_INPVAL
	// Just input an objects attribute. Should result in PacketGumpValueInputResponse::onReceive()
	// ARGS:
	//  m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	if ( !pObj )
		return;

	new PacketGumpValueInput(this, fCancel, type, dwMaxLength, pszText, pszCaption, pObj);
	m_Targ_UID = pObj->GetUID();
	SetTargMode(CLIMODE_INPVAL);
}

void CClient::addGumpDialog(CLIMODE_TYPE mode, const CGString *psControls, size_t iControlCount, const CGString *psText, size_t iTextCount, DWORD x, DWORD y, CObjBase *pObj, DWORD rid)
{
	ADDTOCALLSTACK("CClient::addGumpDialog");
	// Open generic dialog
	// Client should return PacketGumpDialogRet::onReceive()
	// NOTE: These packets can get rather LARGE

	if ( !pObj )
		pObj = m_pChar;

	DWORD dwContext = mode;
	if ( (mode == CLIMODE_DIALOG) && (rid != 0) )
		dwContext = (rid & 0xFFFFFF);

	PacketGumpDialog *cmd = new PacketGumpDialog(x, y, pObj, dwContext);
	cmd->writeControls(this, psControls, iControlCount, psText, iTextCount);
	cmd->push(this);

	if ( m_pChar )
		++m_mapOpenedGumps[dwContext];
}

void CClient::addGumpDialogProps(CObjBase *pObj)
{
	ADDTOCALLSTACK("CClient::addGumpDialogProps");
	// Open object properties dialog
	if ( !pObj )
		return;

	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pObj->IsItem() ? "d_itemprop1" : "d_charprop1");
	if ( !rid.IsValidUID() )
		return;

	m_Prop_UID = m_Targ_UID = pObj->GetUID();
	Dialog_Setup(CLIMODE_DIALOG, rid, 0, pObj);
}

///////////////////////////////////////////////////////////
// CMenuItem

bool CMenuItem::ParseLine(TCHAR *pszArgs, CScriptObj *pObjBase, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CMenuItem::ParseLine");

	if ( *pszArgs == '@' )
		return false;

	TCHAR *pszArgStart = pszArgs;
	while ( _ISCSYM(*pszArgs) )
		++pszArgs;

	if ( *pszArgs )
	{
		*pszArgs = '\0';
		++pszArgs;
		GETNONWHITESPACE(pszArgs);
	}

	// The item id (if we want to have an item type menu) or 0
	if ( strcmp(pszArgStart, "0") != 0 )
	{
		CItemBase *pItemBase = CItemBase::FindItemBase(static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszArgStart)));
		if ( pItemBase )
		{
			m_id = static_cast<WORD>(pItemBase->GetDispID());
			pObjBase = pItemBase;
		}
		else
		{
			DEBUG_ERR(("Bad MENU item id '%s'\n", pszArgStart));
			return false;
		}
	}
	else
		m_id = 0;

	if ( pObjBase )
		pObjBase->ParseText(pszArgs, pSrc);
	else
		g_Serv.ParseText(pszArgs, pSrc);

	// Parsing @color
	if ( *pszArgs == '@' )
	{
		++pszArgs;
		HUE_TYPE wHue = static_cast<HUE_TYPE>(Exp_GetVal(pszArgs));
		if ( wHue != 0 )
			wHue = ((wHue == 1) ? 0x7FF : wHue - 1);

		m_color = wHue;
		SKIP_ARGSEP(pszArgs);
	}
	else
		m_color = 0;

	m_sText = pszArgs;

	if ( m_sText.IsEmpty() )
	{
		if ( pObjBase )		// use the objects name by default
		{
			m_sText = pObjBase->GetName();
			if ( !m_sText.IsEmpty() )
				return true;
		}
		DEBUG_ERR(("Bad MENU item text '%s'\n", pszArgStart));
	}

	return !m_sText.IsEmpty();
}
