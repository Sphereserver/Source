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
	VERFILE_FORMAT m_FileFormat[VERFILE_QTY];		// File format versions
	bool m_IsMultiUopFormat;						// True when multi file is MultiCollection.uop instead multi.mul (STILL INCOMPLETE)
	bool m_IsMapUopFormat[MAP_QTY];					// True when map file is map[x]LegacyMUL.uop instead map[x].mul
	MapAddress m_UopMapAddress[MAP_QTY][MAP_QTY];	// For UOP parsing (note: might need to be ajusted later if format changes)

	CGFile m_File[VERFILE_QTY];		// List of files to access
	CGFile m_Maps[MAP_QTY];			// mapX.mul
	CGFile m_Mapdif[MAP_QTY];		// mapdifX.mul
	CGFile m_Mapdifl[MAP_QTY];		// mapdiflX.mul
	CGFile m_Statics[MAP_QTY];		// staticsX.mul
	CGFile m_Staidx[MAP_QTY];		// staidxX.mul
	CGFile m_Stadif[MAP_QTY];		// stadifX.mul
	CGFile m_Stadifi[MAP_QTY];		// stadifiX.mul
	CGFile m_Stadifl[MAP_QTY];		// stadiflX.mul
	CCSVFile m_CsvFiles[8];			// doors.txt, stairs.txt, roof.txt, misc.txt, teleprts.txt, floors.txt, walls.txt

private:
	CGString m_sMulPath;			// Path to mul files

public:
#ifdef _WIN32
	void FindInstall();
#endif
	static LPCTSTR GetBaseFileName(VERFILE_TYPE i);

	bool OpenFile(CGFile &file, LPCTSTR pszFileName, UINT uFlags);
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

	void SetMulPath(LPCTSTR pszFileName)
	{
		m_sMulPath = pszFileName;
	}
	CGString GetMulPath(LPCTSTR pszFileName = NULL) const
	{
		return CGFile::GetMergedFileName(m_sMulPath, pszFileName);
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
	void QSort(size_t left, size_t right);

public:
	void Load(CGFile &file);
	void Unload()
	{
		m_Data.RemoveAll();
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
