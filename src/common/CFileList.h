/**
* @file CFileList.h
*/

#pragma once
#ifndef _INC_CFILELIST_H
#define _INC_CFILELIST_H


#include "CAssoc.h"
#include <time.h>

/**
* @brief List of CGStrings with methods to read
*/
class CFileList : public CGStringList
{
public:
	static const char *m_sClassName;
	/**
	* @brief Get update time and size of a file.
	* @param pszFilePath file to check.
	* @param dwDateChange update time stored here.
	* @param dwSize size stored here.
	* @return false if can not read info from the file, true if info is setted.
	*/
	static bool ReadFileInfo( LPCTSTR pszFilePath, time_t & dwDateChange, DWORD & dwSize );
	/**
	* @brief Read a dir content and store inside the instance.
	*
	* Content with name that start with '.' are not listed.
	* @param pszFilePath dir to read.
	* @param bShowError Show debug info if can not open the dir.
	* @return -1 on error, otherwise the number of elements listed.
	*/
	int ReadDir( LPCTSTR pszFilePath, bool bShowError = true );

public:
	CFileList() { };
private:
	/**
    * @brief No copy on construction allowed.
    */
	CFileList(const CFileList& copy);
	/**
    * @brief No copy allowed.
    */
	CFileList& operator=(const CFileList& other);
};

#endif	// _INC_CFILELIST_H
