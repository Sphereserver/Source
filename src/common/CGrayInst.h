#ifndef _INC_CGRAYINST_H
#define _INC_CGRAYINST_H
#pragma once

#include "../common/graymul.h"
#include "../common/CCSVFile.h"

class MapAddress
{
public:
	DWORD dwFirstBlock;
	DWORD dwLastBlock;
	INT64 qwAddress;
};

///////////////////////////////////////////////////////////
// CGrayInstall

extern struct CGrayInstall
{
public:
	CGrayInstall()
	{
		memset(m_FileFormat, 0, sizeof(m_FileFormat));
		m_IsMultiUopFormat = false;
		memset(m_IsMapUopFormat, 0, sizeof(m_IsMapUopFormat));
		memset(m_UopMapAddress, 0, sizeof(m_UopMapAddress));
	};

public:
	VERFILE_FORMAT m_FileFormat[VERFILE_QTY];	// File format versions
	bool m_IsMultiUopFormat;					// True when multi file is MultiCollection.uop instead multi.mul (STILL INCOMPLETE)
	bool m_IsMapUopFormat[256];					// True when map file is map[x]LegacyMUL.uop instead map[x].mul
	MapAddress m_UopMapAddress[256][256];		// For UOP parsing (note: might need to be ajusted later if format changes)

	CGFile m_File[VERFILE_QTY];	// List of files to access
	CGFile m_Maps[256];			// mapX.mul
	CGFile m_Mapdif[256];		// mapdifX.mul
	CGFile m_Mapdifl[256];		// mapdiflX.mul
	CGFile m_Statics[256];		// staticsX.mul
	CGFile m_Staidx[256];		// staidxX.mul
	CGFile m_Stadif[256];		// stadifX.mul
	CGFile m_Stadifi[256];		// stadifiX.mul
	CGFile m_Stadifl[256];		// stadiflX.mul
	CCSVFile m_CsvFiles[8];		// doors.txt, stairs.txt, roof.txt, misc.txt, teleprts.txt, floors.txt, walls.txt

private:
	CGString m_sPreferPath;		// Prefer path in which to choose the files (look here first)
	CGString m_sExePath;		// Files that are installed ("ExePath")
	CGString m_sCDPath;			// For files that may still be on the CD ("InstCDPath")

public:
	bool FindInstall();
	static LPCTSTR GetBaseFileName(VERFILE_TYPE i);

	bool OpenFile(CGFile &file, LPCTSTR pszFileName, WORD wFlags);
	bool OpenFile(VERFILE_TYPE i);
	VERFILE_TYPE OpenFiles(DWORD dwMask);
	void CloseFiles();

	bool ReadMulIndex(CGFile &file, DWORD id, CUOIndexRec &index);
	bool ReadMulData(CGFile &file, const CUOIndexRec &index, void *pData);

	bool ReadMulIndex(VERFILE_TYPE fileindex, VERFILE_TYPE filedata, DWORD id, CUOIndexRec &index);
	bool ReadMulData(VERFILE_TYPE filedata, const CUOIndexRec &index, void *pData);
	
	VERFILE_FORMAT GetMulFormat(VERFILE_TYPE i)
	{
		ASSERT(i < VERFILE_QTY);
		return m_FileFormat[i];
	}

	CGFile *GetMulFile(VERFILE_TYPE i)
	{
		ASSERT(i < VERFILE_QTY);
		return &m_File[i];
	}

	void SetPreferPath(LPCTSTR pszFileName)
	{
		m_sPreferPath = pszFileName;
	}
	CGString GetPreferPath(LPCTSTR pszFileName = NULL) const
	{
		return CGFile::GetMergedFileName(m_sPreferPath, pszFileName);
	}
	CGString GetFullExePath(LPCTSTR pszFileName = NULL) const
	{
		return CGFile::GetMergedFileName(m_sExePath, pszFileName);
	}
	CGString GetFullCDPath(LPCTSTR pszFileName = NULL) const
	{
		return CGFile::GetMergedFileName(m_sCDPath, pszFileName);
	}
	CGString GetMulFilesPath() const
	{
		CGString sPath = GetPreferPath();
		if ( !sPath.IsEmpty() )
			return sPath;

		sPath = GetFullExePath();
		if ( !sPath.IsEmpty() )
			return sPath;

		sPath = GetFullCDPath();
		if ( !sPath.IsEmpty() )
			return sPath;

		return NULL;
	}

private:
	void DetectMulVersions();
	UINT64 HashFileName(CGString sFile);


private:
	CGrayInstall(const CGrayInstall &copy);
	CGrayInstall &operator=(const CGrayInstall &other);
} g_Install;

///////////////////////////////////////////////////////////
// CVerDataMul

extern class CVerDataMul
{
	// Find verison diffs to the files listed
	// Assume this is a sorted array of some sort
public:
	static const char *m_sClassName;

	CVerDataMul() { };
	~CVerDataMul()
	{
		Unload();
	}

public:
	CGTypedArray<CUOVersionBlock, CUOVersionBlock &> m_Data;

private:
	int QCompare(size_t left, DWORD dwRefIndex) const;
	void QSort(size_t left, size_t right);

public:
	void Load(CGFile &file);
	void Unload()
	{
		m_Data.Empty();
	}

	bool FindVerDataBlock(VERFILE_TYPE type, DWORD id, CUOIndexRec &index) const;

	size_t GetCount() const
	{
		return m_Data.GetCount();
	}

	const CUOVersionBlock *GetEntry(size_t i) const
	{
		return &m_Data.ElementAt(i);
	}

private:
	CVerDataMul(const CVerDataMul &copy);
	CVerDataMul &operator=(const CVerDataMul &other);
} g_VerData;

#endif	// _INC_CGRAYINST_H
