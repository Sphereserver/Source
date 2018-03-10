//
// CGMPage.h
//

#ifndef _INC_CGMPAGE_H
#define _INC_CGMPAGE_H
#pragma once

class CGMPage : public CGObListRec, public CScriptObj
{
	// RES_GMPAGE
	// Only one page allowed per account at a time
	static LPCTSTR const sm_szLoadKeys[];
private:
	CGString m_sAccount;	// account that paged me
	CClient *m_pGMClient;	// assigned to a GM
	CGString m_sReason;		// players reason for call

public:
	static const char *m_sClassName;
	// Queue a GM page (based on account)
	CServTime m_timePage;	// time of the last call
	CPointMap m_ptOrigin;	// Origin point of call

public:
	CGMPage(LPCTSTR pszAccount);
	~CGMPage();

private:
	CGMPage(const CGMPage &copy);
	CGMPage &operator=(const CGMPage &other);

public:
	CAccountRef FindAccount() const;
	LPCTSTR GetAccountStatus() const;
	LPCTSTR GetName() const
	{
		return m_sAccount;
	}
	LPCTSTR GetReason() const
	{
		return m_sReason;
	}
	void SetReason(LPCTSTR pszReason)
	{
		m_sReason = pszReason;
	}
	CClient *FindGMHandler() const
	{
		return m_pGMClient;
	}
	void ClearGMHandler();
	void SetGMHandler(CClient *pClient);
	INT64 GetAge() const;

	bool r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc);
	void r_Write(CScript &s) const;
	bool r_LoadVal(CScript &s);

	CGMPage *GetNext() const
	{
		return static_cast<CGMPage *>(CGObListRec::GetNext());
	}
};

#endif	// _INC_CGMPAGE_H
