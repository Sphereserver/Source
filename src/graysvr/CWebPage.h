#ifndef _INC_CWEBPAGE_H
#define _INC_CWEBPAGE_H
#pragma once

enum WEBPAGE_TYPE
{
	WEBPAGE_TEMPLATE,
	WEBPAGE_TEXT,
	WEBPAGE_BMP,
	WEBPAGE_GIF,
	WEBPAGE_JPG,
	WEBPAGE_PNG,
	WEBPAGE_QTY
};

enum WTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list
	WTRIG_Load = 1,
	WTRIG_QTY
};

class CWebPageDef : public CResourceLink
{
	// RES_WEBPAGE
	// This is a single web page we are generating or serving
public:
	static const char *m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szPageType[];
	static LPCTSTR const sm_szPageExt[];
	static LPCTSTR const sm_szTrigName[WTRIG_QTY + 1];

	explicit CWebPageDef(RESOURCE_ID rid);
	virtual ~CWebPageDef() { };

public:
	static int sm_iListIndex;

private:
	WEBPAGE_TYPE m_type;		// File type (text, image, etc)
	PLEVEL_TYPE m_privlevel;	// Required plevel to view this page
	CServTime m_timeNextUpdate;
	int m_iUpdatePeriod;		// How often in seconds the page is updated (0 = never)
	int m_iUpdateLog;			// Create a daily copy of the page
	CGString m_sSrcFilePath;	// Determines what page is used as source
	CGString m_sDstFilePath;	// Determines where the final page is saved

public:
	bool WebPageUpdate(bool fNow, LPCTSTR pszDstName, CTextConsole *pSrc);
	void WebPageLog();

	bool SetSourceFile(LPCTSTR pszName, CClient *pClient);
	bool IsMatch(LPCTSTR pszMatch) const;

	bool ServPagePost(CClient *pClient, TCHAR *pszContent, int iContentLength);
	static bool ServPage(CClient *pClient, TCHAR *pszPageName, CGTime *pTimeLastModified);

	LPCTSTR GetName() const
	{
		return m_sSrcFilePath;
	}
	LPCTSTR GetDstName() const
	{
		return m_sDstFilePath;
	}

	virtual bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc = NULL);
	virtual bool r_LoadVal(CScript &s);
	virtual bool r_Verb(CScript &s, CTextConsole *pSrc);	// some command on this object as a target

private:
	int ServPageRequest(CClient *pClient, LPCTSTR pszURLArgs, CGTime *pTimeLastModified);

private:
	CWebPageDef(const CWebPageDef &copy);
	CWebPageDef &operator=(const CWebPageDef &other);
};

class CFileConsole : public CTextConsole
{
public:
	static const char *m_sClassName;
	CFileText m_FileOut;

	CFileConsole() { };

public:
	virtual LPCTSTR GetName() const
	{
		return "WebFile";
	}
	virtual PLEVEL_TYPE GetPrivLevel() const
	{
		return PLEVEL_Admin;
	}
	virtual void SysMessage(LPCTSTR pszMessage) const
	{
		if ( !pszMessage || ISINTRESOURCE(pszMessage) )
			return;
		const_cast<CFileConsole *>(this)->m_FileOut.WriteString(pszMessage);
	}

private:
	CFileConsole(const CFileConsole &copy);
	CFileConsole &operator=(const CFileConsole &other);
};

#endif	// _INC_CWEBPAGE_H
