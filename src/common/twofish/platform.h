/***************************************************************************
PLATFORM.H	-- Platform-specific defines for TWOFISH code

Submitters:
Bruce Schneier, Counterpane Systems
Doug Whiting,	Hi/fn
John Kelsey,	Counterpane Systems
Chris Hall,		Counterpane Systems
David Wagner,	UC Berkeley

Code Author:		Doug Whiting,	Hi/fn

Version  1.00		April 1998

Copyright 1998, Hi/fn and Counterpane Systems.  All rights reserved.

Notes:
*	Tab size is set to 4 characters in this file

***************************************************************************/

/* use intrinsic rotate if possible */
#define ROL( x, y ) ( ( (x) << (y) ) | ( (x) >> ( 32 - (y) ) ) )
#define ROR( x, y ) ( ( (x) >> (y) ) | ( (x) << ( 32 - (y) ) ) )

#if defined(_MSC_VER)
#include	<stdlib.h>					/* get prototypes for rotation functions */
#undef	ROL
#undef	ROR
#pragma intrinsic(_lrotl,_lrotr)		/* use intrinsic compiler rotations */
#define	ROL(x,n)	_lrotl(x,n)			
#define	ROR(x,n)	_lrotr(x,n)
#endif

// Change this to compile on a BigEndian machine
#if !defined(Q_OS_MAC)
#define LittleEndianCrypt 1
#endif
#define ALIGN32 1

#if LittleEndianCrypt
#define		Bswap(x)			(x)		/* NOP for little-endian machines */
#define		ADDR_XOR			0		/* NOP for little-endian machines */
#else
#define		Bswap(x)			((ROR(x,8) & 0xFF00FF00) | (ROL(x,8) & 0x00FF00FF))
#define		ADDR_XOR			3		/* convert byte address in dword */
#endif

/*	Macros for extracting bytes from dwords (correct for endianness) */
#define	_b(x,N)	(((BYTE *)&x)[((N) & 3) ^ ADDR_XOR]) /* pick bytes out of a dword */

#define		b0(x)			_b(x,0)		/* extract LSB of DWORD */
#define		b1(x)			_b(x,1)
#define		b2(x)			_b(x,2)
#define		b3(x)			_b(x,3)		/* extract MSB of DWORD */

