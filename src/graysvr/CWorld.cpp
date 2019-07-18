#include "graysvr.h"	// predef header
#include "../network/send.h"

#ifndef _WIN32
	#include <time.h>
#endif

LPCTSTR GetReasonForGarbageCode(int iCode = -1)
{
	switch ( iCode )
	{
		case -1:
		default:
			return "Unknown";

		case 0x1102:
			return "Disconnected char not acting as such";

		case 0x1103:
		case 0x1203:
			return "Ridden NPC not acting as such";

		case 0x1104:
		case 0x1204:
			return "Ridden NPC without a mount item";

		case 0x1105:
			return "Ridden NPC with a mislinked mount item";

		case 0x1106:
			return "Disconnected NPC neither dead nor ridden";

		case 0x1107:
		case 0x1205:
			return "In game char that is neither a player nor an NPC";

		case 0x1108:
			return "Char not on a valid position";

		case 0x2102:
			return "Item without ITEMDEF";

		case 0x2103:
			return "Item ITEMDEF with ID = 0";

		case 0x2104:
			return "Disconnected item";

		case 0x2105:
			return "Item not on a valid position";

		case 0x2106:
			return "Item flagged as being in a container but it isn't";

		case 0x2202:
			return "Item flagged as equipped but it isn't";

		case 0x2205:
			return "Mislinked item";

		case 0x2206:
			return "GM Robe / Deathshroud not on a char";

		case 0x2207:
			return "Deathshroud not on a dead char";

		case 0x2208:
			return "GM Robe on a char without privilege";

		case 0x2220:
			return "Trade window memory not equipped in the correct layer or equipped on disconnected char";

		case 0x2221:
			return "Client linger memory not equipped in the correct layer";

		case 0x2226:
			return "Mount memory not equipped in the correct layer";

		case 0x2227:
		case 0x2228:
			return "Hair/Beard item not equipped / not in a corpse / not in a vendor box";

		case 0x2229:
			return "Game piece not in a game board";

		case 0x2230:
			return "Item equipped in the trade window layer but it isn't a trade window";

		case 0x2231:
			return "Item equipped in the memory layer but it isn't a memory";

		case 0x2233:
			return "Item equipped in the mount memory layer but it isn't a mount memory";

		case 0x2234:
			return "Item equipped in the client linger layer but it isn't a client linger memory";

		case 0x2235:
			return "Item equipped in the murder memory layer but it isn't a murder memory";

		case 0x2236:
			return "Item flagged as decay but without timer set";

		case 0x3101:
			return "Object is totaly lost, no parent exists";

		case 0x3102:
			return "Object was deleted or UID is incorrect";

		case 0x3201:
			return "Object not correctly loaded by server (UID conflict)";

		case 0x3202:
			return "Object not placed in the world";

		case 0x2222:
		case 0x4222:
			return "Memory not equipped / not in the memory layer / without color";

		case 0x4223:
			return "Memory not on a char";

		case 0x4224:
			return "Stone/Guild memory mislinked";

		case 0x4225:
			return "Stone/Guild memory linked to the wrong stone";

		case 0xFFFF:
			return "Bad memory allocation";
	}
}

void ReportGarbageCollection(CObjBase *pObj, int iResultCode)
{
	ASSERT(pObj);
	DEBUG_ERR(("UID=0%lx, id=0%hx '%s', Invalid code=%0x (%s)\n", static_cast<DWORD>(pObj->GetUID()), pObj->GetBaseID(), pObj->GetName(), iResultCode, GetReasonForGarbageCode(iResultCode)));
}

///////////////////////////////////////////////////////////
// CTimedFunctionHandler

CTimedFunctionHandler::CTimedFunctionHandler()
{
	m_iCurTick = 0;
	m_fBeingProcessed = false;
}

void CTimedFunctionHandler::OnTick()
{
	ADDTOCALLSTACK("CTimedFunctionHandler::OnTick");
	m_fBeingProcessed = true;

	++m_iCurTick;
	if ( m_iCurTick >= TICK_PER_SEC )
		m_iCurTick = 0;

	int iTick = m_iCurTick;
	std::vector<TimedFunction *>::iterator it;
	ProfileTask scriptsTask(PROFILE_SCRIPTS);

	if ( m_timedFunctions[iTick].size() > 0 )
	{
		bool fEndLoop = false;
		for ( it = m_timedFunctions[iTick].begin(); (it != m_timedFunctions[iTick].end()) && !fEndLoop; )
		{
			TimedFunction *tf = *it;
			tf->elapsed--;
			if ( tf->elapsed <= 0 )
			{
				CScript s(tf->funcname);
				CObjBase *pObj = tf->uid.ObjFind();

				if ( pObj )		// just in case
				{
					CObjBaseTemplate *pObjTop = pObj->GetTopLevelObj();
					CTextConsole *pSrc = pObjTop->IsChar() ? dynamic_cast<CTextConsole *>(pObjTop) : &g_Serv;

					m_tFrecycled.push_back(tf);
					if ( m_timedFunctions[iTick].size() == 1 )
					{
						// vector::erase() crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
						m_timedFunctions[iTick].pop_back();
						fEndLoop = true;
					}
					else
						it = m_timedFunctions[iTick].erase(it);

					pObj->r_Verb(s, pSrc);
				}
				else
				{
					m_tFrecycled.push_back(tf);
					if ( m_timedFunctions[iTick].size() == 1 )
					{
						// vector::erase() crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
						m_timedFunctions[iTick].pop_back();
						fEndLoop = true;
					}
					else
						it = m_timedFunctions[iTick].erase(it);
				}
			}
			else
				++it;
		}
	}

	m_fBeingProcessed = false;

	while ( m_tFqueuedToBeAdded.size() > 0 )
	{
		TimedFunction *tf = m_tFqueuedToBeAdded.back();
		m_tFqueuedToBeAdded.pop_back();
		m_timedFunctions[iTick].push_back(tf);
	}
}

void CTimedFunctionHandler::Erase(CGrayUID uid)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Erase");
	for ( int iTick = 0; iTick < TICK_PER_SEC; ++iTick )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[iTick].begin(); it != m_timedFunctions[iTick].end(); )
		{
			TimedFunction *tf = *it;
			if ( tf->uid == uid )
			{
				m_tFrecycled.push_back(tf);
				if ( m_timedFunctions[iTick].size() == 1 )
				{
					// vector::erase() crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
					m_timedFunctions[iTick].pop_back();
					break;
				}
				else
					it = m_timedFunctions[iTick].erase(it);
			}
			else
				++it;
		}
	}
}

int CTimedFunctionHandler::IsTimer(CGrayUID uid, LPCTSTR pszFuncName)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::IsTimer");
	for ( int iTick = 0; iTick < TICK_PER_SEC; ++iTick )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[iTick].begin(); it != m_timedFunctions[iTick].end(); )
		{
			TimedFunction *tf = *it;
			if ( (tf->uid == uid) && !strcmpi(tf->funcname, pszFuncName) )
				return tf->elapsed;

			++it;
		}
	}
	return 0;
}

void CTimedFunctionHandler::Stop(CGrayUID uid, LPCTSTR pszFuncName)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Stop");
	for ( int iTick = 0; iTick < TICK_PER_SEC; ++iTick )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[iTick].begin(); it != m_timedFunctions[iTick].end(); )
		{
			TimedFunction *tf = *it;
			if ( (tf->uid == uid) && !strcmpi(tf->funcname, pszFuncName) )
			{
				m_tFrecycled.push_back(tf);
				if ( m_timedFunctions[iTick].size() == 1 )
				{
					// vector::erase() crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
					m_timedFunctions[iTick].pop_back();
					break;
				}
				else
					it = m_timedFunctions[iTick].erase(it);
			}
			else
				++it;
		}
	}
}

TRIGRET_TYPE CTimedFunctionHandler::Loop(LPCTSTR pszFuncName, int iLoopsMade, CScriptLineContext StartContext, CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs, CGString *sResult)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Loop");
	bool fEndLoop = false;
	for ( int iTick = 0; (iTick < TICK_PER_SEC) && !fEndLoop; ++iTick )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[iTick].begin(); (it != m_timedFunctions[iTick].end()) && !fEndLoop; )
		{
			++iLoopsMade;
			if ( g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes) )
			{
				g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", iLoopsMade);
				return TRIGRET_ENDIF;
			}

			TimedFunction *tf = *it;
			if ( !strcmpi(tf->funcname, pszFuncName) )
			{
				CObjBase *pObj = tf->uid.ObjFind();
				TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, sResult);
				if ( iRet == TRIGRET_BREAK )
				{
					fEndLoop = true;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				s.SeekContext(StartContext);
			}
			++it;
		}
	}
	return TRIGRET_ENDIF;
}

