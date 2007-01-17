#include "graysvr.h"
#include "CClient.h"
#include "common/zlib/zlib.h"
#include "network/send.h"

enum GUMPCTL_TYPE	// controls we can put in a gump.
{
	GUMPCTL_BUTTON	= 0,		// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
	GUMPCTL_BUTTONTILEART,		// NEW: 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
	GUMPCTL_CHECKBOX,		// 6 = x,y,gumpcheck,gumpuncheck,starting state,checkid

	GUMPCTL_CHECKERTRANS,		// NEW: x,y,w,h
	GUMPCTL_CROPPEDTEXT,		// 6 = x,y,sx,sy,color?,startindex

	GUMPCTL_DCROPPEDTEXT,
	GUMPCTL_DHTMLGUMP,
	GUMPCTL_DORIGIN,
	GUMPCTL_DTEXT,
	GUMPCTL_DTEXTENTRY,
	GUMPCTL_DTEXTENTRYLIMITED,
	GUMPCTL_DYNAMIC,

	GUMPCTL_GROUP,

	GUMPCTL_GUMPPIC,		// 3 = x,y,gumpID	 hue=color// put gumps in the dlg.
	GUMPCTL_GUMPPICTILED,		// x, y, gumpID, w, h, hue=color
	GUMPCTL_HTMLGUMP,		// 7 = x,y,sx,sy, 0 0 0

	// Not really controls but more attributes.
	GUMPCTL_NOCLOSE,		// 0 = not really used
	GUMPCTL_NODISPOSE,		// 0 = not really used  (modal?)
	GUMPCTL_NOMOVE,			// 0 = The gump cannot be moved around.

	GUMPCTL_PAGE,			// 1 = set current page number	// for multi tab dialogs.

	GUMPCTL_RADIO,			// 6 = x,y,gump1,gump2,starting state,id
	GUMPCTL_RESIZEPIC,		// 5 = x,y,gumpback,sx,sy	// can come first if multi page. put up some background gump
	GUMPCTL_TEXT,			// 4 = x,y,color?,startstringindex	// put some text here.
	GUMPCTL_TEXTENTRY,
	GUMPCTL_TEXTENTRYLIMITED,
	GUMPCTL_TILEPIC,		// 3 = x,y,item	// put item tiles in the dlg.
	GUMPCTL_TILEPICHUE,		// NEW: x,y,item,color

	GUMPCTL_TOOLTIP,		// From SE client. tooltip cliloc(1003000)

	GUMPCTL_XMFHTMLGUMP,		// 7 = x,y,sx,sy, cliloc(1003000) hasBack canScroll
	GUMPCTL_XMFHTMLGUMPCOLOR,	// NEW: x,y,w,h ???
	GUMPCTL_XMFHTMLTOK,			// new... syntax???

	GUMPCTL_QTY,
};


//*******************************************
// -CDialogDef
LPCTSTR const CDialogDef::sm_szLoadKeys[GUMPCTL_QTY+1] =
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
	"dynamic",

	"group",

	"gumppic",
	"gumppictiled",
	"htmlgump",

	"noclose",
	"nodispose",
	"nomove",

	"page",

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

	NULL,
};


int	CDialogDef::GumpAddText( LPCTSTR pszText )
{
	Element storage;
	storage.m_index = m_iTexts;
	storage.m_contents = pszText;
	m_text.push_back(storage);
	m_iTexts++;
	return (m_iTexts-1);
}

#define SKIP_ALL( args )		SKIP_SEPARATORS( args ); GETNONWHITESPACE( args );
#define GET_ABSOLUTE( c )		SKIP_ALL( pszArgs );	int c = Exp_GetSingle( pszArgs );

#define GET_EVAL( c )		SKIP_ALL( pszArgs );	int c = Exp_GetVal( pszArgs );

