#include "graysvr.h"	// predef header.
#include "CClient.h"
#pragma warning(disable:4096)
#include "../common/zlib/zlib.h"
#pragma warning(default:4096)

// endgroup, master, hue ????

enum GUMPCTL_TYPE	// controls we can put in a gump.
{
	GUMPCTL_BUTTON	= 0,		// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
	GUMPCTL_BUTTONTILEART,		// NEW: 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
	GUMPCTL_CHECKBOX,			// 6 = x,y,gumpcheck,gumpuncheck,starting state,checkid

	GUMPCTL_CHECKERTRANS,		// NEW: x,y,w,h
	GUMPCTL_CROPPEDTEXT,		// 6 = x,y,sx,sy,color?,startindex

	GUMPCTL_DCROPPEDTEXT,
	GUMPCTL_DHTMLGUMP,
	GUMPCTL_DORIGIN,
	GUMPCTL_DTEXT,
	GUMPCTL_DTEXTENTRY,
	GUMPCTL_DTEXTENTRYLIMITED,

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
	ADDTOCALLSTACK("CDialogDef::GumpAddText");
	m_sText[m_iTexts] = pszText;
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
	

bool CDialogDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
	ADDTOCALLSTACK("CDialogDef::r_Verb");
	EXC_TRY("Verb");
	// The first part of the key is GUMPCTL_TYPE
	LPCSTR		pszKey = s.GetKey();

	int index = FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index < 0 )
	{
		CGString sVal;
		CScriptTriggerArgs Args(s.GetArgRaw());
		if ( r_Call(s.GetKey(), pSrc, &Args, &sVal) )
			return true;
		if (!m_pObj)
			return CResourceLink::r_Verb(s, pSrc);
		return m_pObj->r_Verb(s, pSrc);
	}
	
	LPCSTR	pszArgs	= s.GetArgStr();

	switch( index )
	{
		case GUMPCTL_PAGE:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;

			GET_ABSOLUTE( page );

			if ( page <= 0 )		return true;

			int	iNewPage;
			if ( m_iPage == 0 || page > m_iPage || page == 0 )
				iNewPage	= page;
			else if ( page == m_iPage  )
				iNewPage	= 1;
			else
				iNewPage	= page + 1;	

			m_sControls[m_iControls].Format( "page %d", iNewPage );
			m_iControls++;
			return true;
		}
		case GUMPCTL_BUTTON:			// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
		case GUMPCTL_BUTTONTILEART:		// 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;

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
				m_sControls[m_iControls].Format( "button %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id );
			else
			{
				GET_ABSOLUTE( tileId );
				GET_ABSOLUTE( tileHue );
				GET_ABSOLUTE( tileX );
				GET_ABSOLUTE( tileY );				
				
				m_sControls[m_iControls].Format( "buttontileart %d %d %d %d %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id, tileId, tileHue, tileX, tileY );
			}

			m_iControls++;
			return true;
		}
		case GUMPCTL_GUMPPIC:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs );

			m_sControls[m_iControls].Format( "gumppic %d %d %d%s%s", x, y, id, *pszArgs ? " hue=" : "", *pszArgs ? pszArgs : "" );
			m_iControls++;
			return true;
		}
		case GUMPCTL_GUMPPICTILED:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( sX );
			GET_ABSOLUTE( sY );
			GET_ABSOLUTE( id );

			m_sControls[m_iControls].Format( "gumppictiled %d %d %d %d %d", x, y, sX, sY, id );
			m_iControls++;
			return true;
		}
		case GUMPCTL_RESIZEPIC:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			GET_ABSOLUTE( sX );
			GET_ABSOLUTE( sY );

			m_sControls[m_iControls].Format( "resizepic %d %d %d %d %d", x, y, id, sX, sY );
			m_iControls++;
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
				m_sControls[m_iControls].Format( "tilepic %d %d %d", x, y, id );
			else
				m_sControls[m_iControls].Format( "tilepichue %d %d %d%s%s", x, y, id, *pszArgs ? " " : "", *pszArgs ? pszArgs : "" );

			m_iControls++;
			return true;
		}
		case GUMPCTL_DTEXT:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			m_sControls[m_iControls].Format( "text %d %d %d %d", x, y, hue, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DCROPPEDTEXT:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;
			
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			m_sControls[m_iControls].Format( "croppedtext %d %d %d %d %d %d", x, y, w, h, hue, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DHTMLGUMP:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( bck );
			GET_ABSOLUTE( options );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			m_sControls[m_iControls].Format( "htmlgump %d %d %d %d %d %d %d", x, y, w, h, iText, bck, options );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DTEXTENTRY:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			m_sControls[m_iControls].Format( "textentry %d %d %d %d %d %d %d", x, y, w, h, hue, id, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DTEXTENTRYLIMITED:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			GET_ABSOLUTE( id );
			GET_ABSOLUTE( charLimit );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : "" );
			m_sControls[m_iControls].Format( "textentrylimited %d %d %d %d %d %d %d %d", x, y, w, h, hue, id, iText, charLimit );
			m_iControls++;
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
			
			return true;
		}
		case GUMPCTL_CROPPEDTEXT:
		case GUMPCTL_TEXT:
		case GUMPCTL_TEXTENTRY:
		case GUMPCTL_TEXTENTRYLIMITED:
			break;
		default:
			break;
	}

	if ( m_iControls >= COUNTOF(m_sControls)-1 )
		return false;

	m_sControls[m_iControls].Format("%s %s", pszKey, pszArgs);
	m_iControls++;
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}


