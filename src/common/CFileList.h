#pragma once
#ifndef _INC_CFILELIST_H
#define _INC_CFILELIST_H

#include "CAssoc.h"

/**
* @brief List of CGStrings with methods to read
*/
class CFileList : public CGStringList
{
public:
	CFileList() { };

public:
	static const char *m_sClassName;

	/**
	* @brief Get update time and size of a file.
	* @param pszPath file to read.
	* @param timeDateChange update time stored here.
	* @param dwSize size stored here.
	* @return false if can not read info from the file, true if info is setted.
	*/
	static bool ReadFileInfo(LPCTSTR pszPath, time_t &timeDateChange, DWORD &dwSize);

	/**
	* @brief Read a dir content and store inside the instance (content with name starting with '.' is not listed).
	* @param pszPath dir to read.
	* @return -1 on error, otherwise the number of elements listed.
	*/
	int ReadDir(LPCTSTR pszPath);

private:
	CFileList(const CFileList &copy);
	CFileList &operator=(const CFileList &other);
};

#endif	// _INC_CFILELIST_H
