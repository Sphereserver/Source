#ifndef VARIABLE_LIST_H
#define VARIABLE_LIST_H
#pragma once

#include <set>

/***************************************************************************
 *
 *
 *	class VariableList			Holds list of pairs KEY = VALUE and operates it
 *
 *
 ***************************************************************************/
class VariableList
{
public:
	//	public interface for variables storage
	class Variable
	{
	private:
		CGString m_Key;

	public:
		Variable(LPCTSTR pszKey);
		~Variable();

		LPCTSTR GetKey() const;

		virtual LPCTSTR GetValStr() const = 0;
		virtual int GetValNum() const = 0;
		virtual Variable * CopySelf() const = 0;
	};
	struct ltstr
	{
		bool operator()(Variable * s1, Variable * s2) const;
	};
	typedef std::set<Variable *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

private:
	DefSet m_Container;

public:
	void Copy(const VariableList *pArray, bool deep = false);
	void Empty();
	int GetCount() const;

	VariableList &operator=(const VariableList &array);
	~VariableList();

	LPCTSTR FindValNum(int iVal) const;
	LPCTSTR FindValStr(LPCTSTR pVal) const;

	int SetNumNew(LPCTSTR pszKey, int iVal);
	int SetNum(LPCTSTR pszKey, int iVal, bool fZero = false);
	int SetStrNew(LPCTSTR pszKey, LPCTSTR pszVal);
	int SetStr(LPCTSTR pszKey, bool fQuoted, LPCTSTR pszVal, bool fZero = false);

	Variable *GetAt(int at);
	Variable *GetKey(LPCTSTR pszKey) const;
	int GetKeyNum(LPCTSTR pszKey, bool fZero = false) const;
	LPCTSTR GetKeyStr(LPCTSTR pszKey, bool fZero = false) const;
	Variable *GetParseKey(LPCTSTR &pArgs) const;
	bool GetParseVal(LPCTSTR &pArgs, long *plVal) const;

	void DumpKeys(CTextConsole *pSrc, LPCTSTR pszPrefix = NULL);
	void ClearKeys(LPCTSTR mask = NULL);
	void DeleteKey(LPCTSTR key);

	bool r_LoadVal(CScript &s);
	void r_WritePrefix(CScript &s, LPCTSTR pszPrefix);

private:
	Variable *GetAtKey(LPCTSTR at);
	void DeleteAt(int at);
	void DeleteAtKey(LPCTSTR at);
	void DeleteAtIterator(DefSet::iterator it);

	int SetNumOverride(LPCTSTR pszKey, int iVal);
	int SetStrOverride(LPCTSTR pszKey, LPCTSTR pszVal);

private:
	class VariableSearcher : public Variable
	{
	public:
		VariableSearcher(LPCTSTR pszKey);
		~VariableSearcher();

		LPCTSTR GetValStr() const;
		int GetValNum() const;
		virtual Variable *CopySelf() const;
	};

	class VariableNumber : public Variable
	{
	private:
		int m_iVal;

	public:
		VariableNumber(LPCTSTR pszKey, int iVal);
		VariableNumber(LPCTSTR pszKey);
		~VariableNumber();

		int GetValNum() const;
		void SetValNum(int iVal);
		LPCTSTR GetValStr() const;

		bool r_LoadVal(CScript &s);
		bool r_WriteVal(LPCTSTR pKey, CGString &sVal, CTextConsole *pSrc = NULL);

		virtual Variable *CopySelf() const;
	};

	class VariableString : public Variable
	{
	private:
		CGString m_sVal;

	public:
		VariableString(LPCTSTR pszKey, LPCTSTR pszVal);
		VariableString(LPCTSTR pszKey);
		~VariableString();

		LPCTSTR GetValStr() const;
		void SetValStr(LPCTSTR pszVal);
		int GetValNum() const;

		bool r_LoadVal(CScript &s);
		bool r_WriteVal(LPCTSTR pKey, CGString &sVal, CTextConsole *pSrc = NULL);

		virtual Variable *CopySelf() const;
	};
};

#endif