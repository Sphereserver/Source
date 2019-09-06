#include "graycom.h"
#include "grayproto.h"

#ifdef _WIN32
	const OSVERSIONINFO *GRAY_GetOSInfo()
	{
		static OSVERSIONINFO g_osInfo;
		if ( g_osInfo.dwOSVersionInfoSize != sizeof(g_osInfo) )
		{
			g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
			if ( !GetVersionEx(&g_osInfo) )		// unable to get version (Windows < 98?)
			{
				memset(&g_osInfo, 0, sizeof(g_osInfo));
				g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
				g_osInfo.dwPlatformId = VER_PLATFORM_WIN32s;	// Windows 3.x
			}
		}
		return &g_osInfo;	// NEVER return null!
	}
#else
	#ifdef _BSD
		#include <time.h>
		#include <sys/types.h>

		int getTimezone()
		{
			tm tp;
			memset(&tp, 0x0, sizeof(tp));
			mktime(&tp);
			return static_cast<int>(tp.tm_zone);
		}
	#endif

	int	ATOI(const char *str)
	{
		int	res;
		sscanf(str, "%d", &res);
		return res;
	}

	char *ITOA(int value, char *buffer, int radix)
	{
		sprintf(buffer, (radix == 16) ? "%x" : "%d", value);
		return buffer;
	}

	char *LTOA(long value, char *buffer, int radix)
	{
		sprintf(buffer, (radix == 16) ? "%lx" : "%ld", value);
		return buffer;
	}

	void STRREV(char *str)
	{
		char *pszEnd = str;
		while ( *pszEnd )
			pszEnd++;
		pszEnd--;

		char pszTemp;
		while ( str < pszEnd )
		{
			pszTemp = *pszEnd;
			*pszEnd-- = *str;
			*str++ = pszTemp;
		}
	}
#endif

#ifdef NCHAR
static int CvtSystemToUNICODE(WCHAR &wChar, LPCTSTR pszInp, int iSizeInBytes)
{
	// Convert a UTF8 encoded string to a single unicode char
	// RETURN: The length < iSizeInBytes

	// Bytes	Bits	Representation 
	// 1		7		0bbbbbbb
	// 2		11		110bbbbb 10bbbbbb
	// 3		16		1110bbbb 10bbbbbb 10bbbbbb
	// 4		21		11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

	BYTE ch = *pszInp;
	ASSERT(ch >= 0x80);	// needs special UTF8 decoding

	int iBytes;
	int iStartBits;
	if ( (ch & 0xE0) == 0xC0 ) // 2 bytes
	{
		iBytes = 2;
		iStartBits = 5;
	}
	else if ( (ch & 0xF0) == 0xE0 ) // 3 bytes
	{
		iBytes = 3;
		iStartBits = 4;
	}
	else if ( (ch & 0xF8) == 0xF0 ) // 4 bytes 
	{
		iBytes = 4;
		iStartBits = 3;
	}
	else
		return -1;	// invalid format

	if ( iBytes > iSizeInBytes )	// not big enough to hold it
		return 0;

	WCHAR wCharTmp = ch & ((1 << iStartBits) - 1);
	int iInp = 1;
	for ( ; iInp < iBytes; iInp++ )
	{
		ch = pszInp[iInp];
		if ( (ch & 0xC0) != 0x80 )	// bad coding
			return -1;
		wCharTmp <<= 6;
		wCharTmp |= (ch & 0x3F);
	}

	wChar = wCharTmp;
	return iBytes;
}

static int CvtUNICODEToSystem(TCHAR *pszOut, int iSizeOutBytes, WCHAR wChar)
{
	// Convert a single unicode char to system string
	// RETURN: The length < iSizeOutBytes

	// Bytes	Bits	Representation 
	// 1		7		0bbbbbbb
	// 2		11		110bbbbb 10bbbbbb
	// 3		16		1110bbbb 10bbbbbb 10bbbbbb
	// 4		21		11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

	ASSERT(wChar >= 0x80);	// needs special UTF8 encoding

	int iBytes;
	int iStartBits;
	if ( wChar < (1 << 11) )
	{
		iBytes = 2;
		iStartBits = 5;
	}
	// By default WCHAR have 2 bytes (1 << 11) on Windows and 4 bytes (1 << 21) on Linux.
	// But Sphere linux build also have WCHAR manually set to 2 bytes (unsigned short) to
	// match Windows build, so these 2 checks below are never reached and can be disabled
	// to avoid some static analyzer warnings like "unreachable code", etc
	/*else if ( wChar < (1 << 16) )
	{
		iBytes = 3;
		iStartBits = 4;
	}
	else if ( wChar < (1 << 21) )
	{
		iBytes = 4;
		iStartBits = 3;
	}*/
	else
		return -1;	// not valid UNICODE char

	if ( iBytes > iSizeOutBytes )	// not big enough to hold it
		return 0;

	int iOut = iBytes - 1;
	for ( ; iOut > 0; iOut-- )
	{
		pszOut[iOut] = 0x80 | (wChar & ((1 << 6) - 1));
		wChar >>= 6;
	}

	ASSERT(wChar < (1 << iStartBits));
	pszOut[0] = static_cast<TCHAR>((0xFE << iStartBits) | wChar);
	return iBytes;
}

