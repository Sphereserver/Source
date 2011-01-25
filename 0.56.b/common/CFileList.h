//
// CFileList.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CFILELIST_H
#define _INC_CFILELIST_H
#pragma once

#include "CAssoc.h"
#include <time.h>

class CFileList : public CGStringList
{
public:
	static const char *m_sClassName;
	static bool ReadFileInfo( LPCTSTR pszFilePath, time_t & dwDateChange, DWORD & dwSize );
	int ReadDir( LPCTSTR pszFilePath, bool bShowError = true );

public:
	CFileList() { };
private:
	CFileList(const CFileList& copy);
	CFileList& operator=(const CFileList& other);
};

#endif	// _INC_CFILELIST_H
