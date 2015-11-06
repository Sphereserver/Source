#ifndef _INC_CGRAYINST_H
#define _INC_CGRAYINST_H
#pragma once

#include "../common/graymul.h"
#include "../common/CFile.h"
#include "../common/CArray.h"
#include "../common/CsvFile.h"

////////////////////////////////////////////////////////

class MapAddress
{
public:
	unsigned long dwFirstBlock;
	unsigned long dwLastBlock;
	INT64 qwAdress;
};

unsigned long long HashFileName(CGString csFile);

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
	VERFILE_FORMAT m_FileFormat[ VERFILE_QTY ]; // File format versions

	CGFile	m_Maps[256];		// mapX.mul
	CGFile	m_Mapdif[256];		// mapdifX.mul
	CGFile	m_Mapdifl[256];		// mapdiflX.mul
	CGFile	m_Statics[256];		// staticsX.mul
	CGFile	m_Staidx[256];		// staidxX.mul
	CGFile	m_Stadif[256];		// stadifX.mul
	CGFile	m_Stadifi[256];		// stadifiX.mul
	CGFile	m_Stadifl[256];		// stadiflX.mul
	bool m_IsMapUopFormat[256]; // true for maps that are uop format
	MapAddress m_UopMapAddress[256][256]; //For uop parsing. Note: might need to be ajusted later if format changes.

	CSVFile m_CsvFiles[8];		// doors.txt, stairs.txt (x2), roof.txt, misc.txt, teleprts.txt, floors.txt, walls.txt

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
	void DetectMulVersions();
	VERFILE_TYPE OpenFiles( DWORD dwMask );
	bool OpenFile( CGFile & file, LPCTSTR pszName, WORD wFlags );
	bool OpenFile( VERFILE_TYPE i );
	void CloseFiles();

	static LPCTSTR GetBaseFileName( VERFILE_TYPE i );
	CGFile * GetMulFile( VERFILE_TYPE i )
	{
		ASSERT( i<VERFILE_QTY );
		return( &(m_File[i]));
	}
	VERFILE_FORMAT GetMulFormat( VERFILE_TYPE i )
	{
		ASSERT( i<VERFILE_QTY );
		return( m_FileFormat[i] );
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
	
public:
	CGrayInstall()
	{
		memset(m_FileFormat, 0, sizeof(m_FileFormat));
		memset(m_IsMapUopFormat, 0, sizeof(m_IsMapUopFormat));
		memset(m_UopMapAddress, 0, sizeof(m_UopMapAddress));
	};

private:
	CGrayInstall(const CGrayInstall& copy);
	CGrayInstall& operator=(const CGrayInstall& other);
} g_Install;

///////////////////////////////////////////////////////////////////////////////

extern class CVerDataMul
{
	// Find verison diffs to the files listed.
	// Assume this is a sorted array of some sort.
public:
	static const char *m_sClassName;
	CGTypedArray < CUOVersionBlock, CUOVersionBlock& > m_Data;
private:
	int QCompare( size_t left, DWORD dwRefIndex ) const;
	void QSort( size_t left, size_t right );
public:
	size_t GetCount() const
	{
		return( m_Data.GetCount());
	}
	const CUOVersionBlock * GetEntry( size_t i ) const
	{
		return( &m_Data.ElementAt(i));
	}
	void Unload()
	{
		m_Data.Empty();
	}
	void Load( CGFile & file );
	bool FindVerDataBlock( VERFILE_TYPE type, DWORD id, CUOIndexRec & Index ) const;
	
public:
	CVerDataMul() { };
	~CVerDataMul()
	{
		Unload();
	}
private:
	CVerDataMul(const CVerDataMul& copy);
	CVerDataMul& operator=(const CVerDataMul& other);
} g_VerData;

#endif	// _INC_CGRAYINST_H