CDialogDef::CDialogDef( RESOURCE_ID rid ) :
	CResourceLink( rid )
{
		m_iControls		= 0;
		m_iTexts		= 0;
		m_pObj		= NULL;
}


bool	CDialogDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CDialogDef::r_WriteVal");
	if ( !m_pObj )
		return false;
	return m_pObj->r_WriteVal( pszKey, sVal, pSrc );
}


bool		CDialogDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CDialogDef::r_LoadVal");
	if ( !m_pObj )
		return false;
	return m_pObj->r_LoadVal( s );
}


bool CDialogDef::GumpSetup( int iPage, CClient * pClient, CObjBase * pObjSrc, LPCTSTR Arguments )
{
	ADDTOCALLSTACK("CDialogDef::GumpSetup");
	CResourceLock	s;

	m_iControls		= 0;
	m_iTexts		= 0;
	m_pObj			= pObjSrc;
	m_iOriginX		= 0;
	m_iOriginY		= 0;
	m_iPage			= iPage;

	CScriptTriggerArgs	Args(iPage, 0, pObjSrc);
	//DEBUG_ERR(("Args.m_s1_raw %s  Args.m_s1 %s  Arguments 0x%x\n",Args.m_s1_raw, Args.m_s1, Arguments));
	Args.m_s1_raw = Args.m_s1 = Arguments;

	// read text first
	if ( g_Cfg.ResourceLock( s, RESOURCE_ID( RES_DIALOG, GetResourceID().GetResIndex(), RES_DIALOG_TEXT ) ) )
	{
		while ( s.ReadKey())
		{
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				break;
			m_pObj->ParseText( s.GetKeyBuffer(), pClient->GetChar() );
			m_sText[m_iTexts] = s.GetKey();
			m_iTexts++;
		}
	}
	else
	{
		// no gump text?
	}

	// read the main dialog
	if ( !ResourceLock( s ) )
		return false;
	if ( !s.ReadKey() )		// read the size.
		return( false );

	// starting x,y location.
	int iSizes[2];
	TCHAR * pszBuf = s.GetKeyBuffer();
	m_pObj->ParseText( pszBuf, pClient->GetChar() );

	Str_ParseCmds( pszBuf, iSizes, COUNTOF(iSizes) );
	m_x		= iSizes[0];
	m_y		= iSizes[1];

	if ( OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, pClient->GetChar(), &Args ) == TRIGRET_RET_TRUE )
		return false;
	return true;
}