void CTimedFunctionHandler::Add(CGrayUID uid, int iNumSeconds, LPCTSTR pszFuncName)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Add");
	ASSERT(pszFuncName);
	ASSERT(strlen(pszFuncName) < 128);

	int iTick = m_iCurTick;

	TimedFunction *tf;
	if ( m_tFrecycled.size() > 0 )
	{
		tf = m_tFrecycled.back();
		m_tFrecycled.pop_back();
	}
	else
		tf = new TimedFunction;

	tf->uid = uid;
	tf->elapsed = iNumSeconds;
	strncpy(tf->funcname, pszFuncName, sizeof(tf->funcname) - 1);

	if ( m_fBeingProcessed )
		m_tFqueuedToBeAdded.push_back(tf);
	else
		m_timedFunctions[iTick].push_back(tf);
}

int CTimedFunctionHandler::Load(const char *pszName, bool fQuoted, const char *pszVal)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Load");
	UNREFERENCED_PARAMETER(fQuoted);
	static char tempBuffer[128];
	static TimedFunction *tf = NULL;

	if ( !pszName )
		return -1;

	if ( !strcmpi(pszName, "CurTick") )
	{
		if ( IsDigit(pszVal[0]) )
			m_iCurTick = ATOI(pszVal);
		else
			g_Log.Event(LOGL_ERROR, "Invalid NextTick line in save file: %s=%s\n", pszName, pszVal);
	}
	else if ( !strcmpi(pszName, "TimerFNumbers") )
	{
		TCHAR *ppArgs[4];
		strncpy(tempBuffer, pszVal, sizeof(tempBuffer) - 1);	// because pszVal is constant and Str_ParseCmds wants a non-constant string
		size_t iArgs = Str_ParseCmds(tempBuffer, ppArgs, COUNTOF(ppArgs), " ,\t");

		if ( iArgs == 3 )
		{
			if ( IsDigit(ppArgs[0][0]) && IsDigit(ppArgs[1][0]) && IsDigit(ppArgs[2][0]) )
			{
				bool fNew = false;
				if ( !tf )
				{
					tf = new TimedFunction;
					tf->funcname[0] = '\0';
					fNew = true;
				}
				tf->elapsed = ATOI(ppArgs[2]);
				tf->uid.SetPrivateUID(ATOI(ppArgs[1]));

				if ( !fNew )
				{
					m_timedFunctions[ATOI(ppArgs[0])].push_back(tf);
					tf = NULL;
				}
				else
					g_Log.Event(LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order\n", SPHERE_FILE);
			}
			else
				g_Log.Event(LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s\n", SPHERE_FILE, pszName, pszVal);
		}
	}
	else if ( !strcmpi(pszName, "TimerFCall") )
	{
		bool fNew = false;
		if ( !tf )
		{
			tf = new TimedFunction;
			fNew = true;
		}
		strncpy(tf->funcname, pszVal, sizeof(tf->funcname) - 1);

		if ( !fNew )
			g_Log.Event(LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order\n", SPHERE_FILE);
	}
	return 0;
}

void CTimedFunctionHandler::r_Write(CScript &s)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::r_Write");
	s.WriteKeyFormat("CurTick", "%d", m_iCurTick);
	for ( int iTick = 0; iTick < TICK_PER_SEC; ++iTick )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[iTick].begin(); it != m_timedFunctions[iTick].end(); ++it )
		{
			TimedFunction *tf = *it;
			if ( tf->uid.IsValidUID() )
			{
				s.WriteKeyFormat("TimerFCall", "%s", tf->funcname);
				s.WriteKeyFormat("TimerFNumbers", "%d,%lu,%d", iTick, tf->uid.GetObjUID(), tf->elapsed);
			}
		}
	}
}

///////////////////////////////////////////////////////////
// CWorldSearch

CWorldSearch::CWorldSearch(const CPointMap &pt, int iDist) : m_pt(pt), m_iDist(iDist)
{
	m_fAllShow = false;

	m_pObj = m_pObjNext = NULL;
	m_fInertToggle = false;

	m_pSectorBase = m_pSector = pt.GetSector();
	m_rectSector.SetRect(pt.m_x - iDist, pt.m_y - iDist, pt.m_x + iDist + 1, pt.m_y + iDist + 1, pt.m_map);
	m_iSectorCur = 0;
}

bool CWorldSearch::GetNextSector()
{
	ADDTOCALLSTACK("CWorldSearch::GetNextSector");
	// Move search into nearby CSector if needed
	if ( !m_iDist )
		return false;

	for (;;)
	{
		m_pSector = m_rectSector.GetSector(m_iSectorCur++);
		if ( !m_pSector )
			return false;
		if ( m_pSectorBase == m_pSector )
			continue;
		m_pObj = NULL;	// start at head of next sector
		return true;
	}
}

