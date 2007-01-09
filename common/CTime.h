//
// ctime.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CTIME_H
#define _INC_CTIME_H

#include <time.h>

#ifndef _WIN32
LONGLONG GetTickCount();
#endif

class CGTime	// similar to the MFC CTime and CTimeSpan or COleDateTime
{
	// Get time stamp in the real world. based on struct tm
#undef GetCurrentTime
private:
	time_t m_time;
public:
	static const char *m_sClassName;

	// Constructors
	static CGTime GetCurrentTime();

	CGTime()
	{ m_time = 0; }
	CGTime(time_t time)
	{ m_time = time; }
	CGTime(const CGTime& timeSrc)
	{ m_time = timeSrc.m_time; }

	CGTime( struct tm time );
	CGTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
		int nDST = -1);

	const CGTime& operator=(const CGTime& timeSrc)
	{ m_time = timeSrc.m_time; return *this; }
	const CGTime& operator=(time_t t)
	{ m_time = t; return *this; }

	bool operator<=( time_t t ) const
	{
		return( m_time <= t );
	}
	bool operator==( time_t t ) const
	{
		return( m_time == t );
	}
	bool operator!=( time_t t ) const
	{
		return( m_time != t );
	}

	time_t GetTime() const
	{ return m_time; }

	// Attributes
	struct tm* GetLocalTm(struct tm* ptm = NULL) const;

	int GetYear() const
	{ return (GetLocalTm(NULL)->tm_year) + 1900; }
	int GetMonth() const       // month of year (1 = Jan)
	{ return GetLocalTm(NULL)->tm_mon + 1; }
	int GetDay() const         // day of month
	{ return GetLocalTm(NULL)->tm_mday; }
	int GetHour() const
	{ return GetLocalTm(NULL)->tm_hour; }
	int GetMinute() const
	{ return GetLocalTm(NULL)->tm_min; }

	// Operations
	LPCTSTR Format(LPCTSTR pszFormat) const;
	LPCTSTR FormatGmt(LPCTSTR pszFormat) const;

	// non CTime operations.
	bool Read( TCHAR * pVal );
	void Init()
	{
		m_time = -1;
	}
	bool IsTimeValid() const
	{
		return(( m_time && m_time != -1 ) ? true : false );
	}
	int GetDaysTotal() const
	{
		// Needs to be more consistant than accurate. just for compares.
		return(( GetYear() * 366) + (GetMonth()*31) + GetDay() );
	}
};

#endif // _INC_CTIME_H