#define GET_RELATIVE( c, base )								\
	SKIP_ALL( pszArgs ); int c;								\
	if ( *pszArgs == '-' && isspace(pszArgs[1]))				\
		c	= base, ++pszArgs;								\
	else if ( *pszArgs == '+' )								\
		c = base + Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '-' )								\
		c = base - Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '*' )								\
		base = c	= base + Exp_GetSingle( ++pszArgs );		\
	else													\
		c = Exp_GetSingle( pszArgs );			

#define ADD2GUMP(_a_) \
	Element storage; \
	storage.m_index = m_iControls; \
	storage.m_contents.Format _a_ ; \
	m_controls.push_back(storage); \
	m_iControls++;

bool CDialogDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
	EXC_TRY("Verb");
	// The first part of the key is GUMPCTL_TYPE
	LPCSTR		pszKey = s.GetKey();

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index < 0 )
	{
		CScriptTriggerArgs Args(s.GetArgRaw());
		if ( r_Call(s.GetKey(), pSrc, &Args) )
			return true;
		if (!m_pObj)
			return CResourceLink::r_Verb(s, pSrc);
		return m_pObj->r_Verb(s, pSrc);
	}
	
	LPCSTR	pszArgs	= s.GetArgStr();

	switch ( index )
	{
		case GUMPCTL_DYNAMIC:
			m_dynamic = (s.GetArgVal() != 0);
			break;

		case GUMPCTL_PAGE:
		{
			GET_ABSOLUTE( page );

			if ( page <= 0 )		return true;

			int	iNewPage;
			if ( m_iPage == 0 || page > m_iPage || page == 0 )
				iNewPage	= page;
			else if ( page == m_iPage  )
				iNewPage	= 1;
			else
				iNewPage	= page + 1;	

			ADD2GUMP(("page %d", iNewPage))
			return true;
		}
		case GUMPCTL_BUTTON:			// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
		case GUMPCTL_BUTTONTILEART:		// 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( down );
			GET_ABSOLUTE( up );
			GET_ABSOLUTE( press );
			GET_ABSOLUTE( page );
			GET_ABSOLUTE( id );

			int	iNewPage;
			if ( m_iPage == 0 || page > m_iPage || page == 0 )
				iNewPage	= page;
			else if ( page == m_iPage  )
				iNewPage	= 1;
			else
				iNewPage	= page + 1;	

			if (index == GUMPCTL_BUTTON)
			{
				ADD2GUMP(("button %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id));
			}
			else
			{
				GET_ABSOLUTE( tileId );
				GET_ABSOLUTE( tileHue );
				GET_ABSOLUTE( tileX );
				GET_ABSOLUTE( tileY );				
				
				ADD2GUMP(("buttontileart %d %d %d %d %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id, tileId, tileHue, tileX, tileY))
			}
			return true;
		}
		case GUMPCTL_GUMPPIC:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs );

			ADD2GUMP(("gumppic %d %d %d%s%s", x, y, id, *pszArgs ? " hue=" : "", *pszArgs ? pszArgs : ""))
			return true;
		}
		case GUMPCTL_RESIZEPIC:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			GET_ABSOLUTE( sX );
			GET_ABSOLUTE( sY );

			ADD2GUMP(( "resizepic %d %d %d %d %d", x, y, id, sX, sY ));
			return true;
		}
		case GUMPCTL_TILEPIC:
		case GUMPCTL_TILEPICHUE:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs );

			// TilePic don't use args, TilePicHue yes :)
			if ( index == GUMPCTL_TILEPIC )
			{
				ADD2GUMP(("tilepic %d %d %d", x, y, id));
			}
			else
			{
				ADD2GUMP(("tilepichue %d %d %d%s%s", x, y, id, *pszArgs ? " " : "", *pszArgs ? pszArgs : ""));
			}
			return true;
		}
		case GUMPCTL_DTEXT:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText = GumpAddText( *pszArgs ? pszArgs : "" );
			ADD2GUMP(("text %d %d %d %d", x, y, hue, iText));
			return true;
		}
		case GUMPCTL_DCROPPEDTEXT:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			ADD2GUMP(("croppedtext %d %d %d %d %d %d", x, y, w, h, hue, iText));
			return true;
		}
		case GUMPCTL_DHTMLGUMP:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( bck );
			GET_ABSOLUTE( options );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			ADD2GUMP(("htmlgump %d %d %d %d %d %d %d", x, y, w, h, iText, bck, options));
			return true;
		}
		case GUMPCTL_DTEXTENTRY:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			ADD2GUMP(("textentry %d %d %d %d %d %d %d", x, y, w, h, hue, id, iText));
			return true;
		}
		case GUMPCTL_DTEXTENTRYLIMITED:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			GET_ABSOLUTE( id );
			GET_ABSOLUTE( charLimit );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			ADD2GUMP(("textentrylimited %d %d %d %d %d %d %d %d", x, y, w, h, hue, id, iText, charLimit));
			return true;
		}
		case GUMPCTL_DORIGIN:
		{
			// GET_RELATIVE( x, m_iOriginX );
			// GET_RELATIVE( y, m_iOriginY );
			// m_iOriginX	= x;
			// m_iOriginY	= y;
			
			SKIP_ALL( pszArgs );
			if ( *pszArgs == '-' && (isspace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
			else  if ( *pszArgs == '*' )	m_iOriginX	+= Exp_GetSingle( ++pszArgs );
			else							m_iOriginX	 = Exp_GetSingle( pszArgs );

			SKIP_ALL( pszArgs );
			if ( *pszArgs == '-' && (isspace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
			else  if ( *pszArgs == '*' )	m_iOriginY	+= Exp_GetSingle( ++pszArgs );
			else							m_iOriginY	= Exp_GetSingle( pszArgs );
			
			break;
		}
		default:
		{
			// all the rest goes directly to the client
			ADD2GUMP(("%s %s", pszKey, pszArgs));
		}
	}

	// all the rest goes directly to the client
	ADD2GUMP(("%s %s", pszKey, pszArgs));

	return true;
	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}


CDialogDef::CDialogDef(RESOURCE_ID rid)
	: CResourceLink(rid)
{
	m_dynamic = true;
	m_iControls = 0;
	m_iTexts = 0;
	m_pObj = NULL;
}


bool CDialogDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	if ( !m_pObj )
		return false;
	return m_pObj->r_WriteVal( pszKey, sVal, pSrc );
}


bool CDialogDef::r_LoadVal( CScript & s )
{
	if ( !m_pObj )
		return false;
	return m_pObj->r_LoadVal( s );
}


bool CDialogDef::GumpSetup( int iPage, CClient * pClient, CObjBase * pObjSrc, LPCTSTR Arguments )
{
	//	Static gumps should not be reparsed
	if ( !m_dynamic ) return true;

	CResourceLock	s;

	m_iControls		= 0;
	m_iTexts		= 0;
	m_pObj			= pObjSrc;
	m_iOriginX		= 0;
	m_iOriginY		= 0;
	m_iPage			= iPage;
	m_controls.clear();
	m_text.clear();

	CScriptTriggerArgs	Args(iPage, 0, pObjSrc);
	Args.m_s1_raw = Args.m_s1 = Arguments;

	if ( g_Cfg.ResourceLock(s, RESOURCE_ID( RES_DIALOG, GetResourceID().GetResIndex(), RES_DIALOG_TEXT)) )
	{
		while ( s.ReadKey() )
		{
			m_pObj->ParseText(s.GetKeyBuffer(), pClient->GetChar());
			GumpAddText(s.GetKey());
		}
	}
	
	// read the main dialog
	if ( !ResourceLock(s) )
		return false;

	if ( !s.ReadKey() )		// read the size.
		return false;

	// starting x,y location.
	int iSizes[2];

	TCHAR * pszBuf = s.GetKeyBuffer();
	m_pObj->ParseText( pszBuf, pClient->GetChar() );

	Str_ParseCmds( pszBuf, iSizes, COUNTOF(iSizes) );
	m_x		= iSizes[0];
	m_y		= iSizes[1];

	if ( OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pClient->GetChar(), &Args ) == TRIGRET_RET_TRUE )
		return false;

	return true;
}



