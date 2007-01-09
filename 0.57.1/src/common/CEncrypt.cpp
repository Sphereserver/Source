//
// CEncrypt.cpp
//

// #include "../graysvr/graysvr.h"
#include "graycom.h"
#include "graymul.h"
#include "grayproto.h"

//#define DEBUG_CRYPT_MSGS 1

std::vector<CCryptClientKey *> CCrypt::client_keys;

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

int CCrypt::GetVerFromString( LPCTSTR pszVersion )
{
	// They have this annoying habit of putting letters at the end of the version string.
	// 2.0.0d for example.
	// Since 5.0.6.5 we also have a new formatting for client versions to consider.
	
	if ( pszVersion == NULL || *pszVersion == '\0' )
		return false;

	int iVer = 0;
	int iPoint = 0;
	int iDigPoint = 0;

	for ( int i=0; true; i++ )
	{
		TCHAR ch = pszVersion[i];
		if ( iPoint < 3 )
		{
			if ( isdigit(ch))
			{
				iVer *= 0x10;
				iVer += ( ch - '0' );
				iDigPoint ++;
				continue;
			}

			if ( iPoint )
			{
				if ( iDigPoint == 0 )
				{
					iVer *= 0x100;
				}
				else if ( iDigPoint == 1 )
				{
					int iTmp = iVer & 0x0f;
					iVer &= ~0x0f;
					iVer *= 0x10;
					iVer += iTmp;
				}
			}
			else
			{
				if ( i == 0 && tolower(ch) == 'i' )
				{
					continue;
				}
			}
		}

		if ( ch == '.' )
		{
			if ( ++iPoint > 3 )
			{
				iVer *= 0x10;
				break;
			}
			iDigPoint = 0;
			continue;
		}
	
		if ( ! iPoint && ! isalpha(ch) && ! isdigit(ch) )
		{
			iVer *= 0x10;
			break;
		}

		while ( iPoint< 2 )
		{
			iVer *= 0x100;
			iPoint++;
		}
	
		// last char digit slot.
		iVer *= 0x10;
		if ( isalpha(ch))
		{
			iVer += ( tolower(ch) - 'a' ) + 1;
		}
		else if ( isdigit(ch) )
		{
			iVer += ( ch - '0' ) + 1;
		}
		break;
	}
	
	return iVer;
}

TCHAR* CCrypt::WriteClientVerString( int iClientVersion, TCHAR * pStr )
{
	int iLen = sprintf( pStr, "%x.%x.%x", iClientVersion/0x100000, (iClientVersion/0x1000)%0x100, (iClientVersion/0x10)%0x100 );
	if ( iClientVersion & 0x0f )
	{
		if ( iClientVersion >= 0x500066 )
		{
			pStr[iLen++] = '.';
			pStr[iLen++] = ( iClientVersion & 0x0f ) + '0' - 1;
		}
		else
		{
			pStr[iLen++] = ( iClientVersion & 0x0f ) + 'a' - 1;
		}

		pStr[iLen] = '\0';
	}
	return( pStr );
}

int CCrypt::GetVersionFromString( LPCTSTR pszVersion )
{
	return CCrypt::GetVerFromString( pszVersion );
}

TCHAR* CCrypt::WriteClientVer( TCHAR * pStr ) const
{
	return CCrypt::WriteClientVerString(GetClientVer(), pStr);
}

bool CCrypt::SetClientVerEnum( int iVer, bool bSetEncrypt )
{
	int i = 0;
	for ( ; i < client_keys.size(); i++ )
	{
		CCryptClientKey * key = client_keys.at(i);

		if ( iVer == key->m_client )
		{
			if ( SetClientVerIndex( i, bSetEncrypt ))
				return true;
		}
	}

	return false;
}