bool CClient::Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, LPCTSTR Arguments )
{
	ADDTOCALLSTACK("CClient::Dialog_Setup");
	if ( pObj == NULL )
		return( false );

	CResourceDef *	pRes	= g_Cfg.ResourceGetDef( rid );
	CDialogDef *	pDlg	= dynamic_cast <CDialogDef*>(pRes);
	if ( !pRes )
	{
		DEBUG_ERR(("Invalid RES_DIALOG.\n"));
		return false;
	}

	if ( !pDlg->GumpSetup( iPage, this, pObj, Arguments ) )
		return false;

	// Now pack it up to send,
	// m_tmGumpDialog.m_ResourceID = rid;
	addGumpDialog( mode, pDlg->m_sControls, pDlg->m_iControls, pDlg->m_sText, pDlg->m_iTexts, pDlg->m_x, pDlg->m_y, pObj, rid );
	return( true );
}




void CClient::addGumpInpVal( bool fCancel, INPVAL_STYLE style,
	DWORD iMaxLength,
	LPCTSTR pszText1,
	LPCTSTR pszText2,
	CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addGumpInpVal");
	// CLIMODE_INPVAL
	// Should result in Event_GumpInpValRet
	// just input an objects attribute.

	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	ASSERT( pObj );

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
			"%s (%i chars max)", pszText2, iMaxLength );
		break;
	case INPVAL_STYLE_NUMEDIT: // Numeric
		sprintf(cmd.GumpInpVal.m_text2,
			"%s (0 - %i)", pszText2, iMaxLength );
		break;
	}

	xSendPkt( &cmd, sizeof(cmd.GumpInpVal));

	// m_tmInpVal.m_UID = pObj->GetUID();
	// m_tmInpVal.m_PrvGumpID = m_tmGumpDialog.m_ResourceID;

	m_Targ_UID = pObj->GetUID();
	// m_Targ_Text = verb
	SetTargMode( CLIMODE_INPVAL );
}