bool CClient::Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, LPCTSTR Arguments )
{
	if ( pObj == NULL ) return false;

	CResourceDef *pRes	= g_Cfg.ResourceGetDef(rid);
	CDialogDef *pDlg	= dynamic_cast <CDialogDef*>(pRes);
	if ( !pRes )
	{
		g_Log.Error("Invalid RES_DIALOG.\n");
	}
	else if ( pDlg->GumpSetup(iPage, this, pObj, Arguments) )
	{
		addGumpDialog(mode, &pDlg->m_controls, pDlg->m_iControls, &pDlg->m_text, pDlg->m_iTexts, pDlg->m_x, pDlg->m_y, pObj, rid);
		return true;
	}
	return false;
}





void CClient::addGumpInpVal( bool fCancel, INPVAL_STYLE style,
	DWORD iMaxLength,
	LPCTSTR pszText1,
	LPCTSTR pszText2,
	CObjBase * pObj )
{
	// CLIMODE_INPVAL
	// Should result in Event_GumpInpValRet
	// just input an objects attribute.

	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	CCommand cmd;

	cmd.GumpInpVal.m_Cmd = XCMD_GumpInpVal;
	cmd.GumpInpVal.m_len = sizeof(cmd.GumpInpVal);

	cmd.GumpInpVal.m_UID = pObj->GetUID();
	cmd.GumpInpVal.m_context = CLIMODE_INPVAL; // LOWORD( m_tmGumpDialog.m_ResourceID); // CLIMODE_INPVAL;

	cmd.GumpInpVal.m_textlen1 = sizeof(cmd.GumpInpVal.m_text1);
	strcpy( cmd.GumpInpVal.m_text1, pszText1 );

	cmd.GumpInpVal.m_cancel = fCancel ? 1 : 0;
	cmd.GumpInpVal.m_style = (BYTE) style;
	cmd.GumpInpVal.m_mask = iMaxLength;	// !!!
	cmd.GumpInpVal.m_textlen2 = sizeof(cmd.GumpInpVal.m_text2);
	cmd.GumpInpVal.m_text2[0] = '\0';	// clear just in case.

	switch ( style )
	{
	case INPVAL_STYLE_NOEDIT: // None
		break;
	case INPVAL_STYLE_TEXTEDIT: // Text
		sprintf(cmd.GumpInpVal.m_text2,
			"%s (%d chars max)", pszText2, iMaxLength );
		break;
	case INPVAL_STYLE_NUMEDIT: // Numeric
		sprintf(cmd.GumpInpVal.m_text2,
			"%s (0 - %d)", pszText2, iMaxLength );
		break;
	}

	xSend(&cmd, sizeof(cmd.GumpInpVal));

	// m_tmInpVal.m_UID = pObj->GetUID();
	// m_tmInpVal.m_PrvGumpID = m_tmGumpDialog.m_ResourceID;

	m_Targ_UID = pObj->GetUID();
	// m_Targ_Text = verb
	SetTargMode( CLIMODE_INPVAL );
}


