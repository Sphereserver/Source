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
	enum COLOR_TYPE
	{
		COL_DEFAULT,
		COL_RED,
		COL_GREEN,
		COL_YELLOW,
		COL_BLUE,
		COL_MAGENTA,
		COL_CYAN,
		COL_WHITE,
		COL_QTY
	};

public:
	CUnixTerminal();
	~CUnixTerminal();

public:
	bool isReady();
	TCHAR read();
	void setColor(COLOR_TYPE color);
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
