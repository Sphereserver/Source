
#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "CMD5.h"

CMD5::CMD5()
{
	reset();
}

CMD5::~CMD5()
{
}

void CMD5::reset()
{
	// Reset our finalizd state.
	m_finalized = false;

	// Initialize our initial state.
	m_buffer[0] = 0x67452301;
	m_buffer[1] = 0xefcdab89;
	m_buffer[2] = 0x98badcfe;
	m_buffer[3] = 0x10325476;

	// Reset the Bit Counter.
	m_bits[0] = 0;
	m_bits[1] = 0;
}

inline void byteReverse( unsigned char *buffer, unsigned int longs )
{
    unsigned int temp;
    do
	{
		temp = static_cast<unsigned int>( static_cast<unsigned int>(buffer[3]) << 8 | buffer[2] ) << 16 | ( static_cast<unsigned int>(buffer[1]) << 8 | buffer[0] );
		reinterpret_cast<unsigned int *>(buffer)[0] = temp;
		buffer += 4;
    }
	while( --longs );
}

// I used the defines found in the public domain implementation here
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))
#define MD5STEP( f, w, x, y, z, data, s ) ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

void CMD5::update()
{
    register unsigned int a, b, c, d;
	unsigned int *ptrInput = reinterpret_cast<unsigned int *>( m_input );

    a = m_buffer[0];
    b = m_buffer[1];
    c = m_buffer[2];
    d = m_buffer[3];

    MD5STEP(F1, a, b, c, d, ptrInput[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, ptrInput[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, ptrInput[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, ptrInput[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, ptrInput[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, ptrInput[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, ptrInput[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, ptrInput[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, ptrInput[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, ptrInput[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, ptrInput[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, ptrInput[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, ptrInput[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, ptrInput[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, ptrInput[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, ptrInput[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, ptrInput[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, ptrInput[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, ptrInput[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, ptrInput[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, ptrInput[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, ptrInput[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, ptrInput[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, ptrInput[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, ptrInput[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, ptrInput[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, ptrInput[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, ptrInput[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, ptrInput[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, ptrInput[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, ptrInput[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, ptrInput[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, ptrInput[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, ptrInput[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, ptrInput[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, ptrInput[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, ptrInput[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, ptrInput[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, ptrInput[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, ptrInput[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, ptrInput[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, ptrInput[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, ptrInput[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, ptrInput[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, ptrInput[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, ptrInput[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, ptrInput[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, ptrInput[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, ptrInput[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, ptrInput[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, ptrInput[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, ptrInput[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, ptrInput[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, ptrInput[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, ptrInput[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, ptrInput[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, ptrInput[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, ptrInput[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, ptrInput[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, ptrInput[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, ptrInput[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, ptrInput[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, ptrInput[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, ptrInput[9] + 0xeb86d391, 21);

    m_buffer[0] += a;
    m_buffer[1] += b;
    m_buffer[2] += c;
    m_buffer[3] += d;
}

void CMD5::update( const unsigned char *data, unsigned int length )
{
	if( m_finalized )
		return;
		// throw std::exception( "CMD5::update() although finalized flag is set." );

	unsigned int temp = m_bits[0];

	// First Part overlapped?
	if( (( m_bits[0] = temp + ( length << 3 ) )) < temp )
	{
		++m_bits[1];
	}

	m_bits[1] += length >> 29;

	// Calculate Bitcount
	temp = ( temp >> 3 ) & 0x3F;

	// We still have data waiting
	if( temp )
	{
		unsigned char *ptrInput = &m_input[temp];

		// How much is left
		temp = 64 - temp;

		// If we don't have enough data to fill our buffer,
		// copy the data we have into our "waiting buffer"
		// and return.
		if( length < temp )
		{
			memcpy( ptrInput, data, length );
			return;
		}

		// We have enough data to clear our temporary buffer
		memcpy( ptrInput, data, temp );		
		byteReverse( m_input, 16 );
		update();

		data += temp;
		length -= temp;
	}

	// Process the data in 64 byte chunks or save it for further processing
    while( length >= 64 )
	{
		memcpy( m_input, data, 64 );
		byteReverse( m_input, 16 );
		update();
		data += 64;
		length -= 64;
	}

	memcpy( m_input, data, length );
}

void CMD5::finalize()
{
	if( m_finalized )
		return;
		//throw std::exception( "CMD5::finalize() although finalized flag is set." );

	// Pad if neccesary to a 448 bit boundary and add the 64 bit of our bitcount
	unsigned int count = ( m_bits[0] >> 3 ) & 0x3F;

	unsigned char *ptrInput = &m_input[count];
	
	// Set the first byte of the padding to 0x80
	*ptrInput++ = 0x80;

	count = 64 - 1 - count;

	// Too few bytes are left in our buffer, pad out the current block and then
	// append another
	if( count < 8 )
	{
		memset( ptrInput, 0, count );
		byteReverse( m_input, 16 );
		update();

		// Fill the next block with zeros and leave free space for
		// our bitcount
		memset( m_input, 0, 56 );
	}
	// We have enough space left for our bitcount
	else
	{
		memset( ptrInput, 0, count - 8 );
	}

	// Reverse the first 14 dwords
	byteReverse( m_input, 14 );

	// Append Length and update
	reinterpret_cast<unsigned int *>(m_input)[14] = m_bits[0];
	reinterpret_cast<unsigned int *>(m_input)[15] = m_bits[1];

	update();

	// Reverse our Digest
	byteReverse( reinterpret_cast<unsigned char *>(m_buffer), 4 );

	m_finalized = true;
}

void CMD5::numericDigest( unsigned char *digest )
{
	if( !m_finalized )
		return;
		// throw std::exception( "Call to CMD5::digest() without finalized flag being set." );

	unsigned char * buffer = reinterpret_cast<unsigned char *>( m_buffer );

	// 16 byte a 2 characters
	for ( unsigned int i = 0; i < 16; ++i )
	{
		digest[i] = buffer[i];
	}
}

void CMD5::digest( char *digest )
{
	if( !m_finalized )
		return;
		// throw std::exception( "Call to CMD5::digest() without finalized flag being set." );

	digest[0] = 0;

	unsigned char * buffer = reinterpret_cast<unsigned char *>( m_buffer );

	// 16 byte a 2 characters
    for( unsigned int i = 0; i < 16; ++i )
	{
		char temp[3];
		sprintf( temp, "%02x", buffer[i] );
		strcat( digest, temp );
	}
}
