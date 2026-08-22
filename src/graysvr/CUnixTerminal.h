#ifndef _INC_CUNIXTERMINAL_H
#define _INC_CUNIXTERMINAL_H
#pragma once

#ifndef _WIN32
#include <termios.h>

class CUnixTerminal
{
public:
	static const char *m_sClassName;

private:
	termios m_terminal;
	TCHAR m_szNextChar;
	bool m_fPrepared;

public:
	CUnixTerminal();
	~CUnixTerminal();

public:
	bool isReady();
	TCHAR read();
	void print(LPCTSTR pszText);
	void prepare();

private:
	void restore();

protected:
	CUnixTerminal(const CUnixTerminal &copy);
	CUnixTerminal &operator=(const CUnixTerminal &other);
};

extern CUnixTerminal g_UnixTerminal;

#endif	// _WIN32
#endif	// _INC_CUNIXTERMINAL_H