void CClient::addGumpDialog( CLIMODE_TYPE mode, const CGString * psControls, int iControls, const CGString * psText, int iTexts, int x, int y, CObjBase * pObj, DWORD rid )
{
	ADDTOCALLSTACK("CClient::addGumpDialog");
	// Add a generic GUMP menu.
	// Should return a Event_GumpDialogRet
	// NOTE: These packets can get rather LARGE.
	// x,y = where on the screen ?

	if ( pObj == NULL )
		pObj = m_pChar;
	int lengthControls=1;
	int i=0;
	for ( ; i < iControls; i++)
	{
		lengthControls += psControls[i].GetLength() + 2;
	}

	int lengthText = lengthControls + 20 + 3;
	for ( i=0; i < iTexts; i++)
	{
		int lentext2 = psText[i].GetLength();
		lengthText += (lentext2*2)+2;
	}

	int	context_mode	= mode;
	if ( mode == CLIMODE_DIALOG && rid != 0 )
	{
		context_mode	= GETINTRESOURCE( rid );
	}

	if ( IsClientVer(0x500000) || IsNoCryptVer(0x500000) )
	{
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
			BYTE * mCompress = new BYTE[mCompressLen];

			TemporaryString pszMsg;
			TCHAR * pszFull = new TCHAR[lengthControls];

			for ( i=0; i<iControls; ++i)
			{
				sprintf(pszMsg, "{%s}", (LPCTSTR) psControls[i]);	
				if ( i == 0 )
					strcpy(pszFull, pszMsg);
				else
					strcat(pszFull, pszMsg);
			}
	
			int error = z_compress2(mCompress, &mCompressLen, (BYTE *)pszFull, lengthControls, Z_DEFAULT_COMPRESSION);
			delete [] pszFull;

			if ( error != Z_OK )
			{
				delete [] mCompress;

				g_Log.EventError( "Compress failed with error %d when generating gump. Using old packet.\n", error );
				goto olddialogprocedure;
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

			for ( int i = 0; i < iTexts; i++ )
			{
				int wLen1 = psText[i].GetLength();
				NWORD wLen2; wLen2 = (WORD) wLen1;
				
				memcpy( (m_Deflated + mLen), &wLen2, sizeof(NWORD) );
				mLen += sizeof(NWORD);

				if ( wLen1 )
				{
					NCHAR szTmp[MAX_TALK_BUFFER];
					int len3 = CvtSystemToNUNICODE( szTmp, COUNTOF(szTmp), psText[i], wLen1 );
					memcpy( (m_Deflated + mLen), &szTmp, wLen1*sizeof(NCHAR) );
					mLen += (wLen1*sizeof(NCHAR));
				}
			}

			if ( mLen != iLenghtTxt )
			{
				delete [] m_Deflated;

				g_Log.EventError( "Mismatch in text length (%d - %d). Using old packet.\n", (mLen + 2), iLenghtTxt );
				goto olddialogprocedure;
			}

			z_uLong mCompressLen = z_compressBound( iLenghtTxt );
			BYTE * mCompress = new BYTE[mCompressLen];

			int error = z_compress2(mCompress, &mCompressLen, m_Deflated, iLenghtTxt, Z_DEFAULT_COMPRESSION);
			delete [] m_Deflated;

			if ( error != Z_OK )
			{
				delete [] mCompress;

				g_Log.EventError( "Compress failed with error %d when generating gump. Using old packet.\n", error );
				goto olddialogprocedure;
			}

			memcpy((&cmd.CompressedGumpDialog.m_Cmd + len), mCompress, mCompressLen);

			*m_compressed_lenTxts = (DWORD) mCompressLen + sizeof(cmd.CompressedGumpDialog.m_uncompressed_lenCmds);
			*m_uncompressed_lenTxts = (DWORD) iLenghtTxt;

			delete [] mCompress;

			len += (int)mCompressLen;
		}

		cmd.CompressedGumpDialog.m_len = len;
		xSend( &cmd, len );

/*
		TCHAR * buf = Str_GetTemp();
		TCHAR * buft = Str_GetTemp();
		BYTE * post = &cmd.CompressedGumpDialog.m_Cmd;

		strcpy(buf, "Packet 0xDD:\n");
		int poss = 0;

		for ( int ilist = 0; ilist < len; ilist++ )
		{
			if ( poss == 0x10 ) { strcat( buf, "\n" ); poss = 0; }
		
			sprintf( buft, "0x%0x ", *post );
			post += 1;
			poss += 1;
			strcat( buf, buft );
		}

		g_Log.Event(0, "%s\n", buf);
*/
	}
	else
	{
olddialogprocedure:
		// Send the fixed length stuff
		CCommand cmd;
		memset( &cmd, 0, sizeof( cmd ) );

		cmd.GumpDialog.m_Cmd = XCMD_GumpDialog;
		cmd.GumpDialog.m_len = lengthText;
		cmd.GumpDialog.m_UID = pObj->GetUID();
		cmd.GumpDialog.m_context = context_mode;
		cmd.GumpDialog.m_x = x;
		cmd.GumpDialog.m_y = y;
		cmd.GumpDialog.m_lenCmds = lengthControls;
		xSend( &cmd, 21, true );

		TemporaryString pszMsg;
		for ( i=0; i<iControls; ++i)
		{
			sprintf(pszMsg, "{%s}", (LPCTSTR) psControls[i]);
			xSend(pszMsg, strlen(pszMsg), true );
		}

		// Pack up the variable length stuff
		BYTE Pkt_gump2[3];
		Pkt_gump2[0] = '\0';
		PACKWORD( &Pkt_gump2[1], iTexts );
		xSend( Pkt_gump2, 3, true);

		// Pack in UNICODE type format.
		for ( i=0; i < iTexts; i++)
		{
			int len1 = psText[i].GetLength();

			NWORD len2;
			len2 = len1;
			xSend( &len2, sizeof(NWORD), true);
			if ( len1 )
			{
				NCHAR szTmp[MAX_TALK_BUFFER];
				int len3 = CvtSystemToNUNICODE( szTmp, COUNTOF(szTmp), psText[i], -1 );
				xSend( szTmp, len2*sizeof(NCHAR), true);
			}
		}
		
		// Send the queued dialog
		xFlush();
	}

	// m_tmGumpDialog.m_UID = pObj->GetUID();

	// SetTargMode( mode );
	
	if ( m_pChar )
	{
		m_mapOpenedGumps[context_mode]++;
	}
	
}