bool CCrypt::SetClientVerIndex( int iVer, bool bSetEncrypt )
{
	if ( ( iVer < 0 ) || ( iVer >= client_keys.size() ) )
		return false;

	CCryptClientKey * key = client_keys.at(iVer);

	SetClientVersion(key->m_client);
	SetMasterKeys(key->m_key_1, key->m_key_2); // Hi - Lo
	if ( bSetEncrypt )
		SetEncryptionType(key->m_EncType);

	return true;
}

void CCrypt::SetClientVer( const CCrypt & crypt )
{
	m_fInit = false;
	m_iClientVersion = crypt.m_iClientVersion;
	m_MasterHi = crypt.m_MasterHi;
	m_MasterLo = crypt.m_MasterLo;
}

void CCrypt::SetClientVer(LPCTSTR pszVersion)
{
	int iVer = GetVersionFromString(pszVersion);

	m_fInit = false;

	SetClientVerEnum(iVer);
}

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

CCrypt::CCrypt()
{
	m_fInit = false;
	SetClientVerEnum(0);
}

bool CCrypt::Init( DWORD dwIP, BYTE * pEvent, int iLen )
{
	bool bReturn = true;
	// fprintf( stderr, "Called Init\n" );
	
	if ( iLen == 62 ) // SERVER_Login 1.26.0
	{
		LoginCryptStart( dwIP, pEvent, iLen );
	}
	else if ( iLen == 65 )	// Auto-registering server sending us info.
	{
		GameCryptStart( dwIP, pEvent, iLen );
	}
	else
	{
#ifdef DEBUG_CRYPT_MSGS
		DEBUG_MSG(("Odd login message length %d? [CCrypt::Init()]\n", iLen));
#endif
		// LoginCryptStart( dwIP, pEvent, iLen );
		bReturn = false;
	}
	
	return bReturn;
}

void CCrypt::InitFast( DWORD dwIP, CONNECT_TYPE ctInit )
{
	// fprintf( stderr, "Called FastInit\n" );
	SetConnectType( ctInit );
	m_seed = dwIP;
	
	if ( ctInit == CONNECT_GAME )
	{
		InitTwoFish();
	}
}

