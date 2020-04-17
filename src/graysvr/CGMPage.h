#ifndef _INC_CGMPAGE_H
#define _INC_CGMPAGE_H
#pragma once

class CGMPage : public CGObListRec, public CScriptObj
{
	// RES_GMPAGE
public:
	static const char *m_sClassName;
	static const LPCTSTR sm_szLoadKeys[];

	CGMPage(LPCTSTR pszAccount);
	~CGMPage();

public:
	CClient *m_pClientHandling;
	CGString m_sAccount;
	CGrayUID m_uidChar;
	CPointMap m_pt;
	CGString m_sReason;
	CServTime m_time;

public:
	void SetHandler(CClient *pClient);
	void ClearHandler();

	LPCTSTR GetName() const
	{
		return m_sAccount;
	}

	CGMPage *GetNext() const
	{
		return static_cast<CGMPage *>(CGObListRec::GetNext());
	}

	void r_Write(CScript &s) const;
	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	bool r_LoadVal(CScript &s);

private:
	CGMPage(const CGMPage &copy);
	CGMPage &operator=(const CGMPage &other);
};

#endif	// _INC_CGMPAGE_H