bool CClient::addGumpDialogProps( CGrayUID uid )
{
	ADDTOCALLSTACK("CClient::addGumpDialogProps");
	// put up a prop dialog for the object.
	CObjBase * pObj = uid.ObjFind();
	if ( pObj == NULL )
		return false;
	if ( m_pChar == NULL )
		return( false );
	if ( ! m_pChar->CanTouch( pObj ))	// probably a security issue.
		return( false );

	m_Prop_UID = m_Targ_UID = uid;
	if ( uid.IsChar())
	{
		addSkillWindow((SKILL_TYPE)MAX_SKILL, true);
	}

	TCHAR *pszMsg = Str_GetTemp();
	strcpy(pszMsg, pObj->IsItem() ? "d_ITEMPROP1" : "d_CHARPROP1" );

	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pszMsg);
	if ( ! rid.IsValidUID())
		return false;

	Dialog_Setup( CLIMODE_DIALOG, rid, 0, pObj );
	return( true );
}

TRIGRET_TYPE CClient::Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs )
{
	ADDTOCALLSTACK("CClient::Dialog_OnButton");
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
	// m_tmInpVal.m_UID		= pObj ? pObj->GetUID() : (CGrayUID) 0;

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
		return pObj->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, pArgs );
	}

	return( TRIGRET_ENDIF );
}


bool CClient::Dialog_Close( CObjBase * pObj, RESOURCE_ID_BASE rid, int buttonID )
{
	ADDTOCALLSTACK("CClient::Dialog_Close");
	int gumpContext = GETINTRESOURCE( rid );

	CExtData ExtData;
	ExtData.GumpChange.dialogID		= (DWORD)gumpContext;
	ExtData.GumpChange.buttonID		= buttonID;
	addExtData( EXTDATA_GumpChange, &ExtData, sizeof(ExtData.GumpChange) );

	if ( IsClientVer( 0x400040 ) || IsNoCryptVer( 0x400040 ) )
	{
		CChar * pSrc = dynamic_cast<CChar*>( pObj );
		if ( pSrc )
		{
			OpenedGumpsMap_t::iterator itGumpFound = m_mapOpenedGumps.find( gumpContext );
			if (( itGumpFound != m_mapOpenedGumps.end() ) && ( (*itGumpFound).second > 0 ))
			{
				CEvent eGump;
				eGump.GumpDialogRet.m_Cmd = XCMD_GumpDialogRet;
				eGump.GumpDialogRet.m_len = (WORD) 27;
				eGump.GumpDialogRet.m_UID = (DWORD) pObj->GetUID();
				eGump.GumpDialogRet.m_context = (DWORD) gumpContext;
				eGump.GumpDialogRet.m_buttonID = (DWORD) buttonID;
				eGump.GumpDialogRet.m_checkIds[0] = (DWORD) 0;
				eGump.GumpDialogRet.m_checkQty = (DWORD) 0;
				eGump.GumpDialogRet.m_textQty = (DWORD) 0;
				Event_GumpDialogRet(&eGump);
			}
		}
	}

	return true;
}




TRIGRET_TYPE CClient::Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj ) // Menus for general purpose
{
	ADDTOCALLSTACK("CClient::Menu_OnSelect");
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

			return pObj->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
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

			return pObj->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
		}
	}

	// No selection ?
	return( TRIGRET_ENDIF );
}

bool CMenuItem::ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CMenuItem::ParseLine");

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
	if ( strcmp( pszArgStart, "0" ) )
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
			DEBUG_ERR(( "Bad MENU item id '%s'\n", pszArgStart ));
			return( false );	// skip this.
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
		HUE_TYPE wHue = Exp_GetVal( pszArgs );
		if ( wHue != 0 )
			wHue = (wHue == 1? 0x7FF: wHue-1);

		m_color = wHue;
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
				return( true );
		}
		DEBUG_ERR(( "Bad MENU item text '%s'\n", pszArgStart ));
	}

	return( !m_sText.IsEmpty() );
}

void CClient::Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::Menu_Setup");
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