int CvtSystemToNUNICODE(NCHAR *pszOut, int iSizeOutChars, LPCTSTR pszInp, int iSizeInBytes)
{
	// Convert system default text format UTF8 to UNICODE
	// This need not be a properly terminated string
	// May be network byte order!
	// ARGS:
	//  iSizeInBytes = size of the input string, including null char (-1 = null terminated)
	// RETURN:
	//  Number of wide chars (not including null)

	ASSERT(pszOut);
	ASSERT(pszInp);
	if ( iSizeOutChars <= 0 )
		return -1;

	if ( iSizeInBytes <= -1 )
		iSizeInBytes = strlen(pszInp);

	if ( iSizeInBytes <= 0 )
	{
		pszOut[0] = 0;
		return 0;
	}

	iSizeOutChars--;
	int iOut = 0;

#ifdef _WIN32
	const OSVERSIONINFO *pOSInfo = GRAY_GetOSInfo();
	if ( (pOSInfo->dwPlatformId == VER_PLATFORM_WIN32_NT) || (pOSInfo->dwMajorVersion > 4) )
	{
		int iOutTmp = MultiByteToWideChar(
			CP_UTF8,							// code page
			0,									// character-type options
			pszInp,								// address of string to map
			iSizeInBytes,						// number of bytes in string
			reinterpret_cast<LPWSTR>(pszOut),	// address of wide-character buffer
			iSizeOutChars);						// buffer size

		if ( iOutTmp <= 0 )
		{
			pszOut[0] = 0;
			return 0;
		}
		if ( iOutTmp > iSizeOutChars )	// this should never happen
		{
			pszOut[0] = 0;
			return 0;
		}

		// Flip all the words to network order
		for ( ; iOut < iOutTmp; iOut++ )
			pszOut[iOut] = *(reinterpret_cast<WCHAR *>(&pszOut[iOut]));
	}
	else
#endif
	{
		for ( int iInp = 0; iInp < iSizeInBytes; )
		{
			BYTE ch = pszInp[iInp];
			if ( ch == 0 )
				break;

			if ( iOut >= iSizeOutChars )
				break;

			if ( ch >= 0x80 )	// special UTF8 encoded char
			{
				WCHAR wChar;
				int iInpTmp = CvtSystemToUNICODE(wChar, pszInp + iInp, iSizeInBytes - iInp);
				if ( iInpTmp <= 0 )
					break;

				pszOut[iOut] = wChar;
				iInp += iInpTmp;
			}
			else
			{
				pszOut[iOut] = ch;
				iInp++;
			}
			iOut++;
		}
	}

	pszOut[iOut] = 0;	// make sure it's null terminated
	return iOut;
}

int CvtNUNICODEToSystem(TCHAR *pszOut, int iSizeOutBytes, const NCHAR *pszInp, int iSizeInChars)
{
	// Convert UNICODE to system default text format UTF8
	// This need not be a properly terminated string
	// ARGS:
	//  iSizeInBytes = size of the input string, including null char (-1 = null terminated)
	// RETURN:
	//  Number of bytes (not including null)

	if ( iSizeInChars > iSizeOutBytes )		// iSizeOutBytes should always be bigger
		iSizeInChars = iSizeOutBytes;

	if ( iSizeInChars <= 0 )
	{
		pszOut[0] = 0;
		return 0;
	}

	iSizeOutBytes--;
	int iOut = 0;
	int iInp = 0;

#ifdef _WIN32
	const OSVERSIONINFO *pOSInfo = GRAY_GetOSInfo();
	if ( (pOSInfo->dwPlatformId == VER_PLATFORM_WIN32_NT) || (pOSInfo->dwMajorVersion > 4) )
	{
		// Flip all from network order
		WCHAR szBuffer[1024 * 8];
		for ( ; iInp < COUNTOF(szBuffer) - 1 && (iInp < iSizeInChars) && pszInp[iInp]; iInp++ )
			szBuffer[iInp] = pszInp[iInp];

		szBuffer[iInp] = '\0';

		// Convert to proper UTF8
		iOut = WideCharToMultiByte(
			CP_UTF8,	// code page
			0,			// performance and mapping flags
			szBuffer,	// address of wide-character string
			iInp,		// number of characters in string
			pszOut,		// address of buffer for new string
			iSizeOutBytes,	// size of buffer in bytes
			NULL,		// address of default for unmappable characters
			NULL		// address of flag set when default char. used
		);

		if ( iOut < 0 )
		{
			pszOut[0] = 0;
			return 0;
		}
	}
	else
#endif
	{
		// Just assume its really ASCII
		for ( ; iInp < iSizeInChars; iInp++ )
		{
			// Flip all from network order
			WCHAR wChar = pszInp[iInp];
			if ( !wChar )
				break;

			if ( iOut >= iSizeOutBytes )
				break;
			if ( wChar >= 0x80 )	// needs special UTF8 encoding
			{
				int iOutTmp = CvtUNICODEToSystem(pszOut + iOut, iSizeOutBytes - iOut, wChar);
				if ( iOutTmp <= 0 )
					break;

				iOut += iOutTmp;
			}
			else
			{
				pszOut[iOut] = static_cast<TCHAR>(wChar);
				iOut++;
			}
		}
	}

	pszOut[iOut] = 0;	// make sure it's null terminated
	return iOut;
}

#endif

extern "C"
{
	void globalendsymbol() {}	// put this here as just the ending offset
	const int globalenddata = 0xFFFFFFFF;
}
