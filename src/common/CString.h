/**
* @file CString.
* @brief Custom String implementation.
*/

#ifndef _INC_CSTRING_H
#define _INC_CSTRING_H
#pragma once

#include "os_common.h"

/**
* @brief Custom String implementation.
*/
class CGString
{
private:
	TCHAR	*m_pchData; ///< Data pointer.
	int		m_iLength; ///< Length of string.
	int		m_iMaxLength; ///< Size of memory allocated pointed by m_pchData.

public:
	static const char *m_sClassName;

private:
	/**
	* @brief Initializes internal data.
	*
	* Allocs STRING_DEFAULT_SIZE by default. If DEBUG_STRINGS setted, updates statistical information (total memory allocated).
	*/
	void Init();

public:
	/**
	* @brief CGString destructor.
	*
	* If DEBUG_STRINGS setted, updates statistical information (total CGString instantiated).
	*/
	~CGString();
	/**
	* @brief Default constructor.
	*
	* Initializes string. If DEBUG_STRINGS setted, update statistical information (total CGString instantiated).
	* @see Init()
	*/
	CGString();
	/**
	* @brief Copy constructor.
	*
	* @see Copy()
	* @param pStr string to copy.
	*/
	CGString(LPCTSTR pStr);
	/**
	* @brief Copy constructor.
	*
	* @see Copy()
	* @param pStr string to copy.
	*/
	CGString(const CGString &s);

	/**
	* @brief Check if there is data allocated and if the string is zero ended.
	* @return true if is valid, false otherwise.
	*/
	bool IsValid() const;
	/**
	* @brief Change the length of the CGString.
	*
	* If the new length is lesser than the current lenght, only set a zero at the end of the string.
	* If the new length is bigger than the current length, alloc memory for the string and copy.
	* If DEBUG_STRINGS setted, update statistical information (reallocs count, total memory allocated).
	* @param iLen new length of the string.
	* @return the new length of the CGString.
	*/
	int SetLength(int iLen);
	/**
	* @brief Get the length of the CGString.
	* @return the length of the CGString.
	*/
	int GetLength() const;
	/**
	* @brief Check the length of the CGString.
	* @return true if length is 0, false otherwise.
	*/
	bool IsEmpty() const;
	/**
	* @brief Sets length to zero.
	*
	* If bTotal is true, then free the memory allocated. If DEBUG_STRINGS setted, update statistical information (total memory allocated).
	* @param bTotal true for free the allocated memory.
	*/
	void Empty(bool bTotal = false);
	/**
	* @brief Copy a string into the CGString.
	* @see SetLength()
	* @see strcpylen()
	* @param pStr string to copy.
	*/
	void Copy(LPCTSTR pStr);

	/**
	* @brief Gets the reference to character a specified position (0 based).
	* @param nIndex position of the character.
	* @return reference to character in position nIndex.
	*/
	TCHAR & ReferenceAt(int nIndex);
	/**
	* @brief Gets the caracter in a specified position (0 based).
	* @param nIndex position of the character.
	* @return character in position nIndex.
	*/
	TCHAR GetAt(int nIndex) const;
	/**
	* @brief Puts a character in a specified position (0 based).
	*
	* If character is 0, updates the length of the string (truncated).
	* @param nIndex position to put the character.
	* @param ch character to put.
	*/
	void SetAt(int nIndex, TCHAR ch);
	/**
	* @brief Gets the internal pointer.
	* @return Pointer to internal data.
	*/
	LPCTSTR GetPtr() const;

