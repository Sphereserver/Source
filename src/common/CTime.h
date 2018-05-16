#ifndef _INC_CTIME_H
#define _INC_CTIME_H

#ifndef _WIN32
	ULONGLONG GetTickCount64();
#endif
#undef GetCurrentTime

class CGTime
{
	// Get time stamp in the real world (based on struct tm)
	// Similar to MFC CTime and CTimeSpan or COleDateTime
public:
	static const char *m_sClassName;

	CGTime()
	{
		m_time = 0;
	}
	CGTime(time_t time)
	{
		m_time = time;
	}
	CGTime(int iYear, int iMonth, int iDay, int iHour, int iMin, int iSec, int iDST = -1);

private:
	time_t m_time;

public:
	static CGTime GetCurrentTime();
	struct tm *GetLocalTm() const;
	LPCTSTR Format(LPCTSTR pszFormat) const;
	LPCTSTR FormatGmt(LPCTSTR pszFormat) const;
	bool Read(TCHAR *pszVal);

	void Init()
	{
		m_time = -1;
	}

	time_t GetTime() const
	{
		return m_time;
	}
	bool IsTimeValid() const
	{
		return (m_time && (m_time != -1));
	}

	int GetYear() const
	{
		return GetLocalTm()->tm_year + 1900;
	}
	int GetMonth() const
	{
		return GetLocalTm()->tm_mon + 1;
	}
	int GetDay() const
	{
		return GetLocalTm()->tm_mday;
	}
	int GetHour() const
	{
		return GetLocalTm()->tm_hour;
	}
	int GetMinute() const
	{
		return GetLocalTm()->tm_min;
	}
	int GetDaysTotal() const
	{
		// Needs to be more consistent than accurate, just for compare
		return (GetYear() * 365) + (GetMonth() * 30) + GetDay();
	}

public:
	const CGTime &operator=(const CGTime &timeSrc)
	{
		m_time = timeSrc.m_time;
		return *this;
	}
	const CGTime &operator=(time_t time)
	{
		m_time = time;
		return *this;
	}

	bool operator<=(time_t time) const
	{
		return (m_time <= time);
	}
	bool operator==(time_t time) const
	{
		return (m_time == time);
	}
	bool operator!=(time_t time) const
	{
		return (m_time != time);
	}
};

#endif // _INC_CTIME_H
