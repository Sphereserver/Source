#ifndef _INC_CGRAYINST_H
#define _INC_CGRAYINST_H
#pragma once

#include "../common/graymul.h"
#include "../common/CFile.h"
#include "../common/CArray.h"

////////////////////////////////////////////////////////

extern struct CGrayInstall
{
	// "Software\\Origin Worlds Online\\Ultima Online\\1.0"
	// bool m_fFullInstall;	// Are all files avail ?
private:
	CGString m_sPreferPath;	// Prefer path in which to choose the files. (look here FIRST)
	CGString m_sExePath;		// Files that are installed. "ExePath"
	CGString m_sCDPath;		// For files that may still be on the CD. "InstCDPath"
public:
	CGFile m_File[ VERFILE_QTY ];	// Get our list of files we need to access.

	CGFile	m_Maps[256];		// mapX.mul
	CGFile	m_Statics[256];		// staticsX.mul
	CGFile	m_Staidx[256];		// staidxX.mul

public:
	CGString GetFullExePath( LPCTSTR pszName = NULL ) const
	{
		return( CGFile::GetMergedFileName( m_sExePath, pszName ));
	}
	CGString GetFullCDPath( LPCTSTR pszName = NULL ) const
	{
		return( CGFile::GetMergedFileName( m_sCDPath, pszName ));
	}

public:
	bool FindInstall();
	VERFILE_TYPE OpenFiles( DWORD dwMask );
	bool OpenFile( CGFile & file, LPCTSTR pszName, WORD wFlags );
	bool OpenFile( VERFILE_TYPE i );
	void CloseFiles();

	static LPCTSTR GetBaseFileName( VERFILE_TYPE i );
	CGFile * GetMulFile( VERFILE_TYPE i )
	{
		return &(m_File[i]);
	}

	void SetPreferPath( LPCTSTR pszName )
	{
		m_sPreferPath = pszName;
	}
	CGString GetPreferPath( LPCTSTR pszName = NULL ) const
	{
		return CGFile::GetMergedFileName(m_sPreferPath, pszName);
	}

	bool ReadMulIndex( VERFILE_TYPE fileindex, VERFILE_TYPE filedata, DWORD id, CUOIndexRec & Index );
	bool ReadMulData( VERFILE_TYPE filedata, const CUOIndexRec & Index, void * pData );

	bool ReadMulIndex(CGFile &file, DWORD id, CUOIndexRec &Index);
	bool ReadMulData(CGFile &file, const CUOIndexRec &Index, void * pData);

} g_Install;

#endif	// _INC_CGRAYINST_H