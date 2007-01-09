#include "common.h"
#include "../graysvr.h"

#ifndef _WIN32
	#include <termios.h>
#endif

char g_tmpBuf[10][1024];
long g_tmpBufIdx = 0;

/***************************************************************************
 *
 *
 *	class CLog					logging possibitilites support
 *
 *
 ***************************************************************************/

const char *CLog::m_sClassName = "CLog";
CLog g_Log;

CLog::CLog()
{
	m_logmask = 0x0fff0;
	m_days = 0;
	m_dir[0] = 0;
	m_file = NULL;
	m_lastMessage[0] = 0;
	m_lastMsgCount = 0;
	m_gmlog = NULL;
}

CLog::~CLog()
{
	Close();
}

void CLog::Close()
{
	if ( m_file )
	{
		fclose(m_file);
		m_file = NULL;
		m_days = 0;
	}
	if ( m_gmlog )
	{
		fclose(m_gmlog);
		m_gmlog = NULL;
	}
}

DWORD CLog::LogMask()
{
	return (m_logmask & 0x0fff0);
}

void CLog::LogMask(DWORD mask)
{
	m_logmask &= ~0x0fff0;
	m_logmask |= ( mask & 0x0fff0 );
}

char *CLog::LogDir()
{
	return m_dir;
}

void CLog::LogDir(char *dir)
{
	strcpylen(m_dir, dir, sizeof(m_dir)-1);

	// guarantee having trailing slash for directory
	char *z = &m_dir[strlen(m_dir)-1];
	if (( *z != '\\' ) && ( *dir != '/' ))
	{
		z++;
		*z = '/';
		*(z+1) = 0;
	}

	// cause next command to be written in a new dir
	m_days = 0;
}

void _cdecl CLog::Event(DWORD mask, LPCTSTR format, ... )
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(mask, format, vargs);
	va_end(vargs);
}

void _cdecl CLog::Init(LPCTSTR format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(LOGM_INIT, format, vargs);
	va_end(vargs);
}

#ifdef _DEBUG
void _cdecl CLog::Event(LPCTSTR format, ... )
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(LOGL_EVENT, format, vargs);
	va_end(vargs);
}
#endif

void _cdecl CLog::Debug(LPCTSTR format, ... )
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(LOGM_NOCONTEXT|LOGM_DEBUG, format, vargs);
	va_end(vargs);
}

void _cdecl CLog::Error(LPCTSTR format, ... )
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(LOGL_ERROR, format, vargs);
	va_end(vargs);
}

void _cdecl CLog::Warn(LPCTSTR format, ... )
{
	va_list vargs;
	va_start(vargs, format);
	VEvent(LOGL_WARN, format, vargs);
	va_end(vargs);
}

void CLog::VEvent(DWORD mask, LPCTSTR format, va_list args)
{
	if ( format && *format )
	{
		TEMPSTRING(z);
		size_t len = _vsnprintf(z, (SCRIPT_MAX_LINE_LEN - 1), format, args);
		if ( !len )
			strncpy(z, format, (SCRIPT_MAX_LINE_LEN - 1));
		EventStr(mask, z);
	}
}

void CLog::EventStr(DWORD mask, LPCTSTR message)
{
	if (( mask & 0x0fff0 ) && !(( mask & 0x0fff0 ) & ( m_logmask & 0x0fff0 )) )
		return;
	if ( !message || !*message )
		return;
	if ( !strcmp(message, m_lastMessage) )
	{
		m_lastMsgCount++;
		return;
	}

	static CThreadSafe lock;
	lock.Lock(true);
	try
	{
		//	messaging anti-flood control
		if ( m_lastMsgCount )
		{
			sprintf(m_buf, "Last message repeated %d times\n", m_lastMsgCount);
			g_Serv.PrintStr(m_buf);
		}
		if ( strlen(message) < sizeof(m_lastMessage) )
		{
			m_lastMsgCount = 0;
			strcpy(m_lastMessage, message);
		}

		char timeString[16];
		time_t curtime = time(NULL);
		if ( curtime/(60*60*24) > m_days )
		{
			Close();

			sprintf(m_buf, "%ssphere%s.log", m_dir, formattime(timeString, localtime(&curtime), TIME_DAY));
			m_file = fopen(m_buf, "at");
			m_days = curtime/(60*60*24);

			if ( m_file )
				FILE_SETNOCACHE(m_file);
		}
		formattime(timeString, localtime(&curtime), TIME_TIME);
		strcat(timeString, ":");

		LPCTSTR label = NULL;
		switch ( mask & 0x07 )
		{
		case LOGL_FATAL:	// fatal error !
			label = "FATAL:";
			break;
		case LOGL_CRIT:	// critical.
			label = "CRITICAL:";
			break;
		case LOGL_ERROR:	// non-fatal errors.
			label = "ERROR:";
			break;
		case LOGL_WARN:
			label = "WARNING:";
			break;
		}
		if ( !label && ( mask & LOGM_DEBUG ) && !( mask & LOGM_INIT ))
			label = "DEBUG:";

		TCHAR	context[_MAX_PATH];
		context[0] = 0;
		if ( !( mask&LOGM_NOCONTEXT ) )
		{
			CThread *thread = CThread::Thread();

			if ( thread )
			{
				const CScript *cont = thread->ScriptContext();
				if ( cont )
					sprintf(context, "(%s,%d)", cont->GetFileTitle(), cont->GetContext().m_iLineNum);
			}
		}

		//
		//	output to screen
		//

		if ( !(mask & LOGM_INIT) && !g_Serv.IsLoading() )
		{
#ifdef _WIN32
			NTWindow_PostMsgColor(RGB(127,127,0));
#else
			g_Serv.PrintStr("\e[0;33m");
#endif
			g_Serv.PrintStr(timeString);
#ifdef _WIN32
			NTWindow_PostMsgColor(0);
#else
			g_Serv.PrintStr("\e[0m");
#endif
		}

		if ( label )
		{
#ifdef _WIN32
			NTWindow_PostMsgColor(RGB(255,0,0));
#else
			g_Serv.PrintStr("\e[0;31m");
#endif
			g_Serv.PrintStr(label);
#ifdef _WIN32
			NTWindow_PostMsgColor(0);
#else
			g_Serv.PrintStr("\e[0m");
#endif
		}

		if ( context[0] )
		{
#ifdef _WIN32
			NTWindow_PostMsgColor(RGB(0,127,255));
#else
			g_Serv.PrintStr("\e[1;36m");
#endif
			g_Serv.PrintStr(context);
#ifdef _WIN32
			NTWindow_PostMsgColor(0);
#else
			g_Serv.PrintStr("\e[0m");
#endif
		}

		g_Serv.PrintStr(message);

		//
		//	output to log file
		//
		if ( m_file )
		{
			fputs(timeString, m_file);
			if ( label )
				fputs(label, m_file);
			if ( context[0] )
				fputs(context, m_file);
			fputs(message, m_file);
			FILE_FLUSH(m_file);
		}
	}
	catch (...)
	{
		// could not open log file or had exception inside.
		// there is nothing we can do with this here, so just ignore
	}
	lock.Release();
}

void _cdecl CLog::Catch(CError *pErr, LPCTSTR CatchContext, ...)
{
	try
	{
		char		z[1024];
		LOGL_TYPE	severity;
		int			len = 0;

		if ( pErr )
		{
			severity = (LOGL_TYPE)pErr->m_severity;
			pErr->GetErrorMessage(z);
		}
		else
		{
			severity = LOGL_CRIT;
			strcpy(z, "Exception");
		}
		len = strlen(z);

		strcat(z, ", in ");
		va_list	vargs;
		va_start(vargs, CatchContext);
		strcat(z, CatchContext);
		strcat(z, "\n");

		VEvent(severity, z, vargs);
		va_end(vargs);
	}
	catch (...)
	{
	}
}

void CLog::WriteRaw(LPCTSTR line)
{
	if ( m_file )
		fputs(line, m_file);
}

void _cdecl CLog::WriteRawF(LPCTSTR format, ...)
{
	va_list vargs;
	va_start(vargs, format);
	size_t len = _vsnprintf(m_buf, (SCRIPT_MAX_LINE_LEN - 1), format, vargs);
	if ( !len )
		strncpy(m_buf, format, (SCRIPT_MAX_LINE_LEN - 1));
	va_end(vargs);

	WriteRaw(m_buf);
}

void CLog::gmLog(char *text, CChar *source)
{
	if ( !source || !source->m_pPlayer || ( source->GetPrivLevel() < PLEVEL_Counsel ))
		return;
	if ( !text || !*text )
		return;

	if ( !m_gmlog )
	{
		char fileName[256];
		sprintf(fileName, "%sgm.log", m_dir);
		m_gmlog = fopen(fileName, "at");
		if ( !m_gmlog )
			return;

		FILE_SETNOCACHE(m_file);
	}
	char timeString[32];
	time_t curtime = time(NULL);
	TEMPSTRING(z);

	fputs(formattime(timeString, localtime(&curtime), TIME_LONG), m_gmlog);
	sprintf(z, ": %s [level='%d',account='%s',char='%s',UID=0%x]\n", text,
		source->GetPrivLevel(), source->m_pPlayer->GetAccount()->GetName(), source->GetName(), source->uid());
	fputs(z, m_gmlog);
	FILE_FLUSH(m_file);
}

/***************************************************************************
 *
 *
 *	common functions
 *
 *
 ***************************************************************************/

char *formattime(char *storage, struct tm *time, int mask)
{
	static const char *months[]	= {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static const char *dayow[]	= {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

	if ( mask & TIME_LONG )
	{
		if ( mask & TIME_GMT )
		{
			sprintf(storage, "%s, %02d %s %d %02d:%02d:%02d GMT",
				dayow[time->tm_wday], time->tm_mday, months[time->tm_mon], time->tm_year+1900, time->tm_hour, time->tm_min, time->tm_sec);
		}
		else
		{
			sprintf(storage, "%d/%02d/%02d %02d:%02d:%02d",
				time->tm_year+1900, time->tm_mon+1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
		}
	}
	else if (( mask & TIME_SHORT ) == TIME_SHORT )
	{
		sprintf(storage, "%d-%02d-%02d %02d:%02d", time->tm_year+1900, time->tm_mon+1, time->tm_mday, time->tm_hour, time->tm_min);
	}
	else if ( mask & TIME_DAY )
	{
		sprintf(storage, "%d-%02d-%02d", time->tm_year+1900, time->tm_mon+1, time->tm_mday);
	}
	else if ( mask & TIME_TIME )
	{
		sprintf(storage, "%02d:%02d", time->tm_hour, time->tm_min);
	}
	else if ( mask & TIME_EXACT )
	{
		sprintf(storage, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	}
	return storage;
}

struct T_TRIGGERS
{
	char	m_name[48];
	long	m_used;
};

vector<T_TRIGGERS> g_triggers;

bool IsTrigUsed(E_TRIGGERS id)
{
	return (( id < g_triggers.size() ) && g_triggers[id].m_used );
}

bool IsTrigUsed(const char *name)
{
	vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( !strcmpi(it->m_name, name) )
			return it->m_used;
	}
	return false;
}

void TriglistInit()
{
	T_TRIGGERS	trig;
	g_triggers.clear();

#define ADD(_a_,_b_)	strcpy(trig.m_name, _b_); trig.m_used = 0; g_triggers.push_back(trig);
#include "../tables/triggers.tbl"
#undef ADD
}

void TriglistClear()
{
	vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		(*it).m_used = 0;
	}
}

void TriglistAdd(E_TRIGGERS id)
{
	if ( g_triggers.size() )
		g_triggers[id].m_used++;
}

void TriglistAdd(const char *name)
{
	vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( !strcmpi(it->m_name, name) )
		{
			it->m_used++;
			break;
		}
	}
}

void Triglist(long &total, long &used)
{
	total = used = 0;
	vector<T_TRIGGERS>::iterator it;
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		total++;
		if ( it->m_used )
			used++;
	}
}

void TriglistPrint()
{
	vector<T_TRIGGERS>::iterator it;
	TEMPSTRING(z);
	for ( it = g_triggers.begin(); it != g_triggers.end(); it++ )
	{
		if ( it->m_used )
		{
			g_Serv.SysMessagef("Trigger %s : used %d time%s.\n", it->m_name, it->m_used, (it->m_used > 1) ? "s" : "");
		} 
		else
		{
			g_Serv.SysMessagef("Trigger %s : NOT used.\n", it->m_name);
		}
	}
}

#ifndef _WIN32
// This is used to restore the original flags on exit
void resetNonBlockingIo()
{
	termios term_caps;

	if ( tcgetattr(STDIN_FILENO, &term_caps) < 0 ) return;
	term_caps.c_lflag |= ICANON;
	if( tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0 ) return;
}

void setNonBlockingIo()
{
	termios term_caps;

	if( tcgetattr(STDIN_FILENO, &term_caps) < 0 )
	{
		printf("ERROR: Could not get the termcap settings for this terminal.\n");
		return;
	}

	term_caps.c_lflag &= ~ICANON;
	if( tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0 )
	{
		printf("ERROR: Could not set the termcap settings for this terminal.\n");
		return;
	}
	setbuf(stdin, NULL);
	atexit(resetNonBlockingIo);
}
#endif

int ATOI(const char *str)
{
	int val = 0;

	if (( *str == '0' ) || ( *str == 'x' ) || ( *str == 'X' ))		//	HEX
	{
		while ( *str )
		{
			if (( *str >= '0' ) && ( *str <= '9' ))
				val = ( val * 0x10 ) + ( *str - '0' );
			else
			{
				char ch = tolower(*str);
				if (( ch >= 'a' ) && ( ch <= 'f' ))
					val = ( val * 0x10 ) + ( ch - 'a' + 10 );
				else
					break;
			}
			str++;
		}
	}
	else															//	DEC
	{
		while (( *str >= '0' ) && ( *str <= '9' ))
		{
			val = (val * 10) + ( *str - '0' );
			str++;
		}
	}
	return val;
}

char *ITOA(int i, char *str, int radix)
{
	return LTOA(i, str, radix);
}

char *LTOA(long l, char *str, long radix)
{
	char *z = str;
	bool bMinus = ( l < 0 );

	if ( !l )
		*z++ = '0';
	else if ( bMinus )
		l *= -1;

	while ( l )
	{
		long mod = l % radix;
		l /= radix;

		if ( mod < 10 )				//	first 10 encoded as digits
			*z = mod + '0';
		else						//	the last ones encoded as letters
			*z = (mod - 10) + 'a';
		z++;
	}

	if ( bMinus )
		*z++ = '-';

	*z = 0;
	return STRREV(str);
}

char *STRREV(char *str)
{
	char *end = str, *orig = str;
	char temp;

	while ( *end )
		end++;
	end--;

	while ( str < end )
	{
		temp = *end;
		*end-- = *str;
		*str++ = temp;
	}
	return orig;
}
