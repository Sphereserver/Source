#ifndef _WIN32
#include "graysvr.h"	// predef header
#include "CUnixTerminal.h"
#ifndef _USECURSES
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#endif

CUnixTerminal g_UnixTerminal;

CUnixTerminal::CUnixTerminal()
{
	EXC_TRY("PrepareTerminal");
	prepare();
	EXC_CATCH;
}

CUnixTerminal::~CUnixTerminal()
{
	EXC_TRY("RestoreTerminal");
	restore();
	EXC_CATCH;
}

bool CUnixTerminal::isReady()
{
	ADDTOCALLSTACK("CUnixTerminal::isReady");
	if (m_nextChar != '\0')
		return true;

#ifdef _USECURSES
	int c = wgetch(m_window);
	if (c == ERR)
		return false;

	if (c >= 0 && c < 256)
	{
		// character input received
		m_nextChar = static_cast<TCHAR>(c);
		return true;
	}

	switch (c)
	{
		// map special characters to input
		case KEY_BACKSPACE:
			m_nextChar = '\b';
			return true;
		case KEY_ENTER:
			m_nextChar = '\n';
			return true;

		case KEY_RESIZE:
			// todo: resize window
			return false;
		case KEY_MOUSE:
			// todo: handle mouse event
			return false;
	}
#else
	// check if input is waiting
	fd_set consoleFds;
	FD_ZERO(&consoleFds);
	FD_SET(STDIN_FILENO, &consoleFds);

	timeval tvTimeout;
	tvTimeout.tv_sec = 0;
	tvTimeout.tv_usec = 1;

	if (select(1, &consoleFds, 0, 0, &tvTimeout) <= 0)
		return false;

	// get next character
	int c = fgetc(stdin);
	if (c < 0 || c > 255)
		return false;

	// map backspace
	if (c == m_original.c_cc[VERASE])
		c = '\b';

	// echo to console
	fputc(c, stdout);
	fflush(stdout);

	m_nextChar = static_cast<TCHAR>(c);
	return m_nextChar != '\0';
#endif

	return false;
}

TCHAR CUnixTerminal::read()
{
	ADDTOCALLSTACK("CUnixTerminal::read");
	TCHAR c = m_nextChar;
	m_nextChar = '\0';
	return c;
}

void CUnixTerminal::setColor(COLOR_TYPE color)
{
	if (m_isColorEnabled == false)
		return;

	if (color <= COL_DEFAULT || color >= COL_QTY)
		color = COL_DEFAULT;

#ifdef _USECURSES
	wattrset(m_window, COLOR_PAIR(color));
#else
	if (color == COL_DEFAULT)
		fprintf(stdout, "\033[0m");
	else
		fprintf(stdout, "\033[0;%dm", 30 + static_cast<int>(color));
#endif
}

void CUnixTerminal::print(LPCTSTR message)
{
	ADDTOCALLSTACK("CUnixTerminal::print");
	ASSERT(message != NULL);
#ifdef _USECURSES
	waddstr(m_window, message);
	wrefresh(m_window);
#else
	fputs(message, stdout);
#endif
}

void CUnixTerminal::prepare()
{
	ADDTOCALLSTACK("CUnixTerminal::prepare");
	ASSERT(m_prepared == false);

#ifdef _USECURSES
	initscr();	// init screen
	cbreak();	// read one character at a time
	echo();		// echo input

	// create a window, same size as terminal
	int lines, columns;
	getmaxyx(stdscr, lines, columns);
	m_window = newwin(lines, columns, 0, 0);

	keypad(m_window, TRUE);		// process special chars
	nodelay(m_window, TRUE);	// non-blocking input
	scrollok(m_window, TRUE);	// allow scrolling
	refresh();		// draw screen

#else
	// save existing attributes
	if (tcgetattr(STDIN_FILENO, &m_original) < 0)
		throw CGrayError(LOGL_WARN, 0, "failed to get terminal attributes");

	// set new terminal attributes
	termios term_caps = m_original;
	term_caps.c_lflag &= ~(ICANON | ECHO);
	term_caps.c_cc[VMIN] = 1;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0)
		throw CGrayError(LOGL_WARN, 0, "failed to set terminal attributes");

	setbuf(stdin, NULL);
#endif

	prepareColor();
	m_nextChar = '\0';
	m_prepared = true;
}

void CUnixTerminal::prepareColor()
{
	ADDTOCALLSTACK("CUnixTerminal::prepareColor");

#ifdef _USECURSES
	m_isColorEnabled = has_colors();
	if (m_isColorEnabled)
	{
		// initialise colours, and map our enum to the actual colour codes
		start_color();
		init_pair(COL_RED,		COLOR_RED,		COLOR_BLACK);
		init_pair(COL_GREEN,	COLOR_GREEN,	COLOR_BLACK);
		init_pair(COL_YELLOW,	COLOR_YELLOW,	COLOR_BLACK);
		init_pair(COL_BLUE,		COLOR_BLUE,		COLOR_BLACK);
		init_pair(COL_CYAN,		COLOR_CYAN,		COLOR_BLACK);
		init_pair(COL_MAGENTA,	COLOR_MAGENTA,	COLOR_BLACK);
		init_pair(COL_WHITE,	COLOR_WHITE,	COLOR_BLACK);
	}

#else

	// enable colour based on known terminal types
	m_isColorEnabled = false;

	const char * termtype = getenv("TERM");
	if (termtype != NULL)
	{
		static const char *sz_Supported_Terminals[] =
		{
			"aixterm", "ansi", "color_xterm",
			"con132x25", "con132x30", "con132x43", "con132x60",
			"con80x25", "con80x28", "con80x30", "con80x43", "con80x50", "con80x60",
			"cons25", "console", "gnome", "hft", "kon", "konsole", "kterm",
			"linux", "rxvt", "screen", "screen.linux", "vt100-color",
			"xterm", "xterm-color"
		};

		for (size_t i = 0; i < COUNTOF(sz_Supported_Terminals); ++i)
		{
			if (strcmp(termtype, sz_Supported_Terminals[i]) == 0)
			{
				m_isColorEnabled = true;
				break;
			}
		}
	}
#endif
}

void CUnixTerminal::restore()
{
	ADDTOCALLSTACK("CUnixTerminal::restore");
	ASSERT(m_prepared);

#ifdef _USECURSES
	endwin();	// clean up
	m_window = NULL;
	m_nextChar = '\0';
#else
	// restore original terminal state
	if (tcsetattr(STDIN_FILENO, TCSANOW, &m_original) < 0)
		throw CGrayError(LOGL_WARN, 0, "failed to restore terminal attributes");
#endif

	m_prepared = false;
}

void CUnixTerminal::setColorEnabled(bool enable)
{
	m_isColorEnabled = enable;
}

#endif	// _WIN32