	/**
	* @brief Join a formated string (printf like) with values and copy into this.
	* @param pStr formated string.
	* @param args list of values.
	*/
	void FormatV(LPCTSTR pStr, va_list args);
	/**
	* @brief Join a formated string (printf like) with values and copy into this.
	* @see FormatV()
	* @param pStr formated string.
	* @param ... list of values.
	*/
	void _cdecl Format(LPCTSTR pStr, ...) __printfargs(2, 3);
	/**
	* @brief Print a long value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatVal(long iVal);
	/**
	* @brief Print a long long value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatLLVal(long long iVal);
	/**
	* @brief Print a unsigned long long value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatULLVal(unsigned long long iVal);
	/**
	* @brief Print a unsigned long value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatUVal(unsigned long iVal);
	/**
	* @brief Print a DWORD value into the string (hex format).
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatHex(DWORD dwVal);
	/**
	* @brief Print a unsigned long long value into the string (hex format).
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatLLHex(unsigned long long dwVal);

	/**
	* @brief Compares the CGString to string pStr (strcmp wrapper).
	*
	* This function starts comparing the first character of CGString and the string.
	* If they are equal to each other, it continues with the following
	* pairs until the characters differ or until a terminating null-character
	* is reached. This function performs a binary comparison of the characters.
	* @param pStr string to compare.
	* @return <0 if te first character that not match has lower value in CGString than in pStr. 0 if hte contents of both are equal. >0 if the first character that does not match has greater value in CGString than pStr.
	*/
	int Compare(LPCTSTR pStr) const;
	/**
	* @brief Compares the CGString to string pStr (case insensitive) (_strcmpi wrapper).
	*
	* This function starts comparing the first character of CGString and the string.
	* If they are equal to each other, it continues with the following
	* pairs until the characters differ or until a terminating null-character
	* is reached. This function performs a case insensitive comparison of the characters.
	* @param pStr string to compare.
	* @return <0 if te first character that not match has lower value in CGString than in pStr. 0 if hte contents of both are equal. >0 if the first character that does not match has greater value in CGString than pStr.
	*/
	int CompareNoCase(LPCTSTR pStr) const;
	/**
	* @brief Look for the first occurence of c in CGString.
	* @param c character to look for.
	* @return position of the character in CGString if any, -1 otherwise.
	*/
	int indexOf(TCHAR c);
	/**
	* @brief Look for the first occurence of c in CGString from a position.
	* @param c character to look for.
	* @param offset position from start the search.
	* @return position of the character in CGString if any, -1 otherwise.
	*/
	int indexOf(TCHAR c, int offset);
	/**
	* @brief Look for the first occurence of a substring in CGString from a position.
	* @param str substring to look for.
	* @param offset position from start the search.
	* @return position of the substring in CGString if any, -1 otherwise.
	*/
	int indexOf(CGString str, int offset);
	/**
	* @brief Look for the first occurence of a substring in CGString.
	* @param str substring to look for.
	* @return position of the substring in CGString if any, -1 otherwise.
	*/
	int indexOf(CGString str);
	/**
	* @brief Look for the last occurence of c in CGString.
	* @param c character to look for.
	* @return position of the character in CGString if any, -1 otherwise.
	*/
	int lastIndexOf(TCHAR c);
	/**
	* @brief Look for the last occurence of c in CGString from a position to the end.
	* @param c character to look for.
	* @param from position where stop the search.
	* @return position of the character in CGString if any, -1 otherwise.
	*/
	int lastIndexOf(TCHAR c, int from);
	/**
	* @brief Look for the last occurence of a substring in CGString from a position to the end.
	* @param str substring to look for.
	* @param from position where stop the search.
	* @return position of the substring in CGString if any, -1 otherwise.
	*/
	int lastIndexOf(CGString str, int from);
	/**
	* @brief Look for the last occurence of a substring in CGString.
	* @param str substring to look for.
	* @return position of the substring in CGString if any, -1 otherwise.
	*/
	int lastIndexOf(CGString str);
	/**
	* @brief Adds a char at the end of the CGString.
	* @param ch character to add.
	*/
	void Add(TCHAR ch);
	/**
	* @brief Adds a string at the end of the CGString.
	* @parampszStrh string to add.
	*/
	void Add(LPCTSTR pszStr);
	/**
	* @brief Reverses the CGString.
	*/
	void Reverse();
	/**
	* @brief Changes the capitalization of CGString to upper.
	*/
	void MakeUpper() { _strupr(m_pchData); }
	/**
	* @brief Changes the capitalization of CGString to lower.
	*/
	void MakeLower() { _strlwr(m_pchData); }