void CCrypt::LoginCryptStart( DWORD dwIP, BYTE * pEvent, int iLen )
{
	BYTE m_Raw[ MAX_BUFFER ];
	bool bFirstRun = false;
	memcpy( m_Raw, pEvent, iLen );
	
	m_seed = dwIP;
	SetConnectType( CONNECT_LOGIN );
	
	DWORD m_tmp_CryptMaskLo = (((~m_seed) ^ 0x00001357) << 16) | ((( m_seed) ^ 0xffffaaaa) & 0x0000ffff);
	DWORD m_tmp_CryptMaskHi = ((( m_seed) ^ 0x43210000) >> 16) | (((~m_seed) ^ 0xabcdffff) & 0xffff0000);
	
	SetClientVerIndex(0);
	SetCryptMask(m_tmp_CryptMaskHi, m_tmp_CryptMaskLo);
#define GONEXTSTEP {bFirstRun=true; goto nextone;}
	
	int i = 0;
	while ( true )
	{
		if ( i >= client_keys.size() )
		{
			// Unknown client !!! Set as unencrypted and let Sphere do the rest.
#ifdef DEBUG_CRYPT_MSGS
			DEBUG_ERR(("Unknown client,i = %d", i ));
#endif
			SetClientVerIndex(0);
			SetCryptMask(m_tmp_CryptMaskHi, m_tmp_CryptMaskLo); // Hi - Lo
			break;
		}
		
		SetCryptMask(m_tmp_CryptMaskHi, m_tmp_CryptMaskLo);
		// Set client version properties
		SetClientVerIndex(i);
		
		// Test Decrypt
		Decrypt( m_Raw, pEvent, iLen );

#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
		fprintf(stderr, "LoginCrypt %d (%x) type %x-%x\n", i, GetClientVer(), m_Raw[0], pEvent[0]);
#else
		DEBUG_ERR(("LoginCrypt %d (%x) type %x-%x\n", i, GetClientVer(), m_Raw[0], pEvent[0]));
#endif
#endif
		
		if ( m_Raw[0] == 0x80 && m_Raw[30] == 0x00 && m_Raw[60] == 0x00 )
		{
			// -----------------------------------------------------
			// This is a sanity check, sometimes client keys (like 4.0.0) can intercept, incorrectly,
			// a login packet. When is decrypted it is done not correctly (strange chars after
			// regular account name/password). This prevents that fact, choosing the right keys
			// to decrypt it correctly :)
			
			if ( !bFirstRun )
			{
				int x = 0;
				int bOk = 0;
				for ( x = 1; x <= 30; x++)
				{
#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
					fprintf(stderr, "0x%x ", m_Raw[x]);
#else
					DEBUG_ERR(( "0x%x ", m_Raw[x] ));
#endif
#endif
					if ( m_Raw[x] == 0x00 )
					{
						bOk = 1;
					}
					else
					{
						if ( bOk && x != 30 )
						{
							bOk = 2;
							break;
						}
					}
				}
#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
					fprintf(stderr, "\n");
#else
					DEBUG_ERR(( "\n" ));
#endif
#endif
				if ( bOk == 2 )
					GONEXTSTEP;
				
				bOk = 0;
				for ( x = 31; x <= 60; x++ )
				{
#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
					fprintf(stderr, "0x%x ", m_Raw[x]);
#else
					DEBUG_ERR(( "0x%x ", m_Raw[x] ));
#endif
#endif
					if ( m_Raw[x] == 0x00 )
					{
						bOk = 1;
					}
					else
					{
						if ( bOk && x != 60 )
						{
							bOk = 2;
							break;
						}
					}
				}
#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
					fprintf(stderr, "\n");
#else
					DEBUG_ERR((  "\n" ));
#endif
#endif
				if ( bOk == 2 )
					GONEXTSTEP;
			// -----------------------------------------------------
			}
			
			// set seed, clientversion, cryptmask
			SetClientVerIndex(i);
			SetCryptMask(m_tmp_CryptMaskHi, m_tmp_CryptMaskLo);
			break;
		}

nextone:
		// Next one
		i++;
	}
	
#undef GONEXTSTEP
	m_fInit = true;
}

void CCrypt::GameCryptStart( DWORD dwIP, BYTE * pEvent, int iLen )
{
	BYTE m_Raw[ MAX_BUFFER ];
	memcpy( m_Raw, pEvent, iLen );
	
	m_seed = dwIP;
	SetConnectType( CONNECT_GAME );

	bool bOut = false;

	for ( int i = ENC_NONE; i < ENC_QTY; i++ )
	{
		SetEncryptionType( (ENCRYPTION_TYPE)i );

		if ( GetEncryptionType() == ENC_TFISH )
			InitTwoFish();
		
		Decrypt( m_Raw, pEvent, iLen );
		
#ifdef DEBUG_CRYPT_MSGS
#ifndef _WIN32
		fprintf(stderr, "GameCrypt %d (%x) type %x-%x\n", i, GetClientVer(), m_Raw[0], pEvent[0]);
#else
		DEBUG_ERR(("GameCrypt %d (%x) type %x-%x\n", i, GetClientVer(), m_Raw[0], pEvent[0]));
#endif
#endif
		
		if ( m_Raw[0] == 0x91 && m_Raw[34] == 0x00 && m_Raw[64] == 0x00 )
		{
			// Ok the new detected encryption is ok
			bOut = true;
			break;
		}
	}
	
	// Well no ecryption guessed, set it to none and let Sphere do the dirty job :P
	if ( !bOut )
	{
		SetEncryptionType( ENC_NONE );
	}
	else
	{
		if ( GetEncryptionType() == ENC_TFISH )
			InitTwoFish();
	}
		
	m_fInit = true;
}