void CClient::addGumpDialog( CLIMODE_TYPE mode, vector<CDialogDef::Element> *psControls, int iControls, vector<CDialogDef::Element> *psText, int iTexts, int x, int y, CObjBase * pObj, DWORD rid)
{
	// Add a generic GUMP menu.
	// Should return a Event_GumpDialogRet
	// NOTE: These packets can get rather LARGE.
	// x,y = where on the screen ?

	if ( pObj == NULL )
		pObj = m_pChar;
	int lengthControls=1;
	vector<CDialogDef::Element>::const_iterator it;

	for ( it = psControls->begin(); it != psControls->end(); it++ )
	{
		lengthControls += it->m_contents.GetLength() + 2;
	}

	int lengthText = lengthControls + 20 + 3;
	for ( it = psText->begin(); it != psText->end(); it++ )
	{
		lengthText += (it->m_contents.GetLength()*2)+2;
	}

	int	context_mode	= mode;
	if ( mode == CLIMODE_DIALOG && rid != 0 )
	{
		context_mode	= GETINTRESOURCE( rid );
	}

	CCommand cmd;
	memset( &cmd, 0, sizeof( cmd ) );
	int len = 0;

	cmd.CompressedGumpDialog.m_Cmd = XCMD_CompressedGumpDialog;
	cmd.CompressedGumpDialog.m_len = 0;	// to set later
	cmd.CompressedGumpDialog.m_UID = pObj->GetUID();
	cmd.CompressedGumpDialog.m_context = context_mode;
	cmd.CompressedGumpDialog.m_x = x;
	cmd.CompressedGumpDialog.m_y = y;
	cmd.CompressedGumpDialog.m_compressed_lenCmds = 0;
	cmd.CompressedGumpDialog.m_uncompressed_lenCmds = lengthControls;
	len += 27;

	{
		z_uLong mCompressLen = z_compressBound( lengthControls );
		BYTE * mCompress =  new BYTE[mCompressLen];

		TCHAR * pszFull;
		TEMPSTRING(pszMsg);
		if ( lengthControls > 4096 )
		{
			pszFull = new TCHAR[lengthControls];
			*pszFull = 0;
		}
		else
		{
			TEMPSTRING(secondTemp);
			pszFull = secondTemp;
		}

		for ( it = psControls->begin(); it != psControls->end(); it++ )
		{
			sprintf(pszMsg, "{%s}", (LPCTSTR)it->m_contents);
			strcat(pszFull, pszMsg);
		}

		int error = z_compress2(mCompress, &mCompressLen, (BYTE *)pszFull, lengthControls, Z_DEFAULT_COMPRESSION);
		if ( lengthControls > 4096 ) { delete [] pszFull; }

		if ( error != Z_OK )
		{
			delete [] mCompress;
			g_Log.Error("Compress failed with error %d when generating gump.\n", error);
			return;
		}
		memcpy((&cmd.CompressedGumpDialog.m_Cmd + len), mCompress, mCompressLen);
		
		cmd.CompressedGumpDialog.m_compressed_lenCmds = (DWORD) mCompressLen + sizeof(cmd.CompressedGumpDialog.m_uncompressed_lenCmds);

		delete [] mCompress;

		len += (int)mCompressLen;
	}

	NDWORD * m_lineTxts = (NDWORD *)(&cmd.CompressedGumpDialog.m_Cmd + len);
	len += sizeof(NDWORD);
	*m_lineTxts = (DWORD) iTexts;

	NDWORD * m_compressed_lenTxts = (NDWORD *)(&cmd.CompressedGumpDialog.m_Cmd + len);
	len += sizeof(NDWORD);

	NDWORD * m_uncompressed_lenTxts = (NDWORD *)(&cmd.CompressedGumpDialog.m_Cmd + len);
	len += sizeof(NDWORD);

	{
		int mLen = 0;
		int iLenghtTxt = lengthText - lengthControls - 23;
		BYTE * m_Deflated = new BYTE[iLenghtTxt];

		for ( it = psText->begin(); it != psText->end(); it++ )
		{
			int wLen1 = it->m_contents.GetLength();
			NWORD wLen2; wLen2 = (WORD) wLen1;
			
			memcpy( (m_Deflated + mLen), &wLen2, sizeof(NWORD) );
			mLen += sizeof(NWORD);

			if ( wLen1 )
			{
				NCHAR szTmp[MAX_TALK_BUFFER];
				int len3 = CvtSystemToNUNICODE( szTmp, COUNTOF(szTmp), it->m_contents, wLen1 );
				memcpy( (m_Deflated + mLen), &szTmp, wLen1*sizeof(NCHAR) );
				mLen += (wLen1*sizeof(NCHAR));
			}
		}

		if ( mLen != iLenghtTxt )
		{
			delete [] m_Deflated;
			g_Log.Error("Mismatch in text length (%d - %d).\n", (mLen + 2), iLenghtTxt);
			return;
		}

		z_uLong mCompressLen = z_compressBound( iLenghtTxt );
		BYTE * mCompress = new BYTE[mCompressLen];

		int error = z_compress2(mCompress, &mCompressLen, m_Deflated, iLenghtTxt, Z_DEFAULT_COMPRESSION);
		delete [] m_Deflated;

		if ( error != Z_OK )
		{
			delete [] mCompress;

			g_Log.Error("Compress failed with error %d when generating gump.\n", error);
			return;
		}

		memcpy((&cmd.CompressedGumpDialog.m_Cmd + len), mCompress, mCompressLen);

		*m_compressed_lenTxts = (DWORD) mCompressLen + sizeof(cmd.CompressedGumpDialog.m_uncompressed_lenCmds);
		*m_uncompressed_lenTxts = (DWORD) iLenghtTxt;

		delete [] mCompress;

		len += (int)mCompressLen;
	}

	cmd.CompressedGumpDialog.m_len = len;
	xSend( &cmd, len );

	if ( m_pChar )
	{
		bool bResDialogOk = false;

		CItemMemory * pMemory = m_pChar->Memory_AddObj(this->GetChar(), MEMORY_GUMPRECORD);
		pMemory->m_itNormal.m_more1 = context_mode;
		pMemory->m_itNormal.m_more2 = pObj->GetUID();
		pMemory->SetName("Gump Memory");

		if (context_mode == CLIMODE_DIALOG_GUILD)
		{
			pMemory->GetTagDefs()->SetStr( "dialog_name", false, "guild");
			pMemory->GetTagDefs()->SetNum( "targ_uid", (DWORD) m_Targ_UID , false);
			bResDialogOk = true;
		}
		else
		{
			CResourceDef *	pRes = g_Cfg.ResourceGetDef(RESOURCE_ID( RES_DIALOG, context_mode ));

			if ( pRes )
			{
				CDialogDef * pDlg = dynamic_cast <CDialogDef*>(pRes);
				if ( pDlg )
				{
					pMemory->GetTagDefs()->SetStr("dialog_name", false, pDlg->GetName());
					bResDialogOk = true;
				}
			}
		}

		if ( !bResDialogOk )
			pMemory->GetTagDefs()->SetStr( "dialog_name", false, "undefinied");
	}
	
}

