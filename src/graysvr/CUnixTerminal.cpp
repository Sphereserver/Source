#ifndef _WIN32
#include "graysvr.h"	// predef header
#include "CUnixTerminal.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

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
	if ( (ch == EOF) || (ch > 255) )
		return false;

	// Map backspace
	if ( ch == m_terminal.c_cc[VERASE] )
		ch = '\b';

	// Echo to console
	fputc(ch, stdout);
	fflush(stdout);

	m_szNextChar = static_cast<TCHAR>(ch);
	return (m_szNextChar != '\0');
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
	if ( (color < COL_DEFAULT) || (color >= COL_QTY) )
		color = COL_DEFAULT;

	if ( color == COL_DEFAULT )
		fprintf(stdout, "\033[0m");
	else
		fprintf(stdout, "\033[0;%dm", 30 + static_cast<int>(color));
}

void CUnixTerminal::print(LPCTSTR pszText)
{
	ADDTOCALLSTACK("CUnixTerminal::print");
	ASSERT(pszText);
	fputs(pszText, stdout);
}

void CUnixTerminal::prepare()
{
	ADDTOCALLSTACK("CUnixTerminal::prepare");
	ASSERT(!m_fPrepared);

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
	m_szNextChar = '\0';
	m_fPrepared = true;
}

void CUnixTerminal::restore()
{
	ADDTOCALLSTACK("CUnixTerminal::restore");
	EXC_TRY("Restore");
	ASSERT(m_fPrepared);

	// Restore original terminal state
	if ( tcsetattr(STDIN_FILENO, TCSANOW, &m_terminal) != 0 )
		throw CGrayError(LOGL_WARN, 0, "failed to restore terminal attributes");

	m_fPrepared = false;
	EXC_CATCH;
}

#endif	// _WIN32
