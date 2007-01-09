//
// CEncrypt.h
//

#ifndef _INC_CENCRYPT_H
#define _INC_CENCRYPT_H
#pragma once

#include <vector>

#include "CScript.h"

// --- Two Fish ---
#include "twofish/aes.h"

// --- MD5 ----
#include "CMD5.h"

enum CONNECT_TYPE	// What type of client connection is this ?
{
	CONNECT_NONE,		// There is no connection.
	CONNECT_UNK,		// client has just connected. waiting for first message.
	CONNECT_CRYPT,		// It's a game or login protocol but i don't know which yet.
	CONNECT_LOGIN,		// login client protocol
	CONNECT_GAME,		// game client protocol
	CONNECT_HTTP,		// We are serving web pages to this.
	CONNECT_TELNET,		// we are in telnet console mode.
	CONNECT_QTY,
};

enum ENCRYPTION_TYPE
{
	ENC_NONE=0,
	ENC_TFISH,
	ENC_QTY,
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

// HuffMan Compression Class

class CCompressBranch
{
	// For compressing/decompressing stuff from game server to client.
	friend class CCompressTree;
public:
	static const char *m_sClassName;
private:
	int m_Value; // -1=tree branch, 0-255=character value, 256=PROXY_BRANCH_QTY-1=end
	CCompressBranch *m_pZero;
	CCompressBranch *m_pOne;
private:
	CCompressBranch()
	{
		m_Value = -1;	// just a pass thru branch til later.
		m_pZero = NULL;
		m_pOne = NULL;
	}
	~CCompressBranch()
	{
		delete m_pOne;
		delete m_pZero;
	}
	bool IsLoaded() const
	{
		return (m_pZero != NULL);
	}
};

#define COMPRESS_TREE_SIZE (256+1)

class CCompressTree
{
private:
	CCompressBranch m_Root;
	const CCompressBranch * m_pDecodeCur;
private:
	static const WORD sm_xCompress_Base[COMPRESS_TREE_SIZE];
private:
	bool AddBranch(int Value, WORD wCode, int iBits );
public:
	static const char *m_sClassName;
	void Reset()
	{
		m_pDecodeCur = &m_Root;
	}
	bool IsLoaded() const
	{
		return( m_Root.IsLoaded() );
	}
	
	static int Encode( BYTE * pOutput, const BYTE * pInput, int inplen );
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

struct CCryptClientKey
{
	DWORD m_client;
	DWORD m_key_1;
	DWORD m_key_2;
	ENCRYPTION_TYPE m_EncType;

	CCryptClientKey(DWORD version, DWORD key1, DWORD key2, ENCRYPTION_TYPE encType)
	{
		m_client = version;
		m_key_1 = key1;
		m_key_2 = key2;
		m_EncType = encType;
	}
};

struct CCrypt
{
public:
	union CCryptKey
	{
		#define CRYPT_GAMESEED_LENGTH	8
		BYTE  u_cKey[CRYPT_GAMESEED_LENGTH];
		DWORD u_iKey[2];
	};
private:
	bool m_fInit;
	int m_iClientVersion;
	ENCRYPTION_TYPE m_GameEnc;

protected:
	DWORD m_MasterHi;
	DWORD m_MasterLo;

	DWORD m_CryptMaskHi;
	DWORD m_CryptMaskLo;

	DWORD m_seed;	// seed IP we got from the client.
	
	CONNECT_TYPE m_ConnectType;
	
public:
	static std::vector<CCryptClientKey *> client_keys;

protected:
	// --------------- Two Fish ------------------------------
	#define TFISH_RESET 0x100
	keyInstance tf_key;
	cipherInstance tf_cipher;
	BYTE tf_cipherTable[TFISH_RESET];
	int tf_position;
private:
	void InitTwoFish();
	void DecryptTwoFish( BYTE * pOutput, const BYTE * pInput, int iLen );
	// --------------- EOF TwoFish ----------------------------

protected:
	// -------------------- MD5 ------------------------------
	#define MD5_RESET 0x0F
	CMD5 md5_engine;
	unsigned int md5_position;
	BYTE md5_digest[16];
protected:
	void EncryptMD5( BYTE * pOutput, const BYTE * pInput, int iLen );
	void InitMD5(BYTE * ucInitialize);
	// ------------------ EOF MD5 ----------------------------

private:
	int GetVersionFromString( LPCTSTR pszVersion );

private:
	void SetClientVersion( int iVer )
	{
		m_iClientVersion = iVer;
	}
	
	void SetMasterKeys( DWORD hi, DWORD low )
	{
		m_MasterHi = hi;
		m_MasterLo = low;
	}
	
	void SetCryptMask( DWORD hi, DWORD low )
	{
		m_CryptMaskHi = hi;
		m_CryptMaskLo= low;
	}
	
	bool SetConnectType( CONNECT_TYPE ctWho )
	{
		if ( ctWho > CONNECT_NONE && ctWho < CONNECT_QTY )
		{
			m_ConnectType = ctWho;
			return true;
		}
		
		return false;
	}
	
	bool SetEncryptionType( ENCRYPTION_TYPE etWho )
	{
		if ( etWho >= ENC_NONE && etWho < ENC_QTY )
		{
			m_GameEnc = etWho;
			return true;
		}
		
		return false;
	}

public:
	TCHAR* WriteClientVer( TCHAR * pStr ) const;
	bool SetClientVerEnum( int iVer, bool bSetEncrypt = true );
	bool SetClientVerIndex( int iVer, bool bSetEncrypt = true );
	void SetClientVer( const CCrypt & crypt );
	void SetClientVer( LPCTSTR pszVersion );
	static int GetVerFromString( LPCTSTR pszVersion );
	static TCHAR* WriteClientVerString( int iClientVersion, TCHAR * pStr );

public:
	int GetClientVer() const
	{
		return( m_iClientVersion );
	}
	
	bool IsValid() const
	{
		return( m_iClientVersion >= 0 );
	}

	bool IsInit() const
	{
		return( m_fInit );
	}
		
	CONNECT_TYPE GetConnectType() const
	{
		return( m_ConnectType );
	}
	
	ENCRYPTION_TYPE GetEncryptionType() const
	{
		return( m_GameEnc );
	}

// --------- Basic
public:
	CCrypt();
	bool Init( DWORD dwIP, BYTE * pEvent, int iLen );
	void InitFast( DWORD dwIP, CONNECT_TYPE ctInit );
	void Decrypt( BYTE * pOutput, const BYTE * pInput, int iLen );
	void Encrypt( BYTE * pOutput, const BYTE * pInput, int iLen );
protected:
	void LoginCryptStart( DWORD dwIP, BYTE * pEvent, int iLen );
	void GameCryptStart( DWORD dwIP, BYTE * pEvent, int iLen );
	
};

#endif