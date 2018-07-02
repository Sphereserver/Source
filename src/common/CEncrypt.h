#ifndef _INC_CENCRYPT_H
#define _INC_CENCRYPT_H
#pragma once

#include "twofish/aes.h"
#include "CScript.h"
#include "CMD5.h"

enum CONNECT_TYPE
{
	CONNECT_NONE,		// No connection
	CONNECT_UNK,		// Client has just connected. Waiting for first message
	CONNECT_CRYPT,		// Login or game protocol, but server don't know yet
	CONNECT_LOGIN,		// Login client protocol
	CONNECT_GAME,		// Game client protocol
	CONNECT_HTTP,		// Webpage connection
	CONNECT_TELNET,		// Telnet connection
	CONNECT_UOG,		// UOGateway connection
	CONNECT_AXIS,		// Axis connection
	CONNECT_QTY
};

enum ENCRYPTION_TYPE
{
	ENC_NONE,
	ENC_BFISH,
	ENC_BTFISH,
	ENC_TFISH,
	ENC_QTY
};

struct CCryptClientKey
{
	DWORD m_Version;
	DWORD m_Key1;
	DWORD m_Key2;
	ENCRYPTION_TYPE m_EncryptionType;
};

struct CCrypt
{
public:
	CCrypt();

private:
	bool m_fInit;
	bool m_fRelayPacket;
	DWORD m_dwClientVersion;
	ENCRYPTION_TYPE m_EncryptionType;

protected:
	CONNECT_TYPE m_ConnectType;
	DWORD m_MasterLo;
	DWORD m_MasterHi;
	DWORD m_CryptMaskLo;
	DWORD m_CryptMaskHi;
	DWORD m_Seed;	// seed IP received from client

public:
	bool Init(DWORD dwIP, BYTE *pbEvent, size_t iLen, bool fClientKR = false);
	void InitFast(DWORD dwIP, CONNECT_TYPE type, bool fRelay = true);
	void Encrypt(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);
	void Decrypt(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);
	void DecryptOld(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);

protected:
	void LoginCryptStart(DWORD dwIP, BYTE *pbEvent, size_t iLen);
	void GameCryptStart(DWORD dwIP, BYTE *pbEvent, size_t iLen);
	void RelayGameCryptStart(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);

public:
	#define CRYPT_GAMESEED_LENGTH 8
	union CCryptKey
	{
		BYTE bKey[CRYPT_GAMESEED_LENGTH];
		DWORD dwKey[2];
	};

public:
	void SetClientVer(const CCrypt &crypt);
	bool SetClientVer(LPCTSTR pszVer);
	bool SetClientVerEnum(DWORD dwVer, bool fSetEncrypt = true);
	bool SetClientVerIndex(size_t index, bool fSetEncrypt = true);
	static DWORD GetVerFromString(LPCTSTR pszVer);
	static DWORD GetVerFromNumber(DWORD dwMajor, DWORD dwMinor, DWORD dwRevision, DWORD dwPatch);
	static TCHAR *WriteClientVerString(DWORD dwVer, TCHAR *pszOutput);

private:
	void SetClientVersion(DWORD dwVer)
	{
		m_dwClientVersion = dwVer;
	}

	bool SetConnectType(CONNECT_TYPE type)
	{
		if ( (type > CONNECT_NONE) && (type < CONNECT_QTY) )
		{
			m_ConnectType = type;
			return true;
		}
		return false;
	}

	bool SetEncryptionType(ENCRYPTION_TYPE type)
	{
		if ( (type >= ENC_NONE) && (type < ENC_QTY) )
		{
			m_EncryptionType = type;
			return true;
		}
		return false;
	}

	void SetMasterKeys(DWORD hi, DWORD low)
	{
		m_MasterHi = hi;
		m_MasterLo = low;
	}

	void SetCryptMask(DWORD hi, DWORD low)
	{
		m_CryptMaskHi = hi;
		m_CryptMaskLo = low;
	}

public:
	bool IsInit() const
	{
		return m_fInit;
	}

	DWORD GetClientVer() const
	{
		return m_dwClientVersion;
	}

	CONNECT_TYPE GetConnectType() const
	{
		return m_ConnectType;
	}

	ENCRYPTION_TYPE GetEncryptionType() const
	{
		return m_EncryptionType;
	}

// Client encryption keys table
public:
	static std::vector<CCryptClientKey *> sm_ClientCryptKeys;
	static void addNoCryptKey();
	static void LoadKeyTable(CScript &s);
	static void ClearKeyTable();

// Blowfish
protected:
	#define CRYPT_GAMEKEY_COUNT		25	// CRYPT_MAX_SEQ
	#define CRYPT_GAMEKEY_LENGTH	6
	#define CRYPT_GAMETABLE_START	1
	#define CRYPT_GAMETABLE_STEP	3
	#define CRYPT_GAMETABLE_MODULO	11
	#define CRYPT_GAMETABLE_TRIGGER	21036
	static const BYTE sm_KeyTable[CRYPT_GAMEKEY_COUNT][CRYPT_GAMEKEY_LENGTH];
	static const BYTE sm_SeedTable[2][CRYPT_GAMEKEY_COUNT][2][CRYPT_GAMESEED_LENGTH];
	static bool sm_fTablesReady;

private:
	CCrypt::CCryptKey m_Key;
	size_t m_gameTable;
	size_t m_gameBlockPos;		// 0-7
	size_t m_gameStreamPos;		// use this to track the 21K move to the new blowfish m_gameTable

private:
	static void InitTables();
	static void PrepareKey(CCrypt::CCryptKey &key, size_t iTable);
	void InitBlowfish();
	void InitSeed(size_t iTable);
	void DecryptBlowfish(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);
	BYTE DecryptBlowfishByte(BYTE bEnc);

// Twofish
protected:
	#define TFISH_RESET 0x100
	keyInstance tf_key;
	cipherInstance tf_cipher;
	int tf_position;
	BYTE tf_cipherTable[TFISH_RESET];

private:
	void InitTwofish();
	void DecryptTwofish(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);

// MD5
protected:
	#define MD5_RESET 0xF
	size_t md5_position;
	CMD5 md5_engine;
	BYTE md5_digest[16];

protected:
	void InitMD5(BYTE *pbInitialize);
	void EncryptMD5(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);

private:
	CCrypt(const CCrypt &copy);
	CCrypt &operator=(const CCrypt &other);
};

///////////////////////////////////////////////////////////
// CHuffman

class CHuffman
{
public:
	CHuffman() { };

	static const char *m_sClassName;

private:
	#define COMPRESS_TREE_SIZE (256 + 1)
	static const WORD sm_xCompress_Base[COMPRESS_TREE_SIZE];

public:
	static size_t Compress(BYTE *pbOutput, const BYTE *pbInput, size_t iLen);

private:
	CHuffman(const CHuffman &copy);
	CHuffman &operator=(const CHuffman &other);
};

#endif