void CCrypt::Encrypt( BYTE * pOutput, const BYTE * pInput, int iLen )
{
	if ( ! iLen )
		return;

	if ( m_ConnectType == CONNECT_LOGIN )
	{
		return;
	}
	
	if ( GetEncryptionType() == ENC_TFISH )
	{
		EncryptMD5( pOutput, pInput, iLen );
		return;
	}

	memcpy( pOutput, pInput, iLen );
}


void CCrypt::Decrypt( BYTE * pOutput, const BYTE * pInput, int iLen  )
{
	if ( ! iLen )
		return;

	if ( m_ConnectType == CONNECT_LOGIN )
	{
		for ( int i=0; i<iLen; i++ )
		{
			pOutput[i] = pInput[i] ^ (BYTE) m_CryptMaskLo;
			DWORD MaskLo = m_CryptMaskLo;
			DWORD MaskHi = m_CryptMaskHi;
			m_CryptMaskLo = ((MaskLo >> 1) | (MaskHi << 31)) ^ m_MasterLo;
			MaskHi = ((MaskHi >> 1) | (MaskLo << 31)) ^ m_MasterHi;
			m_CryptMaskHi = ((MaskHi >> 1) | (MaskLo << 31)) ^ m_MasterHi;
		}
		return;
	}
	
	if ( GetEncryptionType() == ENC_NONE )
	{
		memcpy( pOutput, pInput, iLen );
		return;
	}

	if ( GetEncryptionType() == ENC_TFISH )
	{
		DecryptTwoFish( pOutput, pInput, iLen );
	}
}


void CCrypt::EncryptMD5( BYTE * pOutput, const BYTE * pInput, int iLen )
{
	for (int i = 0; i < iLen; i++)
	{
		pOutput[i] = pInput[i] ^ md5_digest[md5_position++];
		md5_position &= MD5_RESET;
	}
}