bool CClient::addGumpDialogProps( UID uid )
{
	// put up a prop dialog for the object.
	CObjBase * pObj = uid.ObjFind();
	if ( pObj == NULL )
		return false;
	if ( m_pChar == NULL )
		return false;
	if ( ! m_pChar->CanTouch( pObj ))	// probably a security issue.
		return false;

	m_Prop_UID = m_Targ_UID = uid;
	if ( uid.IsChar())
	{
		addSkillWindow((SKILL_TYPE)MAX_SKILL, true);
	}

	TEMPSTRING(pszMsg);
	strcpy(pszMsg, pObj->IsItem() ? "d_ITEMPROP1" : "d_CHARPROP1" );

	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pszMsg);
	if ( ! rid.IsValidUID())
		return false;

	Dialog_Setup( CLIMODE_DIALOG, rid, 0, pObj );
	return true;
}

TRIGRET_TYPE CClient::Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs )
{
	// one of the gump dialog buttons was pressed.
	if ( pObj == NULL )		// object is gone ?
		return TRIGRET_ENDIF;

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, RESOURCE_ID( RES_DIALOG, rid.GetResIndex(), RES_DIALOG_BUTTON )))
	{
		return TRIGRET_ENDIF;
	}

	// Set the auxiliary stuff for INPDLG here
	// m_tmInpVal.m_PrvGumpID	= rid;
	// m_tmInpVal.m_UID		= pObj ? pObj->GetUID() : (UID) 0;

	int piCmd[3];
	int iArgs;
	while ( s.ReadKeyParse())
	{
		if ( ! s.IsKeyHead( "ON", 2 ))
			continue;

		iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd) );
		if ( iArgs == 0 )		continue;

		if ( iArgs == 1 ?
				piCmd[0] != dwButtonID
			 :	( dwButtonID < piCmd[0]  || dwButtonID > piCmd[1] ) )
			continue;

		pArgs->m_iN1	= dwButtonID;		
		return pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, m_pChar, pArgs );
	}

	return( TRIGRET_ENDIF );
}

