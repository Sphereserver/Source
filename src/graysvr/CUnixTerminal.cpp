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
	prepare();
}

CUnixTerminal::~CUnixTerminal()
{
	restore();
}

bool CUnixTerminal::isReady()
{
	ADDTOCALLSTACK("CUnixTerminal::isReady");
	if ( m_szNextChar != '\0' )
		return true;

#ifdef _USECURSES
	int ch = wgetch(m_pWindow);
	if ( ch == ERR )
		return false;

	if ( (ch >= 0) && (ch <= 255) )
	{
		// Character input received
		m_szNextChar = static_cast<TCHAR>(ch);
		return true;
	}
	else switch ( ch )
	{
		// Special character input received
		case KEY_BACKSPACE:
			m_szNextChar = '\b';
			return true;
		case KEY_ENTER:
			m_szNextChar = '\n';
			return true;
		//case KEY_RESIZE:		// to-do: resize window
		//case KEY_MOUSE:		// to-do: handle mouse event
		default:
			return false;
	}
#else
	// Check if input is waiting
	fd_set consoleFds;
	FD_ZERO(&consoleFds);
	FD_SET(STDIN_FILENO, &consoleFds);

	timeval tvTimeout;
	tvTimeout.tv_sec = 0;
	tvTimeout.tv_usec = 1;

	if ( select(1, &consoleFds, 0, 0, &tvTimeout) <= 0 )
		return false;

	// Get next character
	int ch = fgetc(stdin);
	if ( (ch < 0) || (ch > 255) )
		return false;

	// Map backspace
	if ( ch == m_terminal.c_cc[VERASE] )
		ch = '\b';

	// Echo to console
	fputc(ch, stdout);
	fflush(stdout);

	m_szNextChar = static_cast<TCHAR>(ch);
	return (m_szNextChar != '\0');
#endif
}

TCHAR CUnixTerminal::read()
{
	ADDTOCALLSTACK("CUnixTerminal::read");
	TCHAR ch = m_szNextChar;
	m_szNextChar = '\0';
	return ch;
}

void CUnixTerminal::setColor(COLOR_TYPE color)
{
	if ( !m_fColorEnabled )
		return;
	if ( (color < COL_DEFAULT) || (color >= COL_QTY) )
		color = COL_DEFAULT;

#ifdef _USECURSES
	wattrset(m_pWindow, COLOR_PAIR(color));
#else
	if ( color == COL_DEFAULT )
		fprintf(stdout, "\033[0m");
	else
		fprintf(stdout, "\033[0;%dm", 30 + static_cast<int>(color));
#endif
}

void CUnixTerminal::print(LPCTSTR pszText)
{
	ADDTOCALLSTACK("CUnixTerminal::print");
	ASSERT(pszText);
#ifdef _USECURSES
	waddstr(m_pWindow, pszText);
	wrefresh(m_pWindow);
#else
	fputs(pszText, stdout);
#endif
}

void CUnixTerminal::prepare()
{
	ADDTOCALLSTACK("CUnixTerminal::prepare");
	ASSERT(!m_fPrepared);

#ifdef _USECURSES
	initscr();	// init screen
	cbreak();	// read one character at a time
	echo();		// echo input

	// Create a window (same size as terminal)
	int lines, columns;
	getmaxyx(stdscr, lines, columns);
	m_pWindow = newwin(lines, columns, 0, 0);

	keypad(m_pWindow, TRUE);		// process special chars
	nodelay(m_pWindow, TRUE);		// non-blocking input
	scrollok(m_pWindow, TRUE);		// allow scrolling
	refresh();						// draw screen

	// Initialize colors, and map enums to color codes
	m_fColorEnabled = has_colors();
	if ( m_fColorEnabled )
	{
		start_color();
		init_pair(COL_RED, COLOR_RED, COLOR_BLACK);
		init_pair(COL_GREEN, COLOR_GREEN, COLOR_BLACK);
		init_pair(COL_YELLOW, COLOR_YELLOW, COLOR_BLACK);
		init_pair(COL_BLUE, COLOR_BLUE, COLOR_BLACK);
		init_pair(COL_CYAN, COLOR_CYAN, COLOR_BLACK);
		init_pair(COL_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(COL_WHITE, COLOR_WHITE, COLOR_BLACK);
	}
#else
	// Save existing attributes
	if ( tcgetattr(STDIN_FILENO, &m_terminal) != 0 )
		throw CGrayError(LOGL_WARN, 0, "failed to get terminal attributes");

	// Set new terminal attributes
	termios term_caps = m_terminal;
	term_caps.c_lflag &= ~(ICANON|ECHO);
	term_caps.c_cc[VMIN] = 1;

	if ( tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) != 0 )
		throw CGrayError(LOGL_WARN, 0, "failed to set terminal attributes");

	setbuf(stdin, NULL);
	m_fColorEnabled = true;		// assume every modern terminal support colors
#endif

	m_szNextChar = '\0';
	m_fPrepared = true;
}

void CUnixTerminal::restore()
{
	ADDTOCALLSTACK("CUnixTerminal::restore");
	EXC_TRY("Restore");
	ASSERT(m_fPrepared);

#ifdef _USECURSES
	endwin();	// clean up
	m_pWindow = NULL;
	m_szNextChar = '\0';
#else
	// Restore original terminal state
	if ( tcsetattr(STDIN_FILENO, TCSANOW, &m_terminal) != 0 )
		throw CGrayError(LOGL_WARN, 0, "failed to restore terminal attributes");
#endif

	m_fPrepared = false;
	EXC_CATCH;
}

#endif	// _WIN32
