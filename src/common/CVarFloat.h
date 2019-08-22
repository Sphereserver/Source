#ifndef _INC_CVARFLOAT_H
#define _INC_CVARFLOAT_H
#pragma once

#include <map>

struct LexNoCaseLess
{
	bool operator()(const CGString &sVal, LPCTSTR pszBeg2) const
	{
		LPCTSTR pszBeg1 = sVal.GetPtr();
		if ( !pszBeg1 || !pszBeg2 )
			return false;

		LPCTSTR pszEnd1 = pszBeg1;
		LPCTSTR pszEnd2 = pszBeg2;
		while ( pszEnd1 )
			++pszEnd1;
		while ( pszEnd2 )
			++pszEnd2;
		++pszEnd1;
		++pszEnd2;

		for ( ; (pszBeg1 != pszEnd1) && (pszBeg2 != pszEnd2); ++pszBeg1, ++pszBeg2 )
		{
			if ( tolower(*pszBeg1) < tolower(*pszBeg2) )
				return true;
			if ( tolower(*pszBeg2) < tolower(*pszBeg1) )
				return false;
		}
		return ((pszBeg1 == pszEnd1) && (pszBeg2 != pszEnd2));
	}
};

class CVarFloat
{
public:
	CVarFloat();
	~CVarFloat();

public:
	typedef double RealType;
	typedef std::map<CGString, RealType, LexNoCaseLess> MapType;

private:
	MapType m_VarMap;

public:
	bool Insert(LPCTSTR pszName, LPCTSTR pszValue, bool fForceSet = false);
	CGString Get(LPCTSTR pszName);

	static CGString FloatMath(LPCTSTR &pszExpr);
	static RealType MakeFloatMath(LPCTSTR &pszExpr);
	static RealType GetRandVal(RealType dQty);
	static RealType GetRandVal2(RealType dMin, RealType dMax);

private:
	inline bool Set(LPCTSTR pszName, LPCTSTR pszValue);
	RealType GetVal(LPCTSTR pszName);

	static RealType GetValMath(RealType dVal, LPCTSTR &pszExpr);
	static RealType GetSingle(LPCTSTR & pszArgs);

private:
	CVarFloat(const CVarFloat &copy);
	CVarFloat &operator=(const CVarFloat &other);
};

class CObjBase;

class CLocalObjMap
{
public:
	CLocalObjMap();
	~CLocalObjMap();

public:
	typedef std::map<WORD, CObjBase *> ObjMap;

private:
	ObjMap m_ObjMap;

public:
	CObjBase *Get(WORD wNumber);
	bool Insert(WORD wNumber, CObjBase *pObj, bool fForceSet = false);

private:
	CLocalObjMap(const CLocalObjMap &copy);
	CLocalObjMap &operator=(const CLocalObjMap &other);
};

#endif	// _INC_CVARFLOAT_H