CItem *CWorldSearch::GetItem()
{
	ADDTOCALLSTACK("CWorldSearch::GetItem");
	for (;;)
	{
		if ( !m_pObj )
		{
			m_fInertToggle = false;
			m_pObj = static_cast<CObjBase *>(m_pSector->m_Items_Inert.GetHead());
		}
		else
			m_pObj = m_pObjNext;

		if ( !m_pObj )
		{
			if ( !m_fInertToggle )
			{
				m_fInertToggle = true;
				m_pObj = static_cast<CObjBase *>(m_pSector->m_Items_Timer.GetHead());
				if ( m_pObj )
					goto jumpover;
			}
			if ( GetNextSector() )
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( (m_fAllShow && (m_pt.GetDistBase(m_pObj->GetTopPoint()) <= m_iDist)) || (m_pt.GetDist(m_pObj->GetTopPoint()) <= m_iDist) )
			return static_cast<CItem *>(m_pObj);
	}
}

CChar *CWorldSearch::GetChar()
{
	ADDTOCALLSTACK("CWorldSearch::GetChar");
	for (;;)
	{
		if ( !m_pObj )
		{
			m_fInertToggle = false;
			m_pObj = static_cast<CObjBase *>(m_pSector->m_Chars_Active.GetHead());
		}
		else
			m_pObj = m_pObjNext;

		if ( !m_pObj )
		{
			if ( !m_fInertToggle && m_fAllShow )
			{
				m_fInertToggle = true;
				m_pObj = static_cast<CObjBase *>(m_pSector->m_Chars_Disconnect.GetHead());
				if ( m_pObj )
					goto jumpover;
			}
			if ( GetNextSector() )
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( (m_fAllShow && (m_pt.GetDistBase(m_pObj->GetTopPoint()) <= m_iDist)) || (m_pt.GetDist(m_pObj->GetTopPoint()) <= m_iDist) )
			return static_cast<CChar *>(m_pObj);
	}
}

///////////////////////////////////////////////////////////
// CWorldThread

CWorldThread::CWorldThread()
{
	m_fSaveParity = false;
	m_dwUIDIndexLast = 1;

	m_dwFreeUIDs = (DWORD *)calloc(FREE_UIDS_SIZE, sizeof(DWORD));
	m_dwFreeOffset = FREE_UIDS_SIZE;
}

CWorldThread::~CWorldThread()
{
	CloseAllUIDs();
}

void CWorldThread::CloseAllUIDs()
{
	ADDTOCALLSTACK("CWorldThread::CloseAllUIDs");
	m_ObjDelete.DeleteAll();
	m_ObjNew.DeleteAll();
	m_UIDs.RemoveAll();

	if ( m_dwFreeUIDs != NULL )
	{
		free(m_dwFreeUIDs);
		m_dwFreeUIDs = NULL;
	}
	m_dwFreeOffset = FREE_UIDS_SIZE;
}

bool CWorldThread::IsSaving() const
{
	return (m_FileWorld.IsFileOpen() && m_FileWorld.IsWriteMode());
}

DWORD CWorldThread::GetUIDCount() const
{
	return m_UIDs.GetCount();
}

CObjBase *CWorldThread::FindUID(DWORD dwIndex) const
{
	if ( (dwIndex <= 0) || (dwIndex >= GetUIDCount()) )
		return NULL;
	if ( m_UIDs[dwIndex] == UID_PLACE_HOLDER )	// unusable for now (background save is running)
		return NULL;
	return m_UIDs[dwIndex];
}

void CWorldThread::FreeUID(DWORD dwIndex)
{
	// UID can be free only after worldsave finishes
	m_UIDs[dwIndex] = IsSaving() ? UID_PLACE_HOLDER : NULL;
}

DWORD CWorldThread::AllocUID(DWORD dwIndex, CObjBase *pObj)
{
	ADDTOCALLSTACK("CWorldThread::AllocUID");

	DWORD dwCountTotal = GetUIDCount();
	if ( !dwIndex )		// auto-select tbe suitable hole
	{
		if ( !dwCountTotal )	// if UID array is empty, increase it
		{
			dwIndex = 1;
			goto setcount;
		}

		if ( (m_dwFreeOffset < FREE_UIDS_SIZE) && (m_dwFreeUIDs != NULL) )
		{
			// Check if there's a free slot on UID array
			for ( ; (m_dwFreeOffset < FREE_UIDS_SIZE) && (m_dwFreeUIDs[m_dwFreeOffset] != 0); ++m_dwFreeOffset )
			{
				// Yes, there's a free slot
				if ( !m_UIDs[m_dwFreeUIDs[m_dwFreeOffset]] )
				{
					dwIndex = m_dwFreeUIDs[m_dwFreeOffset++];
					goto successalloc;
				}
			}
		}

		m_dwFreeOffset = FREE_UIDS_SIZE;	// mark array invalid, since it does not contain any empty slots (use default allocation for a while, till the next garbage collection)
		DWORD dwCount = dwCountTotal - 1;
		dwIndex = m_dwUIDIndexLast;
		while ( m_UIDs[dwIndex] != NULL )
		{
			if ( !--dwIndex )
				dwIndex = dwCountTotal - 1;

			if ( !--dwCount )
			{
				dwIndex = dwCountTotal;
				goto setcount;
			}
		}
	}
	else if ( dwIndex >= dwCountTotal )
	{
setcount:
		// There's no more free UIDs, increase the array
		m_UIDs.SetCount((dwIndex + 0x1000) & ~0xFFF);
	}

successalloc:
	m_dwUIDIndexLast = dwIndex;		// start from here next time so we have even distribution of allocation
	CObjBase *pObjPrv = m_UIDs[dwIndex];
	if ( pObjPrv )
	{
		// Don't use Delete() here, because the UID will still be assigned until async cleanup time
		DEBUG_ERR(("UID conflict delete 0%lx, '%s'\n", dwIndex, pObjPrv->GetName()));
		delete pObjPrv;
	}
	m_UIDs[dwIndex] = pObj;
	return dwIndex;
}

void CWorldThread::SaveThreadClose()
{
	ADDTOCALLSTACK("CWorldThread::SaveThreadClose");
	for ( size_t i = 1; i < GetUIDCount(); ++i )
	{
		if ( m_UIDs[i] == UID_PLACE_HOLDER )
			m_UIDs[i] = NULL;
	}

	m_FileData.Close();
	m_FileWorld.Close();
	m_FilePlayers.Close();
	m_FileMultis.Close();
}

int CWorldThread::FixObjTry(CObjBase *pObj, DWORD uid)
{
	ADDTOCALLSTACK_INTENSIVE("CWorldThread::FixObjTry");
	// RETURN:
	//  0 = success

	if ( !pObj )
		return 0x7102;

	if ( uid != 0 )
	{
		if ( (pObj->GetUID() & UID_O_INDEX_MASK) != uid )
		{
			DEBUG_ERR(("UID 0%lx, '%s', Mislinked\n", uid, pObj->GetName()));
			return 0x7101;
		}
	}
	return pObj->FixWeirdness();
}

int CWorldThread::FixObj(CObjBase *pObj, DWORD uid)
{
	ADDTOCALLSTACK("CWorldThread::FixObj");
	// Attempt to fix problems with this item
	// Ignore any children it may have for now
	// RETURN:
	//  0 = success

	int iResultCode;
	try
	{
		iResultCode = FixObjTry(pObj, uid);
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "FixObj");
		iResultCode = 0xFFFF;	// bad mem?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )	// catch all
	{
		g_Log.CatchEvent(NULL, "FixObj");
		iResultCode = 0xFFFF;	// bad mem?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	if ( iResultCode == 0 )
		return 0;

	try
	{
		uid = static_cast<DWORD>(pObj->GetUID());
		CItem *pItem = dynamic_cast<CItem *>(pObj);
		if ( pItem && pItem->IsType(IT_EQ_MEMORY_OBJ) )
		{
			pObj->Delete();
			return iResultCode;
		}
		ReportGarbageCollection(pObj, iResultCode);
		pObj->Delete();
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "UID=0%lx, Asserted cleanup", uid);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )
	{
		g_Log.CatchEvent(NULL, "UID=0%lx, Asserted cleanup", uid);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return iResultCode;
}

void CWorldThread::GarbageCollection_New()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_New");
	EXC_TRY("GarbageCollection_New");

	// Clean up created objects not placed on world
	if ( m_ObjNew.GetCount() > 0 )
	{
		g_Log.Event(LOGL_ERROR, "GC: Deleted %" FMTSIZE_T " unplaced objects\n", m_ObjNew.GetCount());
		for ( size_t i = 0; i < m_ObjNew.GetCount(); ++i )
		{
			CObjBase *pObj = dynamic_cast<CObjBase *>(m_ObjNew.GetAt(i));
			if ( !pObj )
				continue;

			ReportGarbageCollection(pObj, 0x3202);
			pObj->Delete();
		}
		m_ObjNew.DeleteAll();
	}
	m_ObjDelete.DeleteAll();

	// Clean up GM pages not linked to an valid char/account
	CGMPage *pGMPageNext = NULL;
	for ( CGMPage *pGMPage = static_cast<CGMPage *>(g_World.m_GMPages.GetHead()); pGMPage != NULL; pGMPage = pGMPageNext )
	{
		pGMPageNext = pGMPage->GetNext();
		if ( !pGMPage->m_uidChar.CharFind() )
		{
			DEBUG_ERR(("GC: Deleted GM Page linked to invalid char uid=0%lx\n", static_cast<DWORD>(pGMPage->m_uidChar)));
			delete pGMPage;
		}
		else if ( !g_Accounts.Account_Find(pGMPage->m_sAccount) )
		{
			DEBUG_ERR(("GC: Deleted GM Page linked to invalid account '%s'\n", pGMPage->GetName()));
			delete pGMPage;
		}
	}
	EXC_CATCH;
}

void CWorldThread::GarbageCollection_UIDs()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_UIDs");
	// Loop through m_ppUIDs array looking for objects without links to reality
	// This can take a while.

	GarbageCollection_New();

	size_t iCount = 0;
	for ( DWORD i = 1; i < GetUIDCount(); ++i )
	{
		try
		{
			CObjBase *pObj = m_UIDs[i];
			if ( !pObj || (pObj == UID_PLACE_HOLDER) )
				continue;

			// Look for anomalies and fix them (or delete it if unable to fix)
			int iResultCode = FixObj(pObj, i);
			if ( iResultCode )
			{
				delete pObj;	// immediately delete the obj instead call Delete() which will only delete it later
				FreeUID(i);		// get rid of junk UID
				continue;
			}

			if ( (iCount & 0x1FF) == 0 )
				g_Serv.PrintPercent(iCount, GetUIDCount());

			++iCount;
		}
		catch ( const CGrayError &e )
		{
			g_Log.CatchEvent(&e, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch ( ... )
		{
			g_Log.CatchEvent(NULL, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	GarbageCollection_New();

	if ( iCount != CObjBase::sm_iCount )
		g_Log.Event(LOGL_ERROR, "GC: Object memory leak %" FMTSIZE_T "!=%" FMTSIZE_T "\n", iCount, CObjBase::sm_iCount);
	else
		g_Log.Event(LOGL_EVENT, "GC: %" FMTSIZE_T " objects accounted for\n", iCount);

	// New UID engine - search for empty holes and store it in a huge array.
	// The size of the array should be enough even for huge shards to survive till next garbage collection
	if ( m_dwFreeUIDs != NULL )
	{
		memset(m_dwFreeUIDs, 0, FREE_UIDS_SIZE * sizeof(DWORD));
		m_dwFreeOffset = 0;

		for ( DWORD i = 1; i < GetUIDCount(); ++i )
		{
			CObjBase *pObj = m_UIDs[i];
			if ( !pObj )
			{
				if ( m_dwFreeOffset >= FREE_UIDS_SIZE - 1 )
					break;

				m_dwFreeUIDs[m_dwFreeOffset++] = i;
			}
		}
	}
}

///////////////////////////////////////////////////////////
// CWorldClock

UINT64 CWorldClock::GetSystemClock()
{
	ADDTOCALLSTACK("CWorldClock::GetSystemClock");
	// Return system wall-clock using high resolution value (milliseconds)
#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ((static_cast<UINT64>(ft.dwHighDateTime) << 32) + ft.dwLowDateTime) / 10000;
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (static_cast<UINT64>(ts.tv_sec * 10000) + (ts.tv_nsec / 100000)) / 10;
#endif
}

void CWorldClock::InitTime(UINT64 uTimeBase)
{
	ADDTOCALLSTACK("CWorldClock::InitTime");
	m_SystemClock_Prev = GetSystemClock();
	m_TimeClock.InitTime(uTimeBase);
}

void CWorldClock::Init()
{
	ADDTOCALLSTACK("CWorldClock::Init");
	m_SystemClock_Prev = GetSystemClock();
	m_TimeClock.Init();
}

bool CWorldClock::Advance()
{
	ADDTOCALLSTACK("CWorldClock::Advance");
	UINT64 uSystemClock = GetSystemClock();

	INT64 iTimeDiff = (TICK_PER_SEC * (uSystemClock - m_SystemClock_Prev)) / CLOCKS_PER_SEC;
	if ( !iTimeDiff )
		return false;

	m_SystemClock_Prev = uSystemClock;

	if ( iTimeDiff < 0 )
	{
		// System clock has changed forward
		g_Log.Event(LOGL_WARN, "System clock has changed forward (daylight saving change, etc). This may cause strange behavior on some object timers\n");
		return false;
	}

	CServTime timeClock = m_TimeClock + iTimeDiff;
	m_TimeClock = timeClock;

	if ( timeClock < m_TimeClock )
	{
		// System clock has changed backward
		g_Log.Event(LOGL_WARN, "System clock has changed backward (daylight saving change, etc). This may cause strange behavior on some object timers\n");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////
// CWorld

CWorld::CWorld()
{
	m_savetimer = 0;
	m_iSaveStage = 0;
	m_fSaveNotificationSent = false;

	m_timeSector.Init();
	m_timeSave.Init();
	m_timeRespawn.Init();
	m_timeCallUserFunc.Init();
	m_timeStartup.Init();

	m_Sectors = NULL;
	m_SectorsQty = 0;
	m_Sector_Pulse = 0;

	m_iSaveCountID = 0;
	m_iPrevBuild = 0;
	m_iLoadVersion = 0;
}

void CWorld::Init()
{
	EXC_TRY("Init");
	if ( m_Sectors )	// disable changes on-the-fly
		return;

	g_MapList.Init();
	if ( g_MapList.m_pMapDiffCollection )
		g_MapList.m_pMapDiffCollection->Init();

	// Initialize all sectors
	int iSectors = 0;
	for ( int iMap = 0; iMap < 256; ++iMap )
	{
		if ( !g_MapList.m_maps[iMap] )
			continue;
		iSectors += g_MapList.GetSectorQty(iMap);
	}

	m_Sectors = new CSector *[iSectors];
	TCHAR *pszMapName = Str_GetTemp();
	TCHAR *pszMaps = Str_GetTemp();
	TCHAR *pszSectorSize = Str_GetTemp();
	TCHAR *pszSectors = Str_GetTemp();

	static LPCTSTR const sm_szMapNames[] =
	{
		"Felucca",
		"Trammel",
		"Ilshenar",
		"Malas",
		"Tokuno Islands",
		"Ter Mur"
	};

	for ( int iMap = 0; iMap < 256; ++iMap )
	{
		if ( !g_MapList.m_maps[iMap] )
			continue;

		if ( *pszMaps )
			strcat(pszMaps, ", ");
		sprintf(pszMapName, "%d='%s'", iMap, (static_cast<size_t>(iMap) < COUNTOF(sm_szMapNames)) ? sm_szMapNames[iMap] : "[Unnamed]");
		strcat(pszMaps, pszMapName);

		if ( *pszSectors )
			strcat(pszSectors, ", ");
		sprintf(pszSectorSize, "%d='%d'", iMap, g_MapList.GetSectorQty(iMap));
		strcat(pszSectors, pszSectorSize);

		// Initialize sectors
		for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
		{
			CSector *pSector = new CSector;
			pSector->Init(iSector, iMap);
			m_Sectors[m_SectorsQty++] = pSector;
		}
	}
	ASSERT(m_SectorsQty);

	if ( *pszMaps )
		g_Log.Event(LOGM_INIT, "Expansion maps supported by your MUL files: %s\n", pszMaps);
	if ( *pszSectors )
		g_Log.Event(LOGM_INIT, "Allocating map sectors: %s\n\n", pszSectors);
	EXC_CATCH;
}

CWorld::~CWorld()
{
	Close();
}

void CWorld::GetBackupName(CGString &sBuffer, LPCTSTR pszBaseDir, TCHAR pszType, int iSaveCount)	// static
{
	ADDTOCALLSTACK("CWorld::GetBackupName");
	int iCount = iSaveCount;
	int iGroup = 0;
	for ( ; iGroup < g_Cfg.m_iSaveBackupLevels; ++iGroup )
	{
		if ( iCount & 0x7 )
			break;
		iCount >>= 3;
	}
	sBuffer.Format("%s" SPHERE_FILE "b%d%d%c%s", pszBaseDir, iGroup, iCount & 0x7, pszType, SPHERE_SCRIPT);
}

bool CWorld::OpenScriptBackup(CScript &s, LPCTSTR pszBaseDir, LPCTSTR pszBaseName, int iSaveCount)	// static
{
	ADDTOCALLSTACK("CWorld::OpenScriptBackup");
	ASSERT(pszBaseName);

	CGString sBackupName;
	GetBackupName(sBackupName, pszBaseDir, pszBaseName[0], iSaveCount);

	// Remove previous backup file using the same name
	if ( s.Open(sBackupName, OF_NONCRIT) )
	{
		s.Close();
		if ( remove(sBackupName) != 0 )
			g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Can't remove previous backup file '%s'\n", static_cast<LPCTSTR>(sBackupName));
	}

	// Rename current save file to backup file
	CGString sSaveName;
	sSaveName.Format("%s" SPHERE_FILE "%s%s", pszBaseDir, pszBaseName, SPHERE_SCRIPT);

	if ( s.Open(sSaveName, OF_NONCRIT) )
	{
		s.Close();
		if ( rename(sSaveName, sBackupName) != 0 )
			g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Can't rename save file '%s' into backup file '%s'\n", static_cast<LPCTSTR>(sSaveName), static_cast<LPCTSTR>(sBackupName));
	}

	// Create new empty save file to be filled
	if ( !s.Open(sSaveName, OF_WRITE|OF_TEXT|OF_NONCRIT|OF_DEFAULTMODE) )
	{
		g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Can't create save file '%s'\n", static_cast<LPCTSTR>(sSaveName));
		return false;
	}

	return true;
}

bool CWorld::SaveStage()
{
	ADDTOCALLSTACK("CWorld::SaveStage");
	// Save world state in stages
	// RETURN:
	//  true = continue to next stage
	//  false = save finished
	EXC_TRY("SaveStage");

	bool fRc = true;
	if ( m_iSaveStage == -1 )
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_New();
	}
	else if ( m_iSaveStage < static_cast<int>(m_SectorsQty) )
	{
		// Save world sectors
		if ( IsSetEF(EF_DynamicBackgroundSave) )
		{
			unsigned int uComplexity = 0;

			CSector *pSector = m_Sectors[m_iSaveStage];
			if ( pSector )
			{
				pSector->r_Write();
				uComplexity += (pSector->GetCharComplexity() + pSector->GetInactiveChars()) * 100 + pSector->GetItemComplexity();
			}

			size_t iDynamicStage = m_iSaveStage + 1;
			if ( uComplexity <= g_Cfg.m_iSaveStepMaxComplexity )
			{
				size_t iSectorsCount = 1;
				while ( (iDynamicStage < m_SectorsQty) && (iSectorsCount <= g_Cfg.m_iSaveSectorsPerTick) )
				{
					pSector = m_Sectors[iDynamicStage];
					if ( pSector )
					{
						uComplexity += (pSector->GetCharComplexity() + pSector->GetInactiveChars()) * 100 + pSector->GetItemComplexity();
						if ( uComplexity > g_Cfg.m_iSaveStepMaxComplexity )
							break;

						pSector->r_Write();
						m_iSaveStage = iDynamicStage;
						++iSectorsCount;
					}
					++iDynamicStage;
				}
			}
		}
		else
		{
			if ( m_Sectors[m_iSaveStage] )
				m_Sectors[m_iSaveStage]->r_Write();
		}
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) )
	{
		// Save global variables
		m_FileData.WriteSection("TIMERF");
		g_World.m_TimedFunctions.r_Write(m_FileData);

		m_FileData.WriteSection("GLOBALS");
		g_Exp.m_VarGlobals.r_WritePrefix(m_FileData);

		g_Exp.m_ListGlobals.r_WriteSave(m_FileData);

		size_t iQty = g_Cfg.m_RegionDefs.GetCount();
		for ( size_t i = 0; i < iQty; ++i )
		{
			CRegionBase *pRegion = dynamic_cast<CRegionBase *>(g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			m_FileData.WriteSection("WORLDSCRIPT %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		for ( CGMPage *pGMPage = static_cast<CGMPage *>(m_GMPages.GetHead()); pGMPage != NULL; pGMPage = pGMPage->GetNext() )
			pGMPage->r_Write(m_FileData);
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) + 1 )
	{
		// Save servers (not needed anymore, just skip it)
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) + 2 )
	{
		// Save accounts
		fRc = g_Accounts.Account_SaveAll();
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) + 3 )
	{
		// Save finished
		ULONGLONG llTicksStart = m_savetimer, llTicksEnd;
		TIME_PROFILE_END;

		++m_iSaveCountID;
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time

		m_FileData.WriteSection("EOF");
		m_FileWorld.WriteSection("EOF");
		m_FilePlayers.WriteSection("EOF");
		m_FileMultis.WriteSection("EOF");

		g_Log.Event(LOGM_SAVE, "Context data saved (%s)\n", static_cast<LPCTSTR>(m_FileData.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "World data saved   (%s)\n", static_cast<LPCTSTR>(m_FileWorld.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "Player data saved  (%s)\n", static_cast<LPCTSTR>(m_FilePlayers.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "Multi data saved   (%s)\n", static_cast<LPCTSTR>(m_FileMultis.GetFilePath()));

		TCHAR *pszTime = Str_GetTemp();
		sprintf(pszTime, "%lld.%04lld", static_cast<INT64>(TIME_PROFILE_GET_HI / 1000), static_cast<INT64>(TIME_PROFILE_GET_LO));
		g_Log.Event(LOGM_SAVE, "World save completed, took %s seconds\n", pszTime);

		CScriptTriggerArgs Args(pszTime);
		g_Serv.r_Call("f_onserver_save_finished", &g_Serv, &Args);

		// Clean up all the held over UIDs
		SaveThreadClose();
		m_iSaveStage = INT_MAX;
		return false;
	}

	if ( g_Cfg.m_iSaveBackgroundTime )
	{
		int iNextTime = g_Cfg.m_iSaveBackgroundTime / m_SectorsQty;
		if ( iNextTime > TICK_PER_SEC / 2 )
			iNextTime = TICK_PER_SEC / 2;
		m_timeSave = GetCurrentTime() + iNextTime;
	}
	++m_iSaveStage;
	return fRc;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("stage '%d' qty '%" FMTSIZE_T "' time '%llu'\n", m_iSaveStage, m_SectorsQty, m_timeSave.GetTimeRaw());
	EXC_DEBUG_END;

	++m_iSaveStage;
	return false;
}

bool CWorld::SaveForce()
{
	ADDTOCALLSTACK("CWorld::SaveForce");
	// Save world state

	Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSAVE));
#ifdef _MTNETWORK
	if ( !g_NetworkManager.isOutputThreaded() )
		g_NetworkManager.flushAllClients();
#else
	if ( !g_NetworkOut.isActive() )
		g_NetworkOut.flushAll();
#endif

	g_Serv.SetServerMode(SERVMODE_Saving);
	bool fSaved = true;
	bool fSuccess = true;

	size_t iStage = 0;
	static LPCTSTR const sm_szSaveStage[] =
	{
		"garbage collection",
		"sectors",
		"global variables",
		"servers",
		"accounts",
		""
	};

	while ( fSaved )
	{
		try
		{
			if ( (m_iSaveStage > -1) && (m_iSaveStage < static_cast<int>(m_SectorsQty)) )
				iStage = 1;
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) )
				iStage = 2;
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) + 1 )
				iStage = 3;
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) + 2 )
				iStage = 4;
			else
				iStage = 5;

			fSaved = SaveStage();
			if ( !(m_iSaveStage & 0x1FF) )	// don't update too often
				g_Serv.PrintPercent(m_iSaveStage, m_SectorsQty + 3);

			if ( !fSaved && (iStage != 5) )
				goto failedstage;
		}
		catch ( const CGrayError &e )
		{
			g_Log.CatchEvent(&e, "Save FAILED for stage %d (%s)", m_iSaveStage, sm_szSaveStage[iStage]);
			fSuccess = false;
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch ( ... )
		{
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
			goto failedstage;
		}
		continue;

failedstage:
		g_Log.CatchEvent(NULL, "Save FAILED for stage %d (%s)", m_iSaveStage, sm_szSaveStage[iStage]);
		fSuccess = false;
	}

	g_Serv.SetServerMode(SERVMODE_Run);
	return fSuccess;
}

bool CWorld::SaveTry(bool fForceImmediate)
{
	ADDTOCALLSTACK("CWorld::SaveTry");
	// Save world state

	EXC_TRY("SaveTry");
	if ( m_FileWorld.IsFileOpen() )
	{
		// Server is already saving
		ASSERT(IsSaving());
		if ( fForceImmediate )	// finish it now
			return SaveForce();
		if ( g_Cfg.m_iSaveBackgroundTime )
			return SaveStage();
		return false;
	}

	// Do the write async from here in the future
	if ( g_Cfg.m_fSaveGarbageCollect )
		GarbageCollection();

	ULONGLONG llTicksStart;
	TIME_PROFILE_START;
	m_savetimer = llTicksStart;

	// Determine save name based on counter ID
	if ( !OpenScriptBackup(m_FileData, g_Cfg.m_sWorldBaseDir, "data", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FileWorld, g_Cfg.m_sWorldBaseDir, "world", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FilePlayers, g_Cfg.m_sWorldBaseDir, "chars", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FileMultis, g_Cfg.m_sWorldBaseDir, "multis", m_iSaveCountID) )
		return false;

	m_fSaveParity = !m_fSaveParity;		// flip the parity of the save
	m_iSaveStage = -1;
	m_fSaveNotificationSent = false;
	m_timeSave.Init();

	// Write file headers
	r_Write(m_FileData);
	r_Write(m_FileWorld);
	r_Write(m_FilePlayers);
	r_Write(m_FileMultis);

	if ( fForceImmediate || !g_Cfg.m_iSaveBackgroundTime )
		return SaveForce();

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("immediate '%d'\n", static_cast<int>(fForceImmediate));
	EXC_DEBUG_END;
	return false;
}

bool CWorld::Save(bool fForceImmediate)
{
	ADDTOCALLSTACK("CWorld::Save");
	// Save world state

	bool fSaved = true;
	try
	{
		CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
		TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
		g_Serv.r_Call("f_onserver_save", &g_Serv, &Args, NULL, &tr);
		if ( tr == TRIGRET_RET_TRUE )
			return false;

		// Fushing before the server should fix #2306
		// The scripts fills the clients buffer and the server flush the data during the save.
		// Should we flush only non threaded output or force it to flush on any conditions?
#ifdef _MTNETWORK
		if ( !g_NetworkManager.isOutputThreaded() )
#else
		if ( !g_NetworkOut.isActive() )
#endif
		{
#ifdef _DEBUG 
			g_Log.EventDebug("Flushing %lu clients output data...\n", g_Serv.StatGet(SERV_STAT_CLIENTS));
#endif
#ifdef _MTNETWORK
			g_NetworkManager.flushAllClients();
#else
			g_NetworkOut.flushAll();
#endif
#ifdef _DEBUG
			g_Log.EventDebug("Done flushing clients output data\n");
#endif
		}

		fForceImmediate = (Args.m_iN1 != 0);
		fSaved = SaveTry(fForceImmediate);
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Save FAILED");
		Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();
		m_FileWorld.Close();
		m_FilePlayers.Close();
		m_FileMultis.Close();
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )
	{
		g_Log.CatchEvent(NULL, "Save FAILED");
		Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();
		m_FileWorld.Close();
		m_FilePlayers.Close();
		m_FileMultis.Close();
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
	g_Serv.r_Call(fSaved ? "f_onserver_save_ok" : "f_onserver_save_fail", &g_Serv, &Args);
	return fSaved;
}

void CWorld::SaveStatics()
{
	ADDTOCALLSTACK("CWorld::SaveStatics");
	// Save world static items

	try
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_New();

		CScript m_FileStatics;
		if ( !OpenScriptBackup(m_FileStatics, g_Cfg.m_sWorldBaseDir, "statics", m_iSaveCountID) )
			return;
		r_Write(m_FileStatics);

		Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSTATICSAVE));
#ifdef _MTNETWORK
		if ( !g_NetworkManager.isOutputThreaded() )
			g_NetworkManager.flushAllClients();
#else
		if ( !g_NetworkOut.isActive() )
			g_NetworkOut.flushAll();
#endif

		// Loop through all sectors and save static items
		CSector *pSector;
		CItem *pItem, *pNext;
		for ( int iMap = 0; iMap < 256; ++iMap )
		{
			if ( !g_MapList.m_maps[iMap] )
				continue;

			for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
			{
				pSector = GetSector(iMap, iSector);
				if ( !pSector )
					continue;

				pItem = static_cast<CItem *>(pSector->m_Items_Inert.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( !pItem->IsAttr(ATTR_STATIC) )
						continue;

					pItem->r_WriteSafe(m_FileStatics);
				}

				pItem = static_cast<CItem *>(pSector->m_Items_Timer.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( !pItem->IsAttr(ATTR_STATIC) )
						continue;

					pItem->r_WriteSafe(m_FileStatics);
				}
			}
		}

		m_FileStatics.WriteSection("EOF");
		m_FileStatics.Close();
		g_Log.Event(LOGM_SAVE, "Statics data saved (%s)\n", static_cast<LPCTSTR>(m_FileStatics.GetFilePath()));
	}
	catch ( const CGrayError &e )
	{
		g_Log.CatchEvent(&e, "Statics save FAILED");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch ( ... )
	{
		g_Log.CatchEvent(NULL, "Statics save FAILED");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
}

bool CWorld::LoadFile(LPCTSTR pszFileName)
{
	ADDTOCALLSTACK("CWorld::LoadFile");
	// Load files

	g_Log.Event(LOGL_EVENT, "Loading %s%s\n", static_cast<LPCTSTR>(pszFileName), SPHERE_SCRIPT);

	CScript s;
	if ( !s.Open(pszFileName, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		return false;

	// Get file length
	DWORD dwLoadSize = s.GetLength();
	int iLoadStage = 0;

	CScriptFileContext ScriptContext(&s);

	// Load header first
	CScriptObj::r_Load(s);

	while ( s.FindNextSection() )
	{
		if ( !(++iLoadStage & 0x1FF) )	// don't update too often
			g_Serv.PrintPercent(s.GetPosition(), dwLoadSize);

		try
		{
			// Load file
			g_Cfg.LoadResourceSection(&s);
		}
		catch ( const CGrayError &e )
		{
			g_Log.CatchEvent(&e, "Exception on line %d, " SPHERE_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch ( ... )
		{
			g_Log.CatchEvent(NULL, "Exception on line %d, " SPHERE_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	if ( s.IsSectionType("EOF") )
	{
		s.Close();
		return true;
	}
	else
	{
		g_Log.Event(LOGL_CRIT, "File is corrupt, no [EOF] marker found\n");
		return false;
	}
}

bool CWorld::LoadWorld()
{
	EXC_TRY("LoadWorld");
	// Load world from save files
	int iLoadedFiles = 0;

	CGString sDataName;
	sDataName.Format("%s" SPHERE_FILE "data", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));
	if ( LoadFile(sDataName) )
		++iLoadedFiles;

	CGString sStaticsName;
	sStaticsName.Format("%s" SPHERE_FILE "statics", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));
	static_cast<void>(LoadFile(sStaticsName));	// optional

	CGString sMultisName;
	sMultisName.Format("%s" SPHERE_FILE "multis", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));
	if ( LoadFile(sMultisName) )
		++iLoadedFiles;

	CGString sWorldName;
	sWorldName.Format("%s" SPHERE_FILE "world", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));
	if ( LoadFile(sWorldName) )
		++iLoadedFiles;

	CGString sCharsName;
	sCharsName.Format("%s" SPHERE_FILE "chars", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));
	if ( LoadFile(sCharsName) )
		++iLoadedFiles;

	return ((iLoadedFiles == 0) || (iLoadedFiles == 4));

	EXC_CATCH;
	return false;
}

bool CWorld::LoadAll()
{
	// Load world/accounts from save files

	// Start count (will grow as needed)
	m_UIDs.SetCount(8 * 1024);
	m_Clock.Init();		// will be loaded from world file

	g_Accounts.Account_LoadAll(false);
	if ( !LoadWorld() )
		return false;

	m_timeStartup = GetCurrentTime();
	m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time

	// Set all the sector light levels now that we know the time
	// This should not look like part of the load (CTRIG_EnvironChange triggers should run)
	size_t iCount;
	for ( size_t s = 0; s < m_SectorsQty; ++s )
	{
		EXC_TRYSUB("Load");
		CSector *pSector = m_Sectors[s];
		if ( pSector )
		{
			if ( !pSector->IsLightOverriden() )
				pSector->SetLight(-1);

			iCount = pSector->GetItemComplexity();
			if ( iCount > g_Cfg.m_iMaxSectorComplexity )
				g_Log.Event(LOGL_WARN, "%" FMTSIZE_T " items at %s. Sector too complex!\n", iCount, pSector->GetBasePoint().WriteUsed());

			iCount = pSector->GetCharComplexity();
			if ( iCount > g_Cfg.m_iMaxCharComplexity )
				g_Log.Event(LOGL_WARN, "%" FMTSIZE_T " chars at %s. Sector too complex!\n", iCount, pSector->GetBasePoint().WriteUsed());
		}
		EXC_CATCHSUB("Sector light levels");
	}

	EXC_TRYSUB("Load");
	GarbageCollection();
	EXC_CATCHSUB("Garbage collect");
	return true;
}

void CWorld::r_Write(CScript &s)
{
	ADDTOCALLSTACK("CWorld::r_Write");
	// Write file header
	s.WriteKey("TITLE", SPHERE_TITLE " World Script");
	s.WriteKey("VERSION", SPHERE_VERSION);
	s.WriteKeyVal("PREVBUILD", SPHERE_VER_BUILD);
	s.WriteKeyVal("TIME", GetCurrentTime().GetTimeRaw());
	s.WriteKeyVal("SAVECOUNT", m_iSaveCountID);
	s.Flush();		// force this out to the file now
}

bool CWorld::r_GetRef(LPCTSTR &pszKey, CScriptObj *&pRef)
{
	ADDTOCALLSTACK("CWorld::r_GetRef");
	if ( !strnicmp(pszKey, "LASTNEW", 7) )
	{
		pszKey += 7;
		if ( !strnicmp(pszKey, "ITEM", 4) )
		{
			pszKey += 4;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewItem.ItemFind();
			return true;
		}
		if ( !strnicmp(pszKey, "CHAR", 4) )
		{
			pszKey += 4;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewChar.CharFind();
			return true;
		}
	}
	return false;
}

enum WC_TYPE
{
	WC_PREVBUILD,
	WC_SAVECOUNT,
	WC_TIME,
	WC_TITLE,
	WC_VERSION,
	WC_QTY
};

LPCTSTR const CWorld::sm_szLoadKeys[WC_QTY + 1] =	// static
{
	"PREVBUILD",
	"SAVECOUNT",
	"TIME",
	"TITLE",
	"VERSION",
	NULL
};

bool CWorld::r_WriteVal(LPCTSTR pszKey, CGString &sVal, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CWorld::r_WriteVal");
	EXC_TRY("WriteVal");

	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case WC_PREVBUILD:
			sVal.FormatVal(m_iPrevBuild);
			break;
		case WC_SAVECOUNT:
			sVal.FormatVal(m_iSaveCountID);
			break;
		case WC_TIME:
			sVal.FormatLLVal(GetCurrentTime().GetTimeRaw());
			break;
		case WC_TITLE:
			sVal = SPHERE_TITLE " World Script";
			break;
		case WC_VERSION:
			sVal = SPHERE_VERSION;
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CWorld::r_LoadVal(CScript &s)
{
	ADDTOCALLSTACK("CWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR pszKey = s.GetKey();
	switch ( FindTableSorted(pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys) - 1) )
	{
		case WC_PREVBUILD:
			m_iPrevBuild = s.GetArgVal();
			break;
		case WC_SAVECOUNT:
			m_iSaveCountID = s.GetArgVal();
			break;
		case WC_TIME:
			if ( !g_Serv.IsLoading() )
				DEBUG_WARN(("Setting TIME while running is BAD!\n"));
			m_Clock.InitTime(s.GetArgLLVal());
			break;
		case WC_VERSION:
			m_iLoadVersion = s.GetArgVal();
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CWorld::RespawnDeadNPCs()
{
	ADDTOCALLSTACK("CWorld::RespawnDeadNPCs");
	// Respawn dead NPCs on all sectors

	g_Serv.SetServerMode(SERVMODE_RestockAll);
	for ( int iMap = 0; iMap < 256; ++iMap )
	{
		if ( !g_MapList.m_maps[iMap] )
			continue;

		for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
		{
			CSector *pSector = GetSector(iMap, iSector);
			if ( pSector )
				pSector->RespawnDeadNPCs();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::Restock()
{
	ADDTOCALLSTACK("CWorld::Restock");
	// Restock base items on all sectors

	g_Serv.SetServerMode(SERVMODE_RestockAll);
	for ( size_t i = 0; i < COUNTOF(g_Cfg.m_ResHash.m_Array); ++i )
	{
		for ( size_t j = 0; j < g_Cfg.m_ResHash.m_Array[i].GetCount(); ++j )
		{
			CResourceDef *pResDef = g_Cfg.m_ResHash.m_Array[i][j];
			if ( !pResDef || (pResDef->GetResType() != RES_ITEMDEF) )
				continue;

			CItemBase *pBase = dynamic_cast<CItemBase *>(pResDef);
			if ( pBase )
				pBase->Restock();
		}
	}
	for ( int iMap = 0; iMap < 256; ++iMap )
	{
		if ( !g_MapList.m_maps[iMap] )
			continue;

		for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
		{
			CSector *pSector = GetSector(iMap, iSector);
			if ( pSector )
				pSector->Restock();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::ReloadMultis()
{
	ADDTOCALLSTACK("CWorld::ReloadMultis");
	// Loop through all multi items on world to reload the multi region.
	// This must be called after server resync, when dynamic regions from multis
	// already placed on world got replaced by static regions loaded from scripts

	CSector *pSector = NULL;
	CItem *pItem = NULL;
	CItemMulti *pMulti = NULL;

	for ( int iMap = 0; iMap < 256; ++iMap )
	{
		if ( !g_MapList.m_maps[iMap] )
			continue;

		for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
		{
			pSector = g_World.GetSector(iMap, iSector);
			if ( !pSector )
				continue;

			pItem = static_cast<CItem *>(pSector->m_Items_Inert.GetHead());
			for ( ; pItem != NULL; pItem = pItem->GetNext() )
			{
				if ( !pItem->IsTypeMulti() )
					continue;

				pMulti = static_cast<CItemMulti *>(pItem);
				if ( pMulti && pMulti->m_pRegion )
				{
					pMulti->m_pRegion->UnRealizeRegion();
					pMulti->MultiRealizeRegion();
				}
			}

			pItem = static_cast<CItem *>(pSector->m_Items_Timer.GetHead());
			for ( ; pItem != NULL; pItem = pItem->GetNext() )
			{
				if ( !pItem->IsTypeMulti() )
					continue;

				pMulti = static_cast<CItemMulti *>(pItem);
				if ( pMulti && pMulti->m_pRegion )
				{
					pMulti->m_pRegion->UnRealizeRegion();
					pMulti->MultiRealizeRegion();
				}
			}
		}
	}
}

void CWorld::Close()
{
	ADDTOCALLSTACK("CWorld::Close");
	if ( IsSaving() )
		Save(true);

	m_Stones.RemoveAll();
	m_Parties.DeleteAll();
	m_GMPages.DeleteAll();
	m_ObjStatusUpdates.RemoveAll();

	if ( m_Sectors )
	{
		// Clear everything loaded from all sectors
		for ( size_t s = 0; s < m_SectorsQty; ++s )
			m_Sectors[s]->Close();

		// Now delete all sectors
		for ( size_t s = 0; s < m_SectorsQty; ++s )
		{
			delete m_Sectors[s];
			m_Sectors[s] = NULL;
		}

		delete[] m_Sectors;
		m_Sectors = NULL;
		m_SectorsQty = 0;
	}

	memset(g_MapList.m_maps, 0, sizeof(g_MapList.m_maps));

	if ( g_MapList.m_pMapDiffCollection )
	{
		delete g_MapList.m_pMapDiffCollection;
		g_MapList.m_pMapDiffCollection = NULL;
	}

	CloseAllUIDs();

	m_Clock.Init();		// no more sense of time
}

void CWorld::GarbageCollection()
{
	ADDTOCALLSTACK("CWorld::GarbageCollection");
	g_Log.Flush();
	SERVMODE_TYPE smPrevServMode = g_Serv.m_iModeCode;
	g_Serv.SetServerMode(SERVMODE_Loading);
	GarbageCollection_UIDs();
	g_Serv.SetServerMode(smPrevServMode);
	g_Log.Flush();
}

void CWorld::Speak(const CObjBaseTemplate *pSrc, LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font)
{
	ADDTOCALLSTACK("CWorld::Speak");
	if ( !pszText )
		return;

	bool fSpeakAsGhost = false;
	CGString sTextGhost;

	if ( pSrc )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar *>(pSrc);
		if ( pCharSrc )
		{
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
			if ( wHue == HUE_TEXT_DEF )
				wHue = pCharSrc->m_SpeechHue;
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		CChar *pChar = pClient->GetChar();
		if ( !pChar || !pClient->CanHear(pSrc, mode) )
			continue;

		CGString sSpeak = pszText;
		if ( fSpeakAsGhost && !pChar->CanUnderstandGhost() )
		{
			if ( sTextGhost.IsEmpty() )
			{
				sTextGhost = pszText;
				for ( size_t i = 0; i < sTextGhost.GetLength(); ++i )
				{
					if ( (sTextGhost[i] != ' ') && (sTextGhost[i] != '\t') )
						sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
				}
			}
			sSpeak = sTextGhost;
		}

		if ( !pClient->m_NetState->isClientEnhanced() && pSrc && !pChar->CanSee(pSrc) )
			sSpeak.Format("<%s>%s", pSrc->GetName(), pszText);

		pClient->addBarkParse(sSpeak, pSrc, wHue, mode, font);
	}
}

#define SPEAKFLAG_NONE		0x0
#define SPEAKFLAG_GHOST		0x1
#define SPEAKFLAG_NAMED		0x2

void CWorld::SpeakUNICODE(const CObjBaseTemplate *pSrc, const NCHAR *pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang)
{
	ADDTOCALLSTACK("CWorld::SpeakUNICODE");
	if ( !pszText )
		return;

	bool fSpeakAsGhost = false;
	NCHAR szTextGhost[MAX_TALK_BUFFER] = {};
	NCHAR szTextGhostNamed[MAX_TALK_BUFFER] = {};
	NCHAR szTextNamed[MAX_TALK_BUFFER] = {};

	if ( pSrc )
	{
		const CChar *pCharSrc = dynamic_cast<const CChar *>(pSrc);
		if ( pCharSrc )
		{
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
			if ( wHue == HUE_TEXT_DEF )
				wHue = pCharSrc->m_SpeechHue;
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != NULL; pClient = it.next() )
	{
		CChar *pChar = pClient->GetChar();
		if ( !pChar || !pClient->CanHear(pSrc, mode) )
			continue;

		BYTE bFlags = SPEAKFLAG_NONE;
		if ( fSpeakAsGhost && !pChar->CanUnderstandGhost() )
			bFlags |= SPEAKFLAG_GHOST;
		if ( !pClient->m_NetState->isClientEnhanced() && pSrc && !pChar->CanSee(pSrc) )
			bFlags |= SPEAKFLAG_NAMED;

		if ( bFlags & SPEAKFLAG_GHOST )
		{
			if ( szTextGhost[0] == '\0' )
			{
				size_t i;
				for ( i = 0; (i < MAX_TALK_BUFFER - 1) && pszText[i]; ++i )
				{
					if ( (pszText[i] != ' ') && (pszText[i] != '\t') )
						szTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
					else
						szTextGhost[i] = pszText[i];
				}
				szTextGhost[i] = '\0';
			}

			if ( !(bFlags & SPEAKFLAG_NAMED) )
			{
				pClient->addBarkUNICODE(szTextGhost, pSrc, wHue, mode, font, lang);
				return;
			}
		}
		if ( bFlags & SPEAKFLAG_NAMED )
		{
			if ( bFlags & SPEAKFLAG_GHOST )
			{
				if ( szTextGhostNamed[0] == '\0' )
				{
					CGString sName;
					sName.Format("<%s>", pSrc->GetName());

					int iLen = CvtSystemToNUNICODE(szTextGhostNamed, COUNTOF(szTextGhostNamed), sName, -1);
					for ( size_t i = 0; (szTextGhost[i] != '\0') && (iLen < MAX_TALK_BUFFER - 1); ++i, ++iLen )
						szTextGhostNamed[iLen] = szTextGhost[i];

					szTextGhostNamed[iLen] = '\0';
				}
				pClient->addBarkUNICODE(szTextGhostNamed, pSrc, wHue, mode, font, lang);
				return;
			}
			else
			{
				if ( szTextNamed[0] == '\0' )
				{
					CGString sName;
					sName.Format("<%s>", pSrc->GetName());

					int iLen = CvtSystemToNUNICODE(szTextNamed, COUNTOF(szTextNamed), sName, -1);
					for ( size_t i = 0; (pszText[i] != '\0') && (iLen < MAX_TALK_BUFFER - 1); ++i, ++iLen )
						szTextNamed[iLen] = pszText[i];

					szTextNamed[iLen] = '\0';
				}
				pClient->addBarkUNICODE(szTextNamed, pSrc, wHue, mode, font, lang);
				return;
			}
		}
		pClient->addBarkUNICODE(pszText, pSrc, wHue, mode, font, lang);
	}
}

void CWorld::Broadcast(LPCTSTR pszMsg)
{
	ADDTOCALLSTACK("CWorld::Broadcast");
	// System broadcast in bold text
	Speak(NULL, pszMsg, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD);
}

void __cdecl CWorld::Broadcastf(LPCTSTR pszMsg, ...)
{
	ADDTOCALLSTACK("CWorld::Broadcastf");
	// System broadcast in bold text
	TemporaryString sTemp;
	va_list vargs;
	va_start(vargs, pszMsg);
	_vsnprintf(sTemp, sTemp.realLength(), pszMsg, vargs);
	va_end(vargs);
	Broadcast(sTemp);
}

DWORD CWorld::GetGameWorldTime(CServTime basetime) const
{
	ADDTOCALLSTACK("CWorld::GetGameWorldTime");
	// Convert real word time to ingame time
	// ARGS:
	//  basetime = time in TICK_PER_SEC
	return static_cast<DWORD>(basetime.GetTimeRaw() / g_Cfg.m_iGameMinuteLength);
}

CServTime CWorld::GetNextNewMoon(bool fMoonIndex) const
{
	ADDTOCALLSTACK("CWorld::GetNextNewMoon");
	// Predict time when moon phase will change

	DWORD dwSynodic = fMoonIndex ? FELUCCA_SYNODIC_PERIOD : TRAMMEL_SYNODIC_PERIOD;
	DWORD dwNextMonth = GetGameWorldTime() + dwSynodic;
	UINT64 uiNewStart = dwNextMonth - (dwNextMonth % dwSynodic);

	// Convert to TICK_PER_SEC ticks
	CServTime time;
	time.InitTime(uiNewStart * g_Cfg.m_iGameMinuteLength);
	return time;
}

unsigned int CWorld::GetMoonPhase(bool fMoonIndex) const
{
	ADDTOCALLSTACK("CWorld::GetMoonPhase ");
	// fMoonIndex:
	//  true = Felucca
	//  false = Trammel
	//
	// There are 8 distinct moon phases: New, Waxing Crescent, First Quarter, Waxing Gibbous, Full, Waning Gibbous, Third Quarter, and Waning Crescent
	// To calculate the phase, we use the following formula:
	//			CurrentTime % SynodicPeriod
	//	Phase = --------------------------- * 8
	//			       SynodicPeriod

	if ( fMoonIndex )
		return IMULDIV(GetGameWorldTime() % FELUCCA_SYNODIC_PERIOD, 8, FELUCCA_SYNODIC_PERIOD);
	else
		return IMULDIV(GetGameWorldTime() % TRAMMEL_SYNODIC_PERIOD, 8, TRAMMEL_SYNODIC_PERIOD);
}

void CWorld::OnTick()
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::OnTick");
	if ( g_Serv.IsLoading() || !m_Clock.Advance() )
		return;

	if ( m_timeSector <= GetCurrentTime() )
	{
		// Only need a SECTOR_TICK_PERIOD tick to do world stuff
		m_timeSector = GetCurrentTime() + SECTOR_TICK_PERIOD;	// next tick time
		++m_Sector_Pulse;

		for ( int iMap = 0; iMap < 256; ++iMap )
		{
			if ( !g_MapList.m_maps[iMap] )
				continue;

			for ( int iSector = 0; iSector < g_MapList.GetSectorQty(iMap); ++iSector )
			{
				EXC_TRYSUB("Tick");

				CSector *pSector = GetSector(iMap, iSector);
				if ( pSector )
					pSector->OnTick(m_Sector_Pulse);
				else
					g_Log.EventError("Ticking invalid sector %d for map %d\n", iSector, iMap);

				EXC_CATCHSUB("Sector");
			}
		}

		// Process objects that need status updates
		// These objects are placed inside containers (which doesn't call tick updates) so they must be added to this list to call ticks manually
		if ( m_ObjStatusUpdates.GetCount() > 0 )
		{
			EXC_TRYSUB("Tick");

			// Loop backwards to avoid possible infinite loop if a status update is triggered as part of the status update (eg: property changed under tooltip trigger)
			size_t i = m_ObjStatusUpdates.GetCount();
			while ( i > 0 )
			{
				CObjBase *pObj = m_ObjStatusUpdates.GetAt(--i);
				if ( pObj )
					pObj->OnTickStatusUpdate();
			}
			m_ObjStatusUpdates.RemoveAll();

			EXC_CATCHSUB("StatusUpdates");
		}

		// Delete all queued objects
		m_ObjDelete.DeleteAll();
	}

	EXC_TRYSUB("Tick");
	m_TimedFunctions.OnTick();
	EXC_CATCHSUB("TimerFunction");

	if ( !m_fSaveNotificationSent && (m_timeSave - (10 * TICK_PER_SEC) <= GetCurrentTime()) )
	{
		// Notify all clients 10 seconds before world save starts
		Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSAVE_NOTIFY));
		m_fSaveNotificationSent = true;
	}
	if ( m_timeSave <= GetCurrentTime() )
	{
		// Save world
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;
		g_Log.Flush();
		Save(false);
	}
	if ( m_timeRespawn <= GetCurrentTime() )
	{
		// Respawn all dead NPCs on world
		m_timeRespawn = GetCurrentTime() + (20 * 60 * TICK_PER_SEC);
		RespawnDeadNPCs();
	}
	if ( g_Cfg.m_iTimerCall && (m_timeCallUserFunc < GetCurrentTime()) )
	{
		m_timeCallUserFunc = GetCurrentTime() + g_Cfg.m_iTimerCall;
		CScriptTriggerArgs Args(g_Cfg.m_iTimerCall / (60 * TICK_PER_SEC));
		g_Serv.r_Call("f_onserver_timer", &g_Serv, &Args);
	}
}

CSector *CWorld::GetSector(int iMap, int iSector)
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetSector");
	// Get an sector from map

	if ( (iMap < 0) || (iMap >= 256) || !g_MapList.m_maps[iMap] )
		return NULL;

	if ( iSector >= g_MapList.GetSectorQty(iMap) )
	{
		g_Log.EventError("Invalid sector %d for map %d specified\n", iSector, iMap);
		return NULL;
	}

	int iBase = 0;
	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		if ( m == iMap )
		{
			if ( iSector > g_MapList.GetSectorQty(iMap) )
				return NULL;

			return m_Sectors[iBase + iSector];
		}
		iBase += g_MapList.GetSectorQty(m);
	}
	return NULL;
}
