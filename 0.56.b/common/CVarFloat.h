#include <map>

struct LexNoCaseLess {
	bool operator()(const CGString& CGStr, const char* pBeg2) const
	{
		const char* pBeg1 = CGStr.GetPtr();
		const char* pEnd1 = pBeg1;
		const char* pEnd2 = pBeg2;
		while (*pEnd1) 
			++pEnd1;
		while (*pEnd2)
			++pEnd2;
		++pEnd1, ++pEnd2;

  		for (; pBeg1 != pEnd1 && pBeg2 != pEnd2; ++pBeg1, ++pBeg2)
			if (tolower(*pBeg1) < tolower(*pBeg2))
				return (true);
			else if (tolower(*pBeg2) < tolower(*pBeg1))
				return (false);
		return (pBeg1 == pEnd1 && pBeg2 != pEnd2);
	}
};

class CVarFloat
{
public:
	typedef double RealType;
	typedef std::map<CGString, RealType, LexNoCaseLess> MapType;

public: //vars
	CVarFloat();
	~CVarFloat();

private:
	CVarFloat(const CVarFloat& copy);
	CVarFloat& operator=(const CVarFloat& other);

private: //vars
	MapType m_VarMap;

private: //setting, getting
	inline bool Set( const char* VarName, const char* VarValue );
	RealType GetVal( const char* VarName );
public: //setting, getting
	bool Insert( const char* VarName, const char* VarValue, bool ForceSet = false);
	CGString Get( const char* VarName );

public: //parsing
	static CGString FloatMath( LPCTSTR & Expr );
	static RealType MakeFloatMath( LPCTSTR & Expr );
	static RealType GetRandVal( RealType dQty );
	static RealType GetRandVal2( RealType dMin, RealType dMax );
	//Does not work as it should, would be too slow, and nobody needs that
	/*static RealType GetRange( LPCTSTR & pExpr );
	static int GetRangeVals( LPCTSTR & pExpr, RealType * piVals, short int iMaxQty );*/
private: //parsing
	static RealType GetValMath( RealType dVal, LPCTSTR & pExpr );
	static RealType GetSingle( LPCTSTR & pArgs );
};

class CObjBase;

class CLocalObjMap
{
public:
	CLocalObjMap();
	~CLocalObjMap();
private:
	CLocalObjMap(const CLocalObjMap& copy);
	CLocalObjMap& operator=(const CLocalObjMap& other);

public:
	CObjBase * Get( unsigned short Number );
	bool Insert( unsigned short Number, CObjBase * pObj, bool ForceSet = false );
	typedef std::map<unsigned short, CObjBase*> ObjMap;

private:
	ObjMap m_ObjMap;
};