	/**
	* @brief Gets the caracter in a specified position (0 based).
	* @see GetAt()
	* @param nIndex position of the character.
	* @return character in position nIndex.
	*/
	TCHAR operator[](int nIndex) const
	{
		return GetAt(nIndex);
	}
	/**
	* @brief Gets the reference to character a specified position (0 based).
	* @see ReferenceAt()
	* @param nIndex position of the character.
	* @return reference to character in position nIndex.
	*/
	TCHAR & operator[](int nIndex)
	{
		return ReferenceAt(nIndex);
	}
	/**
	* @brief cast as const LPCSTR.
	* @return internal data pointer.
	*/
	operator LPCTSTR() const
	{
		return(GetPtr());
	}
	/**
	* @brief Concatenate CGString with a string.
	* @param psz string to concatenate with.
	* @return The result of concatenate the CGString with psz.
	*/
	const CGString& operator+=(LPCTSTR psz)	// like strcat
	{
		Add(psz);
		return(*this);
	}
	/**
	* @brief Concatenate CGString with a character.
	* @param ch character to concatenate with.
	* @return The result of concatenate the CGString with ch.
	*/
	const CGString& operator+=(TCHAR ch)
	{
		Add(ch);
		return(*this);
	}
	/**
	* @brief Copy supplied string into the CGString.
	* @param pStr string to copy.
	* @return the CGString.
	*/
	const CGString& operator=(LPCTSTR pStr)
	{
		Copy(pStr);
		return(*this);
	}
	/**
	* @brief Copy supplied CGString into the CGString.
	* @param s CGString to copy.
	* @return the CGString.
	*/
	const CGString& operator=(const CGString &s)
	{
		Copy(s.GetPtr());
		return(*this);
	}
};

/**
* match result defines
*/
enum MATCH_TYPE
{
	MATCH_INVALID = 0,
	MATCH_VALID,		///< valid match
	MATCH_END,			///< premature end of pattern string
	MATCH_ABORT,		///< premature end of text string
	MATCH_RANGE,		///< match failure on [..] construct
	MATCH_LITERAL,		///< match failure on literal match
	MATCH_PATTERN		///< bad pattern
};


/**
* @brief check if a string matches a pattern.
* @see MATCH_TYPE
* @param pPattern pattern to match.
* @param pText text to match against the pattern.
* @return a MATCH_TYPE
*/
MATCH_TYPE Str_Match(LPCTSTR pPattern, LPCTSTR pText);

/**
* @brief check if a string matches a regex.
* @param pPattern regex to match.
* @param pText text to match against the regex.
* @param lastError if any error, error description is stored here.
* @return 1 is regex is matched, 0 if not, -1 if errors.
*/
int Str_RegExMatch(LPCTSTR pPattern, LPCTSTR pText, TCHAR * lastError);

/**
* @brief Wrapper to cstring strcpy, but returns the length of the string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @return length of the string copied.
*/
size_t strcpylen(TCHAR * pDst, LPCTSTR pSrc);
/**
* @brief Wrapper to cstring strncpy, but returns the length of string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @param iMaxSize max data to be coppied.
* @return length of the string copied.
*/
size_t strcpylen(TCHAR * pDst, LPCTSTR pSrc, size_t imaxlen);

