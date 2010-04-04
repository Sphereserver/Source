#ifndef PROFILEDATA_H
#define PROFILEDATA_H
#pragma once

enum PROFILE_TYPE
{
	PROFILE_IDLE,		// Wait for stuff.
	PROFILE_OVERHEAD,	// In between stuff.
	PROFILE_NETWORK_RX,	// Just get client info and monitor new client requests. No processing.
	PROFILE_CLIENTS,	// Client processing.
	PROFILE_NETWORK_TX,	// sending network data
	PROFILE_CHARS,		// ticking characters
	PROFILE_ITEMS,		// ticking items
	PROFILE_MAP,		// reading map data
	PROFILE_NPC_AI,		// processing npc ai
	PROFILE_SCRIPTS,	// running scripts
	PROFILE_TIME_QTY,

	// Qty of bytes. Not Time.
	PROFILE_DATA_TX = PROFILE_TIME_QTY, // network bytes sent
	PROFILE_DATA_RX,					// network bytes received

	PROFILE_QTY,
};

class ProfileData
{
protected:
	struct ProfileDataRec
	{
		LONGLONG m_Time;	// accumulated time in msec.
		int m_iCount;		// how many passes made into this.
	};

	ProfileDataRec m_AverageTimes[PROFILE_QTY];
	ProfileDataRec m_PreviousTimes[PROFILE_QTY];
	ProfileDataRec m_CurrentTimes[PROFILE_QTY];

	int m_iActiveWindowSeconds;	// The sample window size in seconds. 0=off
	int	m_iAverageCount;

	LONGLONG m_TimeTotal;	// Average this over a total time period.

	// Store the last time start time.
	PROFILE_TYPE  m_CurrentTask;	// What task are we currently processing ?
	LONGLONG m_CurrentTime;			// QueryPerformanceCount()

public:
	ProfileData()
	{
		SetActive(10);	// default to 10 sec window.
		Start(PROFILE_IDLE);
	}

	bool IsActive() const { return( m_iActiveWindowSeconds > 0 ? true : false ); }
	int GetActiveWindow() const { return m_iActiveWindowSeconds; }

	void SetActive(int iSampleSec);
	void Start(PROFILE_TYPE id);
	void Count(PROFILE_TYPE id, DWORD dwVal);
	bool HasData(PROFILE_TYPE id = PROFILE_QTY);

	PROFILE_TYPE GetCurrentTask() const;
	LPCTSTR GetName(PROFILE_TYPE id) const;
	LPCTSTR GetDescription(PROFILE_TYPE id) const;
};

#define CurrentProfileData ((AbstractSphereThread *)ThreadHolder::current())->m_profile

class AbstractSphereThread;

class ProfileTask
{
private:
	AbstractSphereThread* m_context;
	PROFILE_TYPE m_previousTask;

public:
	ProfileTask(PROFILE_TYPE id);
	~ProfileTask(void);
};

#endif // PROFILEDATA_H