TRIGRET_TYPE CClient::Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj ) // Menus for general purpose
{
	// A select was made. so run the script.
	// iSelect = 0 = cancel.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		// Can't find the resource ?
		return( TRIGRET_ENDIF );
	}

	if ( pObj == NULL )
		pObj = m_pChar;

	// execute the menu script.
	if ( iSelect == 0 )	// Cancel button
	{
		while ( s.ReadKeyParse() )
		{
			if ( !s.IsKey( "ON" ) || ( *s.GetArgStr() != '@' ) )
				continue;

			if ( strcmpi( s.GetArgStr(), "@cancel" ) )
				continue;

			return pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
		}
	}
	else
	{
		int i=0;	// 1 based selection.
		while ( s.ReadKeyParse())
		{
			if ( !s.IsKey( "ON" ) || ( *s.GetArgStr() == '@' ) )
				continue;

			i++;
			if ( i < iSelect )
				continue;
			if ( i > iSelect )
				break;

			return pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
		}
	}

	// No selection ?
	return( TRIGRET_ENDIF );
}

bool CMenuItem::ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc )
{
	if ( *pszArgs == '@' )
	{
		// This allows for triggers in menus
		return false;
	}

	TCHAR * pszArgStart = pszArgs;
	while ( _ISCSYM( *pszArgs ))
		pszArgs++;

	if ( *pszArgs )
	{
		*pszArgs = '\0';
		pszArgs++;
		GETNONWHITESPACE( pszArgs );
	}

	// The item id (if we want to have an item type menu) or 0

	if ( strcmp( pszArgStart, "0" ))
	{
		m_id = (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, pszArgStart );
		CItemBase * pItemBase = CItemBase::FindItemBase( (ITEMID_TYPE) m_id );
		if ( pItemBase != NULL )
		{
			m_id = pItemBase->GetDispID();
			pObjBase = pItemBase;
		}
		else
		{
			g_Log.Error("Bad MENU item id '%s'\n", pszArgStart);
			return false;
		}
	}
	else
	{
		m_id = 0;
	}

	if ( pObjBase != NULL )
	{
		pObjBase->ParseText( pszArgs, pSrc );
	}
	else
	{
		g_Serv.ParseText( pszArgs, pSrc );
	}

	// Parsing @color
	if ( *pszArgs == '@' )
	{
		pszArgs++;
		m_color = Exp_GetVal( pszArgs );
		SKIP_ARGSEP( pszArgs );
	}
	else
		m_color = 0;

	m_sText = pszArgs;

	if ( m_sText.IsEmpty())
	{
		if ( pObjBase )	// use the objects name by default.
		{
			m_sText = pObjBase->GetName();	
			if ( ! m_sText.IsEmpty())
				return true;
		}
		g_Log.Error("Bad MENU item text '%s'\n", pszArgStart);
	}

	return( !m_sText.IsEmpty() );
}

void CClient::Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj )
{
	// Menus for general purpose
	// Expect Event_MenuChoice() back.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		return;
	}

	if ( pObj == NULL )
	{
		pObj = m_pChar;
	}

	s.ReadKey();	// get title for the menu.
	pObj->ParseText( s.GetKeyBuffer(), m_pChar );

	CMenuItem item[MAX_MENU_ITEMS];
	item[0].m_sText = s.GetKey();
	// item[0].m_id = rid.m_internalrid;	// general context id

	int i=0;
	while (s.ReadKeyParse())
	{
		if ( ! s.IsKey( "ON" ))
			continue;
		if ( ++i >= COUNTOF( item ))
			break;
		if ( ! item[i].ParseLine( s.GetArgRaw(), pObj, m_pChar ))
		{
			i--;			
		}
	}

	m_tmMenu.m_ResourceID = rid;

	addItemMenu( CLIMODE_MENU, item, i, pObj );
}