/**
* @brief Give the article and space to a word. For example, for "boot" will return "a ".
* @param pszWords word to add the article.
* @return string with the article and a space.
*/
LPCTSTR Str_GetArticleAndSpace(LPCTSTR pszWords);
/**
* @brief Filter specific characters from a string.
* @param pszOut output string.
* @param pszInp input string.
* @param iMaxSize max output size.
* @param pszStrip characters to strip (default "{|}~", non printable characters for client).
* @return size of the filtered string.
*/
size_t Str_GetBare(TCHAR * pszOut, LPCTSTR pszInp, size_t iMaxSize, LPCTSTR pszStrip = NULL);
/**
* @param pszIn string to check.
* @return true if string is empty or has '\c' or '\n' characters, false otherwise.
*/
bool Str_Check(LPCTSTR pszIn);
/**
* @param pszIn string to check.
* @return false if string match "[a-zA-Z0-9_- \'\.]+", true otherwise.
*/
bool Str_CheckName(LPCTSTR pszIn);
/**
* @brief replace string representation of special characters by special characters.
*
* Strings replaced:
* - \b
* - \n
* - \r
* - \t
* - \\
* @param pStr string to make replaces on.
* @return string with replaces in (same as pStr).
*/
TCHAR * Str_MakeFiltered(TCHAR * pStr);
/**
* @brief replace special characters by string representation.
*
* Speciual characters replaced:
* - \b
* - \n
* - \r
* - \t
* - \\
* @param pStrOut strint where store the computed string.
* @param pStrIn input string.
* @param iSizeMax length of the input string.
*/
void Str_MakeUnFiltered(TCHAR * pStrOut, LPCTSTR pStrIn, int iSizeMax);
/**
* @brief remove trailing white spaces from a string.
* @param pStr string where remove trailing spaces.
* @param len length of the string.
* @return new lenght of the string.
*/
size_t Str_TrimEndWhitespace(TCHAR * pStr, size_t len);
/**
* @brief Removes heading and trailing white spaces of a string.
* @param pStr string where remove the white spaces.
* @return string with the heading and trailing spaces removed.
*/
TCHAR * Str_TrimWhitespace(TCHAR * pStr);
/**
* @brief find a substring in a string from an offset.
* @param pStr1 string where find the substring.
* @param pStr2 substring to find.
* @param offset position where to start the search.
* @return -1 for a bad offset or if string if not found, otherwise the position of the substring in string.
*/
int Str_IndexOf(TCHAR * pStr1, TCHAR * pStr2, int offset = 0);
/**
* @brief Parse a simple argument from a list of arguments.
* 
* From a line like "    a, 2, 3" it will get "a" (Note that removes heading white spaces) 
* on pLine and  "2, 3" on ppArg.
* @param pLine line to parse and where store the arg parsed.
* @param ppArg where to store the other args (non proccessed pLine).
* @param pSep the list of separators (by default "=, \t").
* @return false if there are no more args to parse, true otherwise.
*/
bool Str_Parse(TCHAR * pLine, TCHAR ** ppArg = NULL, LPCTSTR pSep = NULL);
/**
* @brief Parse a list of arguments.
* @param pCmdLine list of arguments to parse.
* @param ppCmd where to store de parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
size_t Str_ParseCmds(TCHAR * pCmdLine, TCHAR ** ppCmd, size_t iMax, LPCTSTR pSep = NULL);
/**
* @brief Parse a list of arguments (integer version).
* @param pCmdLine list of arguments to parse.
* @param piCmd where to store de parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
size_t Str_ParseCmds(TCHAR * pCmdLine, INT64 * piCmd, size_t iMax, LPCTSTR pSep = NULL);

/**
* @brief Look for a string in a table.
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTable(LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));
/**
* @brief Look for a string in a table (binary search).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableSorted(LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));/**
* @brief Look for a string header in a table (uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHead(LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));
/**
* @brief Look for a string header in a table (binary search, uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHeadSorted(LPCTSTR pFind, LPCTSTR const * ppTable, int iCount, int iElemSize = sizeof(LPCTSTR));

void CharToMultiByteNonNull(BYTE*, const char* , size_t);

// extern TCHAR * Str_GetTemporary(int amount = 1);
#define Str_GetTemp static_cast<AbstractSphereThread *>(ThreadHolder::current())->allocateBuffer

#endif	// _INC_CSTRING_H
