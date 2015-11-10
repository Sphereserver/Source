//
// CgrayCom.cpp
//

#include "graycom.h"
#include "graymul.h"
#include "grayproto.h"

#ifdef _BSD
	#include <time.h>
	#include <sys/types.h>
	int getTimezone()
	{
		tm tp;
		memset(&tp, 0x00, sizeof(tp));
		mktime(&tp);
		return (int) tp.tm_zone;
	}
#endif

#ifndef _WIN32
	int	ATOI( const char * str )
	{
		int	res;
		sscanf( str, "%d", &res );
		return res;
	}

	char * ITOA(int value, char *string, int radix)
	{
		sprintf(string, (radix == 16) ? "%x" : "%d", value);
		return string;
	}

	char * LTOA(long value, char *string, int radix)
	{
		sprintf(string, (radix == 16) ? "%lx" : "%ld", value);
		return string;
	}

	void STRREV( char* string )
	{
		char *pEnd = string;
		char temp;     
		while (*pEnd) pEnd++;  
		pEnd--;                         
		while (string < pEnd) 
		{
			temp = *pEnd;            
			*pEnd-- = *string;
			*string++ = temp;       
		}
	}

#else
	const OSVERSIONINFO * GRAY_GetOSInfo()
	{
		// NEVER return NULL !
		static OSVERSIONINFO g_osInfo;
		if ( g_osInfo.dwOSVersionInfoSize != sizeof(g_osInfo) )
		{
			g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
			if ( ! GetVersionEx(&g_osInfo))
			{
				// must be an old version of windows. win95 or win31 ?
				memset( &g_osInfo, 0, sizeof(g_osInfo));
				g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
				g_osInfo.dwPlatformId = VER_PLATFORM_WIN32s;	// probably not right.
			}
		}
		return( &g_osInfo );
	}
#endif


#ifdef NCHAR
static int CvtSystemToUNICODE( WCHAR & wChar, LPCTSTR pInp, int iSizeInBytes )
{
	// Convert a UTF8 encoded string to a single unicode char.
	// RETURN: The length used from input string. < iSizeInBytes

	// bytes bits representation 
	// 1 7	0bbbbbbb 
	// 2 11 110bbbbb 10bbbbbb 
	// 3 16 1110bbbb 10bbbbbb 10bbbbbb 
	// 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb 

	BYTE ch = *pInp;
	ASSERT( ch >= 0x80 );	// needs special UTF8 decoding.

	int iBytes;
	int iStartBits;
	if (( ch & 0xe0 ) == 0x0c0 ) // 2 bytes
	{
		iBytes = 2;
		iStartBits = 5;
	}
	else if (( ch & 0xf0 ) == 0x0e0 ) // 3 bytes
	{
		iBytes = 3;
		iStartBits = 4;
	}
	else if (( ch & 0xf8 ) == 0x0f0 ) // 3 bytes 
	{
		iBytes = 4;
		iStartBits = 3;
	}
	else 
	{
		return( -1 );	// invalid format !
	}

	if ( iBytes > iSizeInBytes )	// not big enough to hold it.
		return( 0 );

	WCHAR wCharTmp = ch & ((1<<iStartBits)-1);
	int iInp = 1;
	for ( ; iInp < iBytes; iInp++ )
	{
		ch = pInp[iInp];
		if (( ch & 0xc0 ) != 0x80 )	// bad coding.
			return( -1 );
		wCharTmp <<= 6;
		wCharTmp |= ch & 0x3f;
	}

	wChar = wCharTmp;
	return( iBytes );
}

static int CvtUNICODEToSystem( TCHAR * pOut, int iSizeOutBytes, WCHAR wChar )
{
	// Convert a single unicode char to system string.
	// RETURN: The length < iSizeOutBytes

	// bytes bits representation 
	// 1 7	0bbbbbbb 
	// 2 11 110bbbbb 10bbbbbb 
	// 3 16 1110bbbb 10bbbbbb 10bbbbbb 
	// 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb 

	ASSERT( wChar >= 0x80 );	// needs special UTF8 encoding.

	int iBytes;
	int iStartBits;
	if ( wChar < (1<<11) )
	{
		iBytes = 2;
		iStartBits = 5;
	}
	else if ( wChar < (1<<16) )
	{
		iBytes = 3;
		iStartBits = 4;
	}
	else if ( wChar < (1<<21) )
	{
		iBytes = 4;
		iStartBits = 3;
	}
	else
	{
		return( -1 );	// not valid UNICODE char.
	}

	if ( iBytes > iSizeOutBytes )	// not big enough to hold it.
		return( 0 );

	int iOut = iBytes-1;
	for ( ; iOut > 0; iOut-- )
	{
		pOut[iOut] = 0x80 | ( wChar & ((1<<6)-1));
		wChar >>= 6;
	}

	ASSERT( wChar < (1<<iStartBits));
	pOut[0] = static_cast<TCHAR>( ( 0xfe << iStartBits ) | wChar );

	return( iBytes );
}

