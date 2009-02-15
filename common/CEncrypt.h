//
// CEncrypt.h
//

#ifndef _INC_CENCRYPT_H
#define _INC_CENCRYPT_H
#pragma once

#include "CScript.h"

// --- Two Fish ---
#include "twofish/aes.h"

// --- Blow Fish ---
// #include "blowfish/blowfish.h"

// --- MD5 ----
#include "CMD5.h"

#define CLIENT_END 0x00000001

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
	ENC_BFISH,
	ENC_BTFISH,
	ENC_TFISH,
	ENC_QTY,
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

// HuffMan Compression Class

#define COMPRESS_TREE_SIZE (256+1)

class CHuffman
{
private:
	static const WORD sm_xCompress_Base[COMPRESS_TREE_SIZE];
public:
	static const char *m_sClassName;
	
	static int Compress( BYTE * pOutput, const BYTE * pInput, int inplen );
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

struct CCryptClientKey
{
	DWORD m_client;
	DWORD m_key_1;
	DWORD m_key_2;
	ENCRYPTION_TYPE m_EncType;
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
	bool m_fRelayPacket;
	int m_iClientVersion;
	ENCRYPTION_TYPE m_GameEnc;

protected:
	DWORD m_MasterHi;
	DWORD m_MasterLo;

	DWORD m_CryptMaskHi;
	DWORD m_CryptMaskLo;

	DWORD m_seed;	// seed ip we got from the client.
	
	CONNECT_TYPE m_ConnectType;
	
//	#define TOTAL_CLIENTS 33
//	static const DWORD client_keys[TOTAL_CLIENTS+2][4];

	static const WORD packet_size[0xde];

public:
	static void LoadKeyTable(CScript & s);
	static std::vector<CCryptClientKey *> client_keys;
	static void addNoCryptKey(void);

	// --------------- Generic -----------------------------------
	static int GetPacketSize(BYTE packet);
	// --------------- EOF Generic -------------------------------

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
	// -------------- Blow Fish ------------------------------
	#define CRYPT_GAMEKEY_COUNT	25 // CRYPT_MAX_SEQ
	#define CRYPT_GAMEKEY_LENGTH	6
	
	#define CRYPT_GAMETABLE_START	1
	#define CRYPT_GAMETABLE_STEP	3
	#define CRYPT_GAMETABLE_MODULO	11
	#define CRYPT_GAMETABLE_TRIGGER	21036
	static const BYTE sm_key_table[CRYPT_GAMEKEY_COUNT][CRYPT_GAMEKEY_LENGTH];
	static const BYTE sm_seed_table[2][CRYPT_GAMEKEY_COUNT][2][CRYPT_GAMESEED_LENGTH];
	static bool  sm_fTablesReady;
public:
	int	m_gameTable;
	int	m_gameBlockPos;		// 0-7
	int	m_gameStreamPos;	// use this to track the 21K move to the new Blowfish m_gameTable.
private:
	CCrypt::CCryptKey m_Key;
private:
	void InitSeed( int iTable );
	static void InitTables();
	static void PrepareKey( CCrypt::CCryptKey & key, int iTable );
	void DecryptBlowFish( BYTE * pOutput, const BYTE * pInput, int iLen );
	BYTE DecryptBFByte( BYTE bEnc );
	void InitBlowFish();
	// -------------- EOF BlowFish -----------------------

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
	// ------------- Old Encryption ----------------------
	void DecryptOld( BYTE * pOutput, const BYTE * pInput, int iLen  );
	// ------------- EOF Old Encryption ------------------

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
	bool SetClientVer( LPCTSTR pszVersion );
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
	bool Init( DWORD dwIP, BYTE * pEvent, int iLen, bool isclientKr = false );
	void InitFast( DWORD dwIP, CONNECT_TYPE ctInit, bool fRelay = true );
	void Decrypt( BYTE * pOutput, const BYTE * pInput, int iLen );
	void Encrypt( BYTE * pOutput, const BYTE * pInput, int iLen );
protected:
	void LoginCryptStart( DWORD dwIP, BYTE * pEvent, int iLen );
	void GameCryptStart( DWORD dwIP, BYTE * pEvent, int iLen );
	void RelayGameCryptStart( BYTE * pOutput, const BYTE * pInput, int iLen );
};

#endif
