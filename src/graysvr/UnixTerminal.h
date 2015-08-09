#ifndef _INC_UNIXTERMINAL_H_
#define _INC_UNIXTERMINAL_H_
#ifndef _WIN32

#ifdef _USECURSES
#include <curses.h>
#pragma comment(lib, "ncurses")
#else
#include <termios.h>
#endif

class UnixTerminal
{
private:
#ifdef _USECURSES
	WINDOW * m_window;
#else
	termios m_original;
#endif
	TCHAR m_nextChar;
	bool m_isColorEnabled;
	bool m_prepared;

public:
	enum COLOR_TYPE
	{
		COL_DEFAULT = 0,
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
	UnixTerminal();
	~UnixTerminal();

protected:
	UnixTerminal(const UnixTerminal & copy);
	UnixTerminal & operator=(const UnixTerminal & other);

public:
	bool isReady();
	TCHAR read();
	void prepare();
	void print(LPCTSTR message);
	void setColor(COLOR_TYPE color);
	void setColorEnabled(bool enable);

private:
	void prepareColor();
	void restore();

public:
	bool isColorEnabled() const
	{
		return m_isColorEnabled;
	}
};

extern UnixTerminal g_UnixTerminal;

#endif
#endif