void CCrypt::DecryptTwoFish( BYTE * pOutput, const BYTE * pInput, int iLen )
{
	BYTE tmpBuff[TFISH_RESET];
	
	for ( int i = 0; i < iLen; i++ )
	{
		if ( tf_position >= TFISH_RESET )
		{
			blockEncrypt( &tf_cipher, &tf_key, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
			memcpy( &tf_cipherTable, &tmpBuff, TFISH_RESET );
			tf_position = 0;
		}

		pOutput[i] = pInput[i] ^ tf_cipherTable[tf_position++];
	}
}

void CCrypt::InitMD5(BYTE * ucInitialize)
{
	md5_position = 0;
	
	md5_engine.update( ucInitialize, TFISH_RESET );
	md5_engine.finalize();
	md5_engine.numericDigest( &md5_digest[0] );
}

void CCrypt::InitTwoFish()
{
	// Taken from t2tfish.cpp / CCryptNew.cpp
	
	DWORD dwIP = UNPACKDWORD( ((BYTE*) & m_seed ) );
	
	// ---------------------------------------------
	memset(&tf_key, 0, sizeof(keyInstance));
	memset(&tf_cipher, 0, sizeof(cipherInstance));
	tf_position = 0;
	// ---------------------------------------------
	
	makeKey( &tf_key, 1 /*DIR_DECRYPT*/, 0x80, NULL );
	cipherInit( &tf_cipher, 1/*MODE_ECB*/, NULL );

	tf_key.key32[0] = tf_key.key32[1] = tf_key.key32[2] = tf_key.key32[3] = dwIP; //0x7f000001;
	reKey( &tf_key );

	for ( unsigned int i = 0; i < TFISH_RESET; i++ )
		tf_cipherTable[i] = i;

	tf_position = 0;

	BYTE tmpBuff[TFISH_RESET];
	blockEncrypt( &tf_cipher, &tf_key, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
	memcpy( &tf_cipherTable, &tmpBuff, TFISH_RESET );

	if ( GetEncryptionType() == ENC_TFISH )
	{
		InitMD5(&tmpBuff[0]);
	}
}

const WORD CCompressTree::sm_xCompress_Base[COMPRESS_TREE_SIZE] =	// static
{
	// The "golden" key for (0.0.0.0)
	// lowest 4 bits is the length. other source uses 2 int's per WORD here.
	// @ 014b389 in 2.0.3
	// @ 010a3d8 in 2.0.4
	0x0002, 0x01f5, 0x0226, 0x0347, 0x0757, 0x0286, 0x03b6, 0x0327,
	0x0e08, 0x0628, 0x0567, 0x0798, 0x19d9, 0x0978, 0x02a6, 0x0577,
	0x0718, 0x05b8, 0x1cc9, 0x0a78, 0x0257, 0x04f7, 0x0668, 0x07d8,
	0x1919, 0x1ce9, 0x03f7, 0x0909, 0x0598, 0x07b8, 0x0918, 0x0c68,
	0x02d6, 0x1869, 0x06f8, 0x0939, 0x1cca, 0x05a8, 0x1aea, 0x1c0a,
	0x1489, 0x14a9, 0x0829, 0x19fa, 0x1719, 0x1209, 0x0e79, 0x1f3a,
	0x14b9, 0x1009, 0x1909, 0x0136, 0x1619, 0x1259, 0x1339, 0x1959,
	0x1739, 0x1ca9, 0x0869, 0x1e99, 0x0db9, 0x1ec9, 0x08b9, 0x0859,
	0x00a5, 0x0968, 0x09c8, 0x1c39, 0x19c9, 0x08f9, 0x18f9, 0x0919,
	0x0879, 0x0c69, 0x1779, 0x0899, 0x0d69, 0x08c9, 0x1ee9, 0x1eb9,
	0x0849, 0x1649, 0x1759, 0x1cd9, 0x05e8, 0x0889, 0x12b9, 0x1729,
	0x10a9, 0x08d9, 0x13a9, 0x11c9, 0x1e1a, 0x1e0a, 0x1879, 0x1dca,
	0x1dfa, 0x0747, 0x19f9, 0x08d8, 0x0e48, 0x0797, 0x0ea9, 0x0e19,
	0x0408, 0x0417, 0x10b9, 0x0b09, 0x06a8, 0x0c18, 0x0717, 0x0787,
	0x0b18, 0x14c9, 0x0437, 0x0768, 0x0667, 0x04d7, 0x08a9, 0x02f6,
	0x0c98, 0x0ce9, 0x1499, 0x1609, 0x1baa, 0x19ea, 0x39fa, 0x0e59,
	0x1949, 0x1849, 0x1269, 0x0307, 0x06c8, 0x1219, 0x1e89, 0x1c1a,
	0x11da, 0x163a, 0x385a, 0x3dba, 0x17da, 0x106a, 0x397a, 0x24ea,
	0x02e7, 0x0988, 0x33ca, 0x32ea, 0x1e9a, 0x0bf9, 0x3dfa, 0x1dda,
	0x32da, 0x2eda, 0x30ba, 0x107a, 0x2e8a, 0x3dea, 0x125a, 0x1e8a,
	0x0e99, 0x1cda, 0x1b5a, 0x1659, 0x232a, 0x2e1a, 0x3aeb, 0x3c6b,
	0x3e2b, 0x205a, 0x29aa, 0x248a, 0x2cda, 0x23ba, 0x3c5b, 0x251a,
	0x2e9a, 0x252a, 0x1ea9, 0x3a0b, 0x391b, 0x23ca, 0x392b, 0x3d5b,
	0x233a, 0x2cca, 0x390b, 0x1bba, 0x3a1b, 0x3c4b, 0x211a, 0x203a,
	0x12a9, 0x231a, 0x3e0b, 0x29ba, 0x3d7b, 0x202a, 0x3adb, 0x213a,
	0x253a, 0x32ca, 0x23da, 0x23fa, 0x32fa, 0x11ca, 0x384a, 0x31ca,
	0x17ca, 0x30aa, 0x2e0a, 0x276a, 0x250a, 0x3e3b, 0x396a, 0x18fa,
	0x204a, 0x206a, 0x230a, 0x265a, 0x212a, 0x23ea, 0x3acb, 0x393b,
	0x3e1b, 0x1dea, 0x3d6b, 0x31da, 0x3e5b, 0x3e4b, 0x207a, 0x3c7b,
	0x277a, 0x3d4b, 0x0c08, 0x162a, 0x3daa, 0x124a, 0x1b4a, 0x264a,
	0x33da, 0x1d1a, 0x1afa, 0x39ea, 0x24fa, 0x373b, 0x249a, 0x372b,
	0x1679, 0x210a, 0x23aa, 0x1b8a, 0x3afb, 0x18ea, 0x2eca, 0x0627,
	0x00d4 // terminator
} ;

bool CCompressTree::AddBranch( int Value, WORD wCode, int iBits )
{
	// adds a hex value related to it's binary compression code to the decompression tree.

	if ( ! iBits )
		return false;

	CCompressBranch *pCur=&m_Root;
	WORD wMask = 1 << (iBits-1);

    // loop through each bit in the bitfield
	for ( ;wMask; wMask >>= 1 )
 	{
		// if a compression value read in conflicts with an existing
		// compression value, the table is invalid.  Exit program.
		if ( pCur->m_Value >= 0 )
		{
			// if you get here, this means that a new compression value is mutually exclusive
			// with another value.  I.E. both 1101 and 110100 cannot exist in a tree because
			// any decompression routine will stop as soon as it reads 1101
			return false;
		}
        	if ( wCode & wMask )
		{	// if the bit is a 1
			// make a new branch if we need to
			if ( pCur->m_pOne == NULL )
			{
				pCur->m_pOne = new CCompressBranch;
			}
			pCur = pCur->m_pOne;	// traverse to the (possibly new) branch
		}
		else
		{	// if the bit is a 0
			// make a new branch if we need to
			if ( pCur->m_pZero == NULL )
			{
				pCur->m_pZero = new CCompressBranch;
			}
			pCur = pCur->m_pZero;	// traverse to the (possibly new) branch
		}
	}
	// if the end of this bitvalue is within an existing bitvalue's path
	// the table is in error.  I.E. 111 is within 11101's path
	if ( pCur->m_pOne != NULL || pCur->m_pZero != NULL )
		return false;

	// an entry already exists with this binary value
	else if (pCur->m_Value>=0)
	{
		if (((BYTE)pCur->m_Value)!=Value)		// if they are not the same, the table is in error
			return false;
	}
	else
	{
		// set the read in hex value at this compression location
		pCur->m_Value=Value;
	}
	
	return true;
}

int CCompressTree::Encode( BYTE * pOutput, const BYTE * pInput, int inplen ) // static
{
	int iLen=0;
	int bitidx=0;	// Offset in output byte (xOutVal)
	BYTE xOutVal=0;	// Don't bother to init this. It will just roll off all junk anyhow.

	for ( int i=0; i <= inplen; i++ )
	{
		WORD value = sm_xCompress_Base[ ( i == inplen ) ? (COMPRESS_TREE_SIZE-1) : pInput[i] ];
		int nBits = value & 0xF;
		value >>= 4;
		while ( nBits-- )
		{
			xOutVal <<= 1;
			xOutVal |= (value >> nBits) & 0x1;
			if ( ++bitidx == 8)
			{
				bitidx = 0;
				pOutput[iLen++] = xOutVal;
			}
		}
	}
	if (bitidx)	// flush odd bits.
	{
		pOutput[iLen++] = xOutVal << (8-bitidx);
	}

	return iLen;
}
