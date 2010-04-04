#include "../common/graycom.h"
#include "strings.h"
#include "threads.h"

void ProfileData::SetActive(int iSampleSec)
{
	ADDTOCALLSTACK("ProfileData::SetActive");

	memset(m_AverageTimes, 0, sizeof(m_AverageTimes));
	memset(m_CurrentTimes, 0, sizeof(m_CurrentTimes));
	memset(m_PreviousTimes, 0, sizeof(m_PreviousTimes));

	m_iActiveWindowSeconds = iSampleSec;
	m_iAverageCount		= 1;

	if (m_iActiveWindowSeconds == 0)
		return;

	LONGLONG llTicks;
	TIME_PROFILE_START;

	m_CurrentTime = llTicks;
	m_CurrentTask = PROFILE_OVERHEAD;
	m_TimeTotal = 0;
}

void ProfileData::Start(PROFILE_TYPE id)
{
	ADDTOCALLSTACK("ProfileData::Start");
	if (( id < 0 ) || ( id >= PROFILE_TIME_QTY ) || !m_iActiveWindowSeconds )
		return;

	// Stop prev task.
	if ( m_TimeTotal >= llTimeProfileFrequency * m_iActiveWindowSeconds )
	{
		for ( int i = 0; i < PROFILE_QTY; i++ )
		{
			if ( m_iAverageCount < 4 )
			{
				memcpy( m_AverageTimes, m_CurrentTimes, sizeof( m_AverageTimes ));
			}
			else
			{
				if ( m_PreviousTimes[i].m_Time > llTimeProfileFrequency )
					m_PreviousTimes[i].m_Time = llTimeProfileFrequency;

				m_AverageTimes[i].m_Time	= (((m_AverageTimes[i].m_Time * 90) + (m_PreviousTimes[i].m_Time * 10)) / 100);
				m_AverageTimes[i].m_iCount	= (((m_AverageTimes[i].m_iCount * 95) + (m_PreviousTimes[i].m_iCount * 10)) / 100);
			}
		}

		++m_iAverageCount;

		memcpy( m_PreviousTimes, m_CurrentTimes, sizeof( m_PreviousTimes ));
		memset( m_CurrentTimes, 0, sizeof( m_CurrentTimes ));
		m_TimeTotal = 0;
	}

	// Get the current precise time.
	LONGLONG llTicks;
	TIME_PROFILE_START;

	// accumulate the time for this task.
	LONGLONG Diff = ( llTicks - m_CurrentTime );
	m_TimeTotal += Diff;
	m_CurrentTimes[m_CurrentTask].m_Time += Diff;
	m_CurrentTimes[m_CurrentTask].m_iCount ++;

	// We are now on to the new task.
	m_CurrentTime = llTicks;
	m_CurrentTask = id;
}

void ProfileData::Count(PROFILE_TYPE id, DWORD dwVal)
{
	ADDTOCALLSTACK("ProfileData::Count");

	ASSERT( id >= PROFILE_TIME_QTY && id < PROFILE_QTY );
	m_CurrentTimes[id].m_Time += dwVal;
	m_CurrentTimes[id].m_iCount ++;
}

bool ProfileData::HasData(PROFILE_TYPE id)
{
	ADDTOCALLSTACK("ProfileData::HasData");
	if (id < 0 || id > PROFILE_QTY)
		return false;

	if (id < PROFILE_QTY)
		return m_CurrentTimes[id].m_iCount > 0;

	// check all profiles
	for (int i = PROFILE_OVERHEAD; i < PROFILE_QTY; i++)
	{
		if (HasData((PROFILE_TYPE) i))
			return true;
	}

	return false;
}

PROFILE_TYPE ProfileData::GetCurrentTask() const
{
	return m_CurrentTask;
}

LPCTSTR ProfileData::GetName(PROFILE_TYPE id) const
{
	static LPCTSTR const sm_pszProfileName[PROFILE_QTY] =
	{
		"IDLE",
		"OVERHEAD",
		"NETWORK_RX",
		"CLIENTS",
		"NETWORK_TX",
		"CHARS",
		"ITEMS",
		"MAP",
		"NPC_AI",
		"SCRIPTS",
		"DATA_TX",
		"DATA_RX",
	};

	return (( id >= 0 ) && ( id < PROFILE_QTY )) ? sm_pszProfileName[id] : "";
}

LPCTSTR ProfileData::GetDescription(PROFILE_TYPE id) const
{
	ADDTOCALLSTACK("ProfileData::GetDesc");
	TCHAR	*pszTmp = Str_GetTemp();
	int		iCount	= m_PreviousTimes[id].m_iCount;

	if ( id >= PROFILE_TIME_QTY )
	{
		sprintf(pszTmp, "%i (avg: %i) bytes", (int) m_PreviousTimes[id].m_Time, m_AverageTimes[id].m_Time);
	}
	else
	{
		sprintf( pszTmp, "%3i.%04is  avg: %3i.%04is  [samples:%6i  avg:%6i ]  runtime: %is",
			(int)( m_PreviousTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int)((( m_PreviousTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			(int) ( m_AverageTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int) ((( m_AverageTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			iCount,
			(int) m_AverageTimes[id].m_iCount,
			m_iAverageCount );
	}

	return pszTmp;
}

//
// ProfileTask
//
ProfileTask::ProfileTask(PROFILE_TYPE id) : m_context(NULL), m_previousTask(PROFILE_OVERHEAD)
{
	m_context = (AbstractSphereThread *)ThreadHolder::current();
	if (m_context != NULL)
	{
		m_previousTask = m_context->m_profile.GetCurrentTask();
		m_context->m_profile.Start(id);
	}
}

ProfileTask::~ProfileTask(void)
{
	if (m_context != NULL)
		m_context->m_profile.Start(m_previousTask);
}
