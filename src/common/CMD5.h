/*
	This code has been built from scratch using rfc1321 and Colin Plumb's
	public domain code.
*/

#ifndef _INC_CMD5_H
#define _INC_CMD5_H
#pragma once

#include <string.h>

class CMD5
{
private:
	unsigned int m_buffer[4];
	unsigned int m_bits[2];
	unsigned char m_input[64];
	bool m_finalized;

	void update();

public:
	CMD5();
	~CMD5();
private:
	CMD5(const CMD5& copy);
	CMD5& operator=(const CMD5& other);

public:
	void reset();
	void update( const unsigned char * data, unsigned int length );
	void finalize();

	// Digest has to be 33 bytes long
	void digest( char * digest );
	// Get digest in a "numeric" form to be usable
	void numericDigest( unsigned char * digest );
	
	inline static void fastDigest( char * digest, const char * message )
	{
		CMD5 ctx;
		ctx.update( reinterpret_cast<const unsigned char *>(message), static_cast<unsigned int>(strlen( message ) ));
		ctx.finalize();
		ctx.digest( digest );
	}
};

#endif	// _INC_CMD5_H