int CvtSystemToNUNICODE( NCHAR * pOut, int iSizeOutChars, LPCTSTR pInp, int iSizeInBytes )
{
	//
	// Convert the system default text format UTF8 to UNICODE
	// May be network byte order !
	// Add null.
	// ARGS:
	//   iSizeInBytes = size ofthe input string. -1 = null terminated.
	// RETURN:
	//  Number of wide chars. not including null.
	//

	ASSERT(pOut);
	ASSERT(pInp);
	if ( iSizeOutChars <= 0 )
		return -1;

	if ( iSizeInBytes <= -1 )
	{
		iSizeInBytes = strlen(pInp);
	}
	if ( iSizeInBytes <= 0 )
	{
		pOut[0] = 0;
		return( 0 );
	}

	iSizeOutChars--;

	int iOut=0;

#ifdef _WIN32
	const OSVERSIONINFO * posInfo = GRAY_GetOSInfo();
	if ( posInfo->dwPlatformId == VER_PLATFORM_WIN32_NT ||
		posInfo->dwMajorVersion > 4 )
	{
		int iOutTmp = MultiByteToWideChar(
			CP_UTF8,         // code page
			0,         // character-type options
			pInp, // address of string to map
			iSizeInBytes,      // number of bytes in string
			reinterpret_cast<LPWSTR>(pOut),  // address of wide-character buffer
			iSizeOutChars        // size of buffer
			);
		if ( iOutTmp <= 0 )
		{
			pOut[0] = 0;
			return( 0 );
		}
		if ( iOutTmp > iSizeOutChars )	// this should never happen !
		{
			pOut[0] = 0;
			return( 0 );
		}

		// flip all the words to network order .
		for ( ; iOut<iOutTmp; iOut++ )
		{
			pOut[iOut] = *(reinterpret_cast<WCHAR *>(&(pOut[iOut])));
		}
	}
	else
#endif
	{
		// Win95 or Linux
		int iInp=0;
		for ( ; iInp < iSizeInBytes; )
		{
			BYTE ch = pInp[iInp];
			if ( ch == 0 )
				break;

			if ( iOut >= iSizeOutChars )
				break;

			if ( ch >= 0x80 )	// special UTF8 encoded char.
			{
				WCHAR wChar;
				int iInpTmp = CvtSystemToUNICODE( wChar, pInp+iInp, iSizeInBytes-iInp );
				if ( iInpTmp <= 0 )
				{
					break;
				}
				pOut[iOut] = wChar;
				iInp += iInpTmp;
			}
			else
			{
				pOut[iOut] = ch;
				iInp++;
			}

			iOut++;
		}
	}

	pOut[iOut] = 0;
	return( iOut );
}

int CvtNUNICODEToSystem( TCHAR * pOut, int iSizeOutBytes, const NCHAR * pInp, int iSizeInChars )
{
	// ARGS:
	//  iSizeInBytes = space we have (included null char)
	// RETURN:
	//  Number of bytes. (not including null)
	// NOTE:
	//  This need not be a properly terminated string.

	if ( iSizeInChars > iSizeOutBytes )	// iSizeOutBytes should always be bigger
		iSizeInChars = iSizeOutBytes;
	if ( iSizeInChars <= 0 )
	{
		pOut[0] = 0;
		return( 0 );
	}

	iSizeOutBytes--;

	int iOut=0;
	int iInp=0;

#ifdef _WIN32
	const OSVERSIONINFO * posInfo = GRAY_GetOSInfo();
	if ( posInfo->dwPlatformId == VER_PLATFORM_WIN32_NT ||
		posInfo->dwMajorVersion > 4 )
	{
		// Windows 98, 2000 or NT

		// Flip all from network order.
		WCHAR szBuffer[ 1024*8 ];
		for ( ; iInp < COUNTOF(szBuffer) - 1 && iInp < iSizeInChars && pInp[iInp]; iInp++ )
		{
			szBuffer[iInp] = pInp[iInp];
		}
		szBuffer[iInp] = '\0';

		// Convert to proper UTF8
		iOut = WideCharToMultiByte(
			CP_UTF8,         // code page
			0,         // performance and mapping flags
			szBuffer, // address of wide-character string
			iInp,       // number of characters in string
			pOut,  // address of buffer for new string
			iSizeOutBytes,      // size of buffer in bytes
			NULL,  // address of default for unmappable characters
			NULL  // address of flag set when default char. used
			);
		if ( iOut < 0 )
		{
			pOut[0] = 0;	// make sure it's null terminated
			return( 0 );
		}
	}
	else
#endif
	{
		// Win95 or linux = just assume its really ASCII
		for ( ; iInp < iSizeInChars; iInp++ )
		{
			// Flip all from network order.
			WCHAR wChar = pInp[iInp];
			if ( ! wChar )
				break;

			if ( iOut >= iSizeOutBytes )
				break;
			if ( wChar >= 0x80 )	// needs special UTF8 encoding.
			{
				int iOutTmp = CvtUNICODEToSystem( pOut+iOut, iSizeOutBytes-iOut, wChar );
				if ( iOutTmp <= 0 )
				{
					break;
				}
				iOut += iOutTmp;
			}
			else
			{
				pOut[iOut] = static_cast<TCHAR>(wChar);
				iOut++;
			}
		}
	}

	pOut[iOut] = 0;	// make sure it's null terminated
	return( iOut );
}

#endif

extern "C"
{
	void globalendsymbol() {}	// put this here as just the ending offset.
	const int globalenddata = 0xffffffff;
}
