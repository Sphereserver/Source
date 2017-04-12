#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version
#include "../network/network.h"
#include "../network/send.h"

#if !defined( _WIN32 )
#include <time.h>
#endif

LPCTSTR GetReasonForGarbageCode(int iCode = -1)
{
	LPCTSTR pStr;
	switch ( iCode )
	{
		case -1:
		default:
			pStr = "Unknown";
			break;

		case 0x1102:
			pStr = "Disconnected char not acting as such";
			break;

		case 0x1103:
		case 0x1203:
			pStr = "Ridden NPC not acting as such";
			break;
			
		case 0x1104:
		case 0x1204:
			pStr = "Ridden NPC without a mount item";
			break;
			
		case 0x1105:
			pStr = "Ridden NPC with a mislinked mount item";
			break;
			
		case 0x1106:
			pStr = "Disconnected NPC neither dead nor ridden";
			break;

		case 0x1107:
		case 0x1205:
			pStr = "In game char that is neither a player nor an NPC";
			break;
			
		case 0x1108:
			pStr = "Char not on a valid position";
			break;
			
		case 0x2102:
			pStr = "Item without ITEMDEF";
			break;

		case 0x2103:
			pStr = "Item ITEMDEF with ID = 0";
			break;
			
		case 0x2104:
			pStr = "Disconnected item";
			break;
			
		case 0x2105:
			pStr = "Item not on a valid position";
			break;
			
		case 0x2106:
			pStr = "Item flagged as being in a container but it isn't";
			break;
			
		case 0x2202:
			pStr = "Item flagged as equipped but it isn't";
			break;
			
		case 0x2205:
			pStr = "Mislinked item";
			break;
			
		case 0x2206:
			pStr = "Gm Robe / Deathshroud not on a char";
			break;
			
		case 0x2207:
			pStr = "Deathshroud not on a dead char";
			break;
			
		case 0x2208:
			pStr = "Gm Robe on a char without privilege";
			break;
	
		case 0x2220:
			pStr = "Trade window memory not equipped in the correct layer or equipped on disconnected char";
			break;

		case 0x2221:
			pStr = "Client linger memory not equipped in the correct layer";
			break;
			
		case 0x2226:
			pStr = "Mount memory not equipped in the correct layer";
			break;
			
		case 0x2227:
		case 0x2228:
			pStr = "Hair/Beard item not equipped / not in a corpse / not in a vendor box";
			break;
			
		case 0x2229:
			pStr = "Game piece not in a game board";
			break;
			
		case 0x2230:
			pStr = "Item equipped in the trade window layer but it isn't a trade window";
			break;
			
		case 0x2231:
			pStr = "Item equipped in the memory layer but it isn't a memory";
			break;
			
		case 0x2233:
			pStr = "Item equipped in the mount memory layer but it isn't a mount memory";
			break;
			
		case 0x2234:
			pStr = "Item equipped in the client linger layer but it isn't a client linger memory";
			break;
			
		case 0x2235:
			pStr = "Item equipped in the murder memory layer but it isn't a murder memory";
			break;
			
		case 0x2236:
			pStr = "Item flagged as decay but without timer set";
			break;

		case 0x3101:
			pStr = "Object is totaly lost, no parent exists";
			break;

		case 0x3102:
			pStr = "Object was deleted or UID is incorrect";
			break;

		case 0x3201:
			pStr = "Object not correctly loaded by server (UID conflict)";
			break;

		case 0x3202:
			pStr = "Object not placed in the world";
			break;
			
		case 0x2222:	
		case 0x4222:
			pStr = "Memory not equipped / not in the memory layer / without color";
			break;
		
		case 0x4223:
			pStr = "Memory not on a char";
			break;
			
		case 0x4224:
			pStr = "Stone/Guild memory mislinked";
			break;
			
		case 0x4225:
			pStr = "Stone/Guild memory linked to the wrong stone";
			break;
			
		case 0xFFFF:
			pStr = "Bad memory allocation";
			break;
	}
	
	return pStr;
}

void ReportGarbageCollection(CObjBase * pObj, int iResultCode)
{
	ASSERT(pObj);
	DEBUG_ERR(("UID=0%lx, id=0%hx '%s', Invalid code=%0x (%s)\n", static_cast<DWORD>(pObj->GetUID()), pObj->GetBaseID(), pObj->GetName(), iResultCode, GetReasonForGarbageCode(iResultCode)));
}

//////////////////////////////////////////////////////////////////
// -CTimedFunctionHandler

CTimedFunctionHandler::CTimedFunctionHandler()
{
	m_curTick = 0;
	m_isBeingProcessed = false;
}

void CTimedFunctionHandler::OnTick()
{
	ADDTOCALLSTACK("CTimedFunctionHandler::OnTick");
	m_isBeingProcessed = true;

	m_curTick++;

	if ( m_curTick >= TICK_PER_SEC )
	{
		m_curTick = 0;
	}

	int tick = m_curTick;
	std::vector<TimedFunction *>::iterator it;
	ProfileTask scriptsTask(PROFILE_SCRIPTS);

	if ( m_timedFunctions[tick].size() > 0 )
	{
		for ( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
		{
			TimedFunction* tf = *it;
			tf->elapsed -= 1;
			if ( tf->elapsed <= 0 ) 
			{
				CScript s(tf->funcname);
				CObjBase * obj = tf->uid.ObjFind();
				int theEnd = 0;

				if ( obj != NULL ) //just in case
				{	
					CObjBaseTemplate * topobj = obj->GetTopLevelObj();
					CTextConsole* src;

					if ( topobj->IsChar() ) 
					{
						src = dynamic_cast <CTextConsole*> ( topobj );
					} 
					else 
					{
						src = &g_Serv;
					}

					m_tFrecycled.push_back( tf );
					//vector::erase crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
					if ( m_timedFunctions[tick].size()==1 )
					{
						m_timedFunctions[tick].pop_back();
						theEnd = 1;
					}
					else
					{
						it=m_timedFunctions[tick].erase( it );
					}

					obj->r_Verb( s, src );
				} 
				else 
				{
					m_tFrecycled.push_back( tf );
					//vector::erase crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
					if ( m_timedFunctions[tick].size()==1 )
					{
						m_timedFunctions[tick].pop_back();
						theEnd = 1;
					}
					else
					{
						it = m_timedFunctions[tick].erase( it );
					}
				}

				if (theEnd) 
				{
					break;
				}
			}
			else
			{
				++it;
			}
		}
	}
	
	m_isBeingProcessed = false;

	while ( m_tFqueuedToBeAdded.size() > 0 )
	{
		TimedFunction *tf = m_tFqueuedToBeAdded.back();
		m_tFqueuedToBeAdded.pop_back();
		m_timedFunctions[tick].push_back( tf );
	}
}

void CTimedFunctionHandler::Erase( CGrayUID uid )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Erase");
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
		{
			TimedFunction* tf = *it;
			if ( tf->uid == uid) 
			{
				m_tFrecycled.push_back( tf );
				//vector::erase crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
				if ( m_timedFunctions[tick].size()==1 )
				{
					m_timedFunctions[tick].pop_back();
					break;
				}
				else
				{
					it = m_timedFunctions[tick].erase( it );
				}
			}
			else
			{
				++it;
			}
		}
	}
}

int CTimedFunctionHandler::IsTimer( CGrayUID uid, LPCTSTR funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::IsTimer");
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
		{
			TimedFunction* tf = *it;
			if (( tf->uid == uid) && (!strcmpi( tf->funcname, funcname)))
				return tf->elapsed;

			++it;
		}
	}
	return 0;
}

void CTimedFunctionHandler::Stop( CGrayUID uid, LPCTSTR funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Stop");
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
		{
			TimedFunction* tf = *it;
			if (( tf->uid == uid) && (!strcmpi( tf->funcname, funcname)))
			{
				m_tFrecycled.push_back( tf );
				//vector::erase crashes if the iterator is pointing at the only thing left in the list. So, we check if size is 1 and do pop_back instead if that's the case. -SL
				if ( m_timedFunctions[tick].size()==1 )
				{
					m_timedFunctions[tick].pop_back();
					break;
				}
				else
				{
					it = m_timedFunctions[tick].erase( it );
				}
			}
			else
			{
				++it;
			}
		}
	}
}

TRIGRET_TYPE CTimedFunctionHandler::Loop(LPCTSTR funcname, int LoopsMade, CScriptLineContext StartContext, CScriptLineContext EndContext, CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Loop");
	bool endLooping = false;
	for (int tick = 0; tick < TICK_PER_SEC && endLooping == false; tick++)
	{
		std::vector<TimedFunction *>::iterator it;
		for (it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end() && endLooping == false;)
		{
			++LoopsMade;
			if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
			{
				g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", LoopsMade);
				return TRIGRET_ENDIF;
			}

			TimedFunction* tf = *it;
			if (!strcmpi(tf->funcname, funcname))
			{
				CObjBase * pObj = tf->uid.ObjFind();
				TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
				if (iRet == TRIGRET_BREAK)
				{
					EndContext = StartContext;
					endLooping = true;
					break;
				}
				if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
					return(iRet);
				if (iRet == TRIGRET_CONTINUE)
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);
			}
			++it;
		}
	}
	return TRIGRET_ENDIF;
}

void CTimedFunctionHandler::Add( CGrayUID uid, int numSeconds, LPCTSTR funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Add");
	ASSERT(funcname != NULL);
	ASSERT(strlen(funcname) < 1024);

	int tick = m_curTick;
	TimedFunction *tf;
	if ( m_tFrecycled.size() > 0 )
	{
		tf = m_tFrecycled.back();
		m_tFrecycled.pop_back();
	}
	else
	{
		tf = new TimedFunction;
	}
	tf->uid = uid;
	tf->elapsed = numSeconds;
	strcpy( tf->funcname, funcname );
	if ( m_isBeingProcessed )
	{
		m_tFqueuedToBeAdded.push_back( tf );
	}
	else
	{
		m_timedFunctions[tick].push_back( tf );
	}
}

int CTimedFunctionHandler::Load( const char *pszName, bool fQuoted, const char *pszVal)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Load");
	UNREFERENCED_PARAMETER(fQuoted);
	static char tempBuffer[1024];
	static TimedFunction *tf = NULL;

	if ( !pszName )
	{
		return -1;
	}

	if ( strcmpi( pszName, "CurTick" ) == 0 )
	{
		if ( IsDigit(pszVal[0] ) )
		{
			m_curTick = ATOI(pszVal);
		}
		else
		{
			g_Log.Event( LOGM_INIT|LOGL_ERROR,	"Invalid NextTick line in save file: %s=%s\n", pszName, pszVal );
		}
	}
	else if ( strcmpi( pszName, "TimerFNumbers" ) == 0 )
	{
		TCHAR * ppVal[4];
		strcpy( tempBuffer, pszVal );	//because pszVal is constant and Str_ParseCmds wants a non-constant string
		size_t iArgs = Str_ParseCmds( tempBuffer, ppVal, COUNTOF( ppVal ), " ,\t" );
		if ( iArgs == 3 )
		{
			if ( IsDigit( ppVal[0][0] ) && IsDigit( ppVal[1][0] ) && IsDigit( ppVal[2][0] ) )
			{
				int tick = ATOI(ppVal[0]);
				int uid = ATOI(ppVal[1]);
				int elapsed = ATOI(ppVal[2]);
				int isNew = 0;
				if ( tf == NULL )
				{
					tf = new TimedFunction;
					tf->funcname[0] = 0;
					isNew = 1;
				}
				tf->elapsed = elapsed;
				tf->uid.SetPrivateUID( uid );
				if ( !isNew )
				{
					m_timedFunctions[tick].push_back( tf );
					tf = NULL;
				}
				else
				{
					g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order\n", GRAY_FILE );
				}
			}
			else
			{
				g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s\n", GRAY_FILE, pszName, pszVal );
			}
		}
	}
	else if ( strcmpi( pszName, "TimerFCall" ) == 0 )
	{
		int isNew = 0;
		if ( tf == NULL )
		{
			tf = new TimedFunction;
			isNew = 1;
		}
		strcpy( tf->funcname, pszVal );
		if ( !isNew )
		{
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order\n", GRAY_FILE );
		}
	}

	return( 0 );
}

void CTimedFunctionHandler::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::r_Write");
	s.WriteKeyFormat( "CurTick", "%d", m_curTick );
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for ( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ++it )
		{
			TimedFunction *tf = *it;
			if ( tf->uid.IsValidUID() )
			{
				s.WriteKeyFormat("TimerFCall", "%s", tf->funcname);
				s.WriteKeyFormat("TimerFNumbers", "%d,%lu,%d", tick, tf->uid.GetObjUID(), tf->elapsed);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldSearch

CWorldSearch::CWorldSearch( const CPointMap & pt, int iDist ) :
	m_pt(pt), m_iDist(iDist)
{
	// define a search of the world.
	m_fAllShow = false;
	m_fSearchSquare = false;
	m_pObj = m_pObjNext = NULL;
	m_fInertToggle = false;

	m_pSectorBase = m_pSector = pt.GetSector();

	m_rectSector.SetRect( 
		pt.m_x - iDist,
		pt.m_y - iDist,
		pt.m_x + iDist + 1,
		pt.m_y + iDist + 1,
		pt.m_map);

	// Get upper left of search rect.
	m_iSectorCur = 0;
}

bool CWorldSearch::GetNextSector()
{
	ADDTOCALLSTACK("CWorldSearch::GetNextSector");
	// Move search into nearby CSector(s) if necessary

	if ( ! m_iDist )
		return( false );

	for (;;)
	{
		m_pSector = m_rectSector.GetSector(m_iSectorCur++);
		if ( m_pSector == NULL )
			return( false );	// done searching.
		if ( m_pSectorBase == m_pSector )
			continue;	// same as base.
		m_pObj = NULL;	// start at head of next Sector.
		return( true );
	}
}

CItem * CWorldSearch::GetItem()
{
	ADDTOCALLSTACK("CWorldSearch::GetItem");
	for (;;)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = static_cast<CObjBase *>(m_pSector->m_Items_Inert.GetHead());
		}
		else
		{
			m_pObj = m_pObjNext;
		}
		if ( m_pObj == NULL )
		{
			if ( ! m_fInertToggle )
			{
				m_fInertToggle = true;
				m_pObj = static_cast<CObjBase*>(m_pSector->m_Items_Timer.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fSearchSquare )
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistSightBase(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CItem *>(m_pObj);
			}
			else
			{
				if ( m_pt.GetDistSight(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CItem *>(m_pObj);
			}
		}
		else
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistBase(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CItem *>(m_pObj);
			}
			else
			{
				if ( m_pt.GetDist(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CItem *>(m_pObj);
			}
		}
	}
}

CChar * CWorldSearch::GetChar()
{
	ADDTOCALLSTACK("CWorldSearch::GetChar");
	for (;;)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = static_cast<CObjBase *>(m_pSector->m_Chars_Active.GetHead());
		}
		else
		{
			m_pObj = m_pObjNext;
		}
		if ( m_pObj == NULL )
		{
			if ( ! m_fInertToggle && m_fAllShow )
			{
				m_fInertToggle = true;
				m_pObj = static_cast<CObjBase *>(m_pSector->m_Chars_Disconnect.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fSearchSquare )
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistSightBase(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CChar *>(m_pObj);
			}
			else
			{
				if ( m_pt.GetDistSight(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CChar *>(m_pObj);
			}
		}
		else
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistBase(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CChar *>(m_pObj);
			}
			else
			{
				if ( m_pt.GetDist(m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast<CChar *>(m_pObj);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldThread

CWorldThread::CWorldThread()
{
	m_fSaveParity = false;		// has the sector been saved relative to the char entering it ?
	m_iUIDIndexLast = 1;

	m_FreeUIDs = (DWORD*)calloc(FREE_UIDS_SIZE, sizeof(DWORD));
	m_FreeOffset = FREE_UIDS_SIZE;
}

CWorldThread::~CWorldThread()
{
	CloseAllUIDs();
}

void CWorldThread::CloseAllUIDs()
{
	ADDTOCALLSTACK("CWorldThread::CloseAllUIDs");
	m_ObjDelete.DeleteAll();	// clean up our delete list.
	m_ObjNew.DeleteAll();
	m_UIDs.RemoveAll();

	if ( m_FreeUIDs != NULL )
	{
		free(m_FreeUIDs);
		m_FreeUIDs = NULL;
	}
	m_FreeOffset = FREE_UIDS_SIZE;
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
	if ( !dwIndex || dwIndex >= GetUIDCount() )
		return NULL;
	if ( m_UIDs[ dwIndex ] == UID_PLACE_HOLDER )	// unusable for now. (background save is going on)
		return NULL;
	return m_UIDs[dwIndex];
}

void CWorldThread::FreeUID(DWORD dwIndex)
{
	// Can't free up the UID til after the save !
	m_UIDs[dwIndex] = ( IsSaving()) ? UID_PLACE_HOLDER : NULL;
}

DWORD CWorldThread::AllocUID( DWORD dwIndex, CObjBase * pObj )
{
	ADDTOCALLSTACK("CWorldThread::AllocUID");
	DWORD dwCountTotal = GetUIDCount();

	if ( !dwIndex )					// auto-select tbe suitable hole
	{
		if ( !dwCountTotal )		// if the uids array is empty - increase it.
		{
			dwIndex = 1;
			goto setcount;
		}

		if ( m_FreeOffset < FREE_UIDS_SIZE && m_FreeUIDs != NULL )
		{
			//	We do have a free uid's array. Use it if possible to determine the first free element
			for ( ; m_FreeOffset < FREE_UIDS_SIZE && m_FreeUIDs[m_FreeOffset] != 0; m_FreeOffset++ )
			{
				//	yes, that's a free slot
				if ( !m_UIDs[m_FreeUIDs[m_FreeOffset]] )
				{
					dwIndex = m_FreeUIDs[m_FreeOffset++];
					goto successalloc;
				}
			}
		}
		m_FreeOffset = FREE_UIDS_SIZE;	// mark array invalid, since it does not contain any empty slots
										// use default allocation for a while, till the next garbage collection
		DWORD dwCount = dwCountTotal - 1;
		dwIndex = m_iUIDIndexLast;
		while ( m_UIDs[dwIndex] != NULL )
		{
			if ( ! -- dwIndex )
			{
				dwIndex = dwCountTotal - 1;
			}
			if ( ! -- dwCount )
			{
				dwIndex = dwCountTotal;
				goto setcount;
			}
		}
	}
	else if ( dwIndex >= dwCountTotal )
	{
setcount:
		// We have run out of free UID's !!! Grow the array
		m_UIDs.SetCount(( dwIndex + 0x1000 ) &~ 0xFFF );
	}

successalloc:
	m_iUIDIndexLast = dwIndex; // start from here next time so we have even distribution of allocation.
	CObjBase	*pObjPrv = m_UIDs[dwIndex];
	if ( pObjPrv )
	{
		//NOTE: We cannot use Delete() in here because the UID will
		//	still be assigned til the async cleanup time. Delete() will not work here!
		DEBUG_ERR(("UID conflict delete 0%lx, '%s'\n", dwIndex, pObjPrv->GetName()));
		delete pObjPrv;
	}
	m_UIDs[dwIndex] = pObj;
	return dwIndex;
}

void CWorldThread::SaveThreadClose()
{
	ADDTOCALLSTACK("CWorldThread::SaveThreadClose");
	for ( size_t i = 1; i < GetUIDCount(); i++ )
	{
		if ( m_UIDs[i] == UID_PLACE_HOLDER )
			m_UIDs[i] = NULL;
	}

	m_FileData.Close();
	m_FileWorld.Close();
	m_FilePlayers.Close();
	m_FileMultis.Close();
}

int CWorldThread::FixObjTry( CObjBase * pObj, DWORD dwUID )
{
	ADDTOCALLSTACK_INTENSIVE("CWorldThread::FixObjTry");
	// RETURN: 0 = success.
	if ( !pObj )
		return 0x7102;

	if ( dwUID != 0 )
	{
		if (( pObj->GetUID() & UID_O_INDEX_MASK ) != dwUID )
		{
			// Miss linked in the UID table !!! BAD
			// Hopefully it was just not linked at all. else How the hell should i clean this up ???
			DEBUG_ERR(("UID 0%lx, '%s', Mislinked\n", dwUID, pObj->GetName()));
			return 0x7101;
		}
	}

	return pObj->FixWeirdness();
}

int CWorldThread::FixObj( CObjBase * pObj, DWORD dwUID )
{
	ADDTOCALLSTACK("CWorldThread::FixObj");
	// Attempt to fix problems with this item.
	// Ignore any children it may have for now.
	// RETURN: 0 = success.
	//

	int iResultCode;

	try
	{
		iResultCode = FixObjTry(pObj, dwUID);
	}
	catch ( const CGrayError& e ) // catch all
	{
		g_Log.CatchEvent( &e, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(NULL, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	if ( iResultCode == 0 )
		return 0;

	try
	{
		dwUID = static_cast<DWORD>(pObj->GetUID());

		// is it a real error ?
		if ( pObj->IsItem())
		{
			CItem * pItem = dynamic_cast <CItem*>(pObj);
			if ( pItem != NULL && pItem->IsType(IT_EQ_MEMORY_OBJ) )
			{
				pObj->Delete();
				return iResultCode;
			}
		}

		ReportGarbageCollection(pObj, iResultCode);
		pObj->Delete();
	}
	catch ( const CGrayError& e )	// catch all
	{
		g_Log.CatchEvent( &e, "UID=0%lx, Asserted cleanup", dwUID );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "UID=0%lx, Asserted cleanup", dwUID );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return( iResultCode );
}

void CWorldThread::GarbageCollection_New()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_New");
	EXC_TRY("GarbageCollection_New");
	// Clean up new objects that are never placed.
	if ( m_ObjNew.GetCount() > 0 )
	{
		g_Log.Event( LOGL_ERROR, "GC: %" FMTSIZE_T " unplaced object deleted\n", m_ObjNew.GetCount());

		for (size_t i = 0; i < m_ObjNew.GetCount(); ++i)
		{
			CObjBase * pObj = dynamic_cast<CObjBase*>(m_ObjNew.GetAt(i));
			if (pObj == NULL)
				continue;

			ReportGarbageCollection(pObj, 0x3202);
			pObj->Delete();
		}

		m_ObjNew.DeleteAll();
	}
	m_ObjDelete.DeleteAll();	// clean up our delete list.

	// Make sure all GM pages have accounts.
	CGMPage *pPage = static_cast<CGMPage *>(g_World.m_GMPages.GetHead());
	while ( pPage != NULL )
	{
		CGMPage * pPageNext = pPage->GetNext();
		if ( ! pPage->FindAccount()) // Open script file
		{
			DEBUG_ERR(("GC: GM Page has invalid account '%s'\n", pPage->GetName()));
			delete pPage;
		}
		pPage = pPageNext;
	}
	EXC_CATCH;
}

void CWorldThread::GarbageCollection_UIDs()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_UIDs");
	// Go through the m_ppUIDs looking for Objects without links to reality.
	// This can take a while.

	GarbageCollection_New();

	size_t iCount = 0;
	for ( size_t i = 1; i < GetUIDCount(); i++ )
	{
		try
		{
			CObjBase * pObj = m_UIDs[i];
			if ( !pObj || pObj == UID_PLACE_HOLDER )
				continue;

			// Look for anomalies and fix them (that might mean delete it.)
			int iResultCode = FixObj(pObj, i);
			if ( iResultCode )
			{
				// Do an immediate delete here instead of Delete()
				delete pObj;
				FreeUID(i);	// Get rid of junk uid if all fails..
				continue;
			}

			if ((iCount & 0x1FF ) == 0)
			{
				g_Serv.PrintPercent(iCount, GetUIDCount());
			}
			iCount ++;
		}
		catch ( const CGrayError& e )
		{
			g_Log.CatchEvent(&e, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch(...)
		{
			g_Log.CatchEvent(NULL, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	GarbageCollection_New();

	if ( iCount != CObjBase::sm_iCount )	// All objects must be accounted for.
	{
		g_Log.Event(LOGL_ERROR, "GC: Object memory leak %" FMTSIZE_T "!=%" FMTSIZE_T "\n", iCount, CObjBase::sm_iCount);
	}
	else
	{
		g_Log.Event(LOGL_EVENT, "GC: %" FMTSIZE_T " objects accounted for\n", iCount);
	}

	if ( m_FreeUIDs != NULL )	// new UID engine - search for empty holes and store it in a huge array
	{							// the size of the array should be enough even for huge shards
								// to survive till next garbage collection
		memset(m_FreeUIDs, 0, FREE_UIDS_SIZE * sizeof(DWORD));
		m_FreeOffset = 0;

		for ( DWORD d = 1; d < GetUIDCount(); d++ )
		{
			CObjBase	*pObj = m_UIDs[d];

			if ( !pObj )
			{
				if ( m_FreeOffset >= ( FREE_UIDS_SIZE - 1 ))
					break;

				m_FreeUIDs[m_FreeOffset++] = d;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldClock

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
	return ((ts.tv_sec * 10000) + (ts.tv_nsec / 100000)) / 10;
#endif
}

void CWorldClock::InitTime( UINT64 lTimeBase )
{
	ADDTOCALLSTACK("CWorldClock::InitTime");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock.InitTime(lTimeBase);
}

void CWorldClock::Init()
{
	ADDTOCALLSTACK("CWorldClock::Init");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock.Init();
}

bool CWorldClock::Advance()
{
	ADDTOCALLSTACK("CWorldClock::Advance");
	UINT64 Clock_Sys = GetSystemClock();

	INT64 iTimeDiff = Clock_Sys - m_Clock_SysPrev;
	iTimeDiff = IMULDIVDOWN(TICK_PER_SEC, iTimeDiff, CLOCKS_PER_SEC);
	if ( !iTimeDiff )
		return false;
	else if ( iTimeDiff < 0 )
	{
		// System clock has changed forward
		// Just wait until next cycle and it should be ok
		g_Log.Event(LOGL_WARN, "System clock has changed forward (daylight saving change, etc). This may cause strange behavior on some objects\n");
		m_Clock_SysPrev = Clock_Sys;
		return false;
	}

	m_Clock_SysPrev = Clock_Sys;
	CServTime Clock_New = m_timeClock + iTimeDiff;

	// CServTime is signed !
	// NOTE: This will overflow after 7 or so years of run time !
	if ( Clock_New < m_timeClock )
	{
		// System clock has changed backward
		g_Log.Event(LOGL_WARN, "System clock has changed backward (daylight saving change, etc). This may cause strange behavior on some objects\n");
		m_timeClock = Clock_New;
		return false;
	}

	m_timeClock = Clock_New;
	return true;
}

//////////////////////////////////////////////////////////////////
// -CWorld

CWorld::CWorld()
{
	m_savetimer = 0;
	m_iSaveCountID = 0;
	m_iSaveStage = 0;
	m_iPrevBuild = 0;
	m_iLoadVersion = 0;
	m_bSaveNotificationSent = false;
	m_timeSector.Init();
	m_timeSave.Init();
	m_timeRespawn.Init();
	m_timeStartup.Init();
	m_timeCallUserFunc.Init();

	m_Sectors = NULL;
	m_SectorsQty = 0;
	m_Sector_Pulse = 0;
}

void CWorld::Init()
{
	EXC_TRY("Init");

	if ( m_Sectors )	//	disable changes on-a-fly
		return;

	g_MapList.Init();
	if ( g_MapList.m_pMapDiffCollection )
		g_MapList.m_pMapDiffCollection->Init();

	//	initialize all sectors
	int	sectors = 0;
	int m = 0;
	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;
		sectors += g_MapList.GetSectorQty(m);
	}

	m_Sectors = new CSector*[sectors];
	TemporaryString pszSectors;
	TemporaryString pszMapSectors;
	TCHAR *pszMaps = Str_GetTemp();
	TCHAR *pszMapName = Str_GetTemp();

	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		// Get map name
		switch ( m )
		{
			case 0:		sprintf(pszMapName, "%d='Felucca'", m);			break;
			case 1:		sprintf(pszMapName, "%d='Trammel'", m);			break;
			case 2:		sprintf(pszMapName, "%d='Ilshenar'", m);		break;
			case 3:		sprintf(pszMapName, "%d='Malas'", m);			break;
			case 4:		sprintf(pszMapName, "%d='Tokuno Islands'", m);	break;
			case 5:		sprintf(pszMapName, "%d='Ter Mur'", m);			break;
			default:	sprintf(pszMapName, "%d='[Custom]'", m);		break;
		}
		if ( *pszMaps )
			strcat(pszMaps, ", ");
		strcat(pszMaps, pszMapName);

		// Initialize sectors
		sprintf(pszMapSectors, " %d='%d'", m, g_MapList.GetSectorQty(m));
		strcat(pszSectors, pszMapSectors);
		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = new CSector;
			ASSERT(pSector);
			pSector->Init(s, m);
			m_Sectors[m_SectorsQty++] = pSector;
		}
	}
	ASSERT(m_SectorsQty);

	if ( *pszMaps )
		g_Log.Event(LOGM_INIT, "Expansion maps supported by your MUL files: %s\n", pszMaps);
	g_Log.Event(LOGM_INIT, "Allocating map sectors:%s\n\n", static_cast<LPCTSTR>(pszSectors));
	EXC_CATCH;
}

CWorld::~CWorld()
{
	Close();
}

///////////////////////////////////////////////
// Loading and Saving.

void CWorld::GetBackupName( CGString & sArchive, LPCTSTR pszBaseDir, TCHAR chType, int iSaveCount ) // static
{
	ADDTOCALLSTACK("CWorld::GetBackupName");
	int iCount = iSaveCount;
	int iGroup = 0;
	for ( ; iGroup<g_Cfg.m_iSaveBackupLevels; iGroup++ )
	{
		if ( iCount & 0x7 )
			break;
		iCount >>= 3;
	}
	sArchive.Format("%s" GRAY_FILE "b%d%d%c%s", pszBaseDir, iGroup, iCount & 0x07, chType, GRAY_SCRIPT);
}

bool CWorld::OpenScriptBackup( CScript & s, LPCTSTR pszBaseDir, LPCTSTR pszBaseName, int iSaveCount ) // static
{
	ADDTOCALLSTACK("CWorld::OpenScriptBackup");
	ASSERT(pszBaseName);

	CGString sArchive;
	GetBackupName( sArchive, pszBaseDir, pszBaseName[0], iSaveCount );

	// remove possible previous archive of same name
	remove( sArchive );

	// rename previous save to archive name.
	CGString sSaveName;
	sSaveName.Format( "%s" GRAY_FILE "%s%s", pszBaseDir, pszBaseName, GRAY_SCRIPT );

	rename(sSaveName, sArchive);
	//if ( rename(sSaveName, sArchive) )	// may not exist if this is the first time
	//	g_Log.Event(LOGM_SAVE|LOGL_WARN, "Rename %s to '%s' FAILED code %d?\n", static_cast<LPCTSTR>(sSaveName), static_cast<LPCTSTR>(sArchive), CGFile::GetLastError());

	if ( ! s.Open( sSaveName, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
	{
		g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Save '%s' FAILED\n", static_cast<LPCTSTR>(sSaveName));
		return( false );
	}

	return( true );
}

bool CWorld::SaveStage() // Save world state in stages.
{
	ADDTOCALLSTACK("CWorld::SaveStage");
	// Do the next stage of the save.
	// RETURN: true = continue; false = done.
	EXC_TRY("SaveStage");
	bool bRc = true;

	if ( m_iSaveStage == -1 ) 
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
		{
			GarbageCollection_New();
		}
	}
	else if ( m_iSaveStage < static_cast<int>(m_SectorsQty) )
	{
		// NPC Chars in the world secors and the stuff they are carrying.
		// Sector lighting info.
		if(IsSetEF(EF_Dynamic_Backsave))
		{
			unsigned int szComplexity = 0;

			CSector *s = m_Sectors[m_iSaveStage];
			if( s )
			{
				s->r_Write();
				szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();
			}
			unsigned int dynStage = m_iSaveStage+1;
			if( szComplexity <= g_Cfg.m_iSaveStepMaxComplexity )
			{
				unsigned int szSectorCnt = 1;
				while(dynStage < m_SectorsQty && szSectorCnt <= g_Cfg.m_iSaveSectorsPerTick)
				{
					s = m_Sectors[dynStage];
					if ( s )
					{
						szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();

						if(szComplexity <= g_Cfg.m_iSaveStepMaxComplexity)
						{
							s->r_Write();
							m_iSaveStage = dynStage;
							szSectorCnt++;
						}
						else
						{
							break;
						}
					}
					dynStage++;
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
		m_FileData.WriteSection( "TIMERF" );
		g_World.m_TimedFunctions.r_Write(m_FileData);

		m_FileData.WriteSection("GLOBALS");
		g_Exp.m_VarGlobals.r_WritePrefix(m_FileData, NULL);

		g_Exp.m_ListGlobals.r_WriteSave(m_FileData);

		size_t iQty = g_Cfg.m_RegionDefs.GetCount();
		for ( size_t i = 0; i < iQty; i++ )
		{
			CRegionBase *pRegion = dynamic_cast <CRegionBase*> (g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			m_FileData.WriteSection("WORLDSCRIPT %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		// GM_Pages.
		CGMPage *pPage = static_cast<CGMPage *>(m_GMPages.GetHead());
		for ( ; pPage != NULL; pPage = pPage->GetNext())
		{
			pPage->r_Write(m_FileData);
		}
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty)+1 )
	{
		//	Empty save stage
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty)+2 )
	{
		// Now make a backup of the account file.
		bRc = g_Accounts.Account_SaveAll();
	}
	else if ( m_iSaveStage == static_cast<int>(m_SectorsQty)+3 )
	{
		// EOF marker to show we reached the end.
		m_FileData.WriteSection("EOF");
		m_FileWorld.WriteSection("EOF");
		m_FilePlayers.WriteSection("EOF");
		m_FileMultis.WriteSection("EOF");

		m_iSaveCountID++;	// Save only counts if we get to the end winout trapping.
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time.

		g_Log.Event(LOGM_SAVE, "World data saved   (%s)\n", static_cast<LPCTSTR>(m_FileWorld.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "Player data saved  (%s)\n", static_cast<LPCTSTR>(m_FilePlayers.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "Multi data saved   (%s)\n", static_cast<LPCTSTR>(m_FileMultis.GetFilePath()));
		g_Log.Event(LOGM_SAVE, "Context data saved (%s)\n", static_cast<LPCTSTR>(m_FileData.GetFilePath()));

		ULONGLONG llTicksStart = m_savetimer, llTicksEnd;
		TIME_PROFILE_END;

		TCHAR * time = Str_GetTemp();
		sprintf(time, "%lld.%04lld", static_cast<INT64>(TIME_PROFILE_GET_HI/1000), static_cast<INT64>(TIME_PROFILE_GET_LO));

		g_Log.Event(LOGM_SAVE, "World save completed, took %s seconds\n", time);

		CScriptTriggerArgs Args;
		Args.Init(time);
		g_Serv.r_Call("f_onserver_save_finished", &g_Serv, &Args);

		// Now clean up all the held over UIDs
		SaveThreadClose();
		m_iSaveStage = INT_MAX;
		return false;
	}

	if ( g_Cfg.m_iSaveBackgroundTime )
	{
		int iNextTime = g_Cfg.m_iSaveBackgroundTime / m_SectorsQty;
		if ( iNextTime > TICK_PER_SEC/2 )
			iNextTime = TICK_PER_SEC/2;	// max out at 30 minutes or so.
		m_timeSave = GetCurrentTime() + iNextTime;
	}
	m_iSaveStage++;
	return bRc;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("stage '%d' qty '%u' time '%lld'\n", m_iSaveStage, m_SectorsQty, m_timeSave.GetTimeRaw());
	EXC_DEBUG_END;

	m_iSaveStage++;	// to avoid loops, we need to skip the current operation in world save
	return false;
}

bool CWorld::SaveForce() // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveForce");
	Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE ) );
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive() == false)
		g_NetworkOut.flushAll();
#else
	if (g_NetworkManager.isOutputThreaded() == false)
		g_NetworkManager.flushAllClients();
#endif

	g_Serv.SetServerMode( SERVMODE_Saving );	// Forced save freezes the system.
	bool	bSave = true;
	bool	bSuccess = true;

	static LPCTSTR const msgs[] =
	{
		"garbage collection",
		"sectors",
		"global variables, regions, gmpages",
		"servers",
		"accounts",
		""
	};
	const char *pCurBlock = msgs[0];

	while ( bSave )
	{
		try
		{
			if (( m_iSaveStage >= 0 ) && ( m_iSaveStage < static_cast<int>(m_SectorsQty) ))
				pCurBlock = msgs[1];
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty) )
				pCurBlock = msgs[2];
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty)+1 )
				pCurBlock = msgs[3];
			else if ( m_iSaveStage == static_cast<int>(m_SectorsQty)+2 )
				pCurBlock = msgs[4];
			else
				pCurBlock = msgs[5];

			bSave = SaveStage();
			if (! ( m_iSaveStage & 0x1FF ))
			{
				g_Serv.PrintPercent( m_iSaveStage, m_SectorsQty+3 );
			}
			if ( !bSave && ( pCurBlock != msgs[5] ))
				goto failedstage;
		}
		catch ( const CGrayError& e )
		{
			g_Log.CatchEvent(&e, "Save FAILED for stage %d (%s)", m_iSaveStage, pCurBlock);
			bSuccess = false;
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
			goto failedstage;
		}
		continue;
failedstage:
		g_Log.CatchEvent( NULL, "Save FAILED for stage %d (%s)", m_iSaveStage, pCurBlock);
		bSuccess = false;
	}

	g_Serv.SetServerMode(SERVMODE_Run);			// Game is up and running
	return bSuccess;
}

bool CWorld::SaveTry( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveTry");
	EXC_TRY("SaveTry");
	if ( m_FileWorld.IsFileOpen())
	{
		// Save is already active !
		ASSERT( IsSaving());
		if ( fForceImmediate )	// finish it now !
		{
			return SaveForce();
		}
		else if ( g_Cfg.m_iSaveBackgroundTime )
		{
			return SaveStage();
		}
		return false;
	}

	// Do the write async from here in the future.
	if ( g_Cfg.m_fSaveGarbageCollect )
	{
		GarbageCollection();
	}

	ULONGLONG llTicksStart;
	TIME_PROFILE_START;
	m_savetimer = llTicksStart;

	// Determine the save name based on the time.
	// exponentially degrade the saves over time.
	if ( !OpenScriptBackup(m_FileData, g_Cfg.m_sWorldBaseDir, "data", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FileWorld, g_Cfg.m_sWorldBaseDir, "world", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FilePlayers, g_Cfg.m_sWorldBaseDir, "chars", m_iSaveCountID) )
		return false;
	if ( !OpenScriptBackup(m_FileMultis, g_Cfg.m_sWorldBaseDir, "multis", m_iSaveCountID) )
		return false;

	m_fSaveParity = ! m_fSaveParity; // Flip the parity of the save.
	m_iSaveStage = -1;
	m_bSaveNotificationSent = false;
	m_timeSave.Init();

	// Write the file headers.
	r_Write(m_FileData);
	r_Write(m_FileWorld);
	r_Write(m_FilePlayers);
	r_Write(m_FileMultis);

	if ( fForceImmediate || !g_Cfg.m_iSaveBackgroundTime )	// Save now !
		return SaveForce();

	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("immediate '%d'\n", fForceImmediate ? 1 : 0);
	EXC_DEBUG_END;
	return false;
}

bool CWorld::Save( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::Save");
	bool	bSaved = false;
	try
	{
		CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
		enum TRIGRET_TYPE tr;

		if ( g_Serv.r_Call("f_onserver_save", &g_Serv, &Args, NULL, &tr) )
			if ( tr == TRIGRET_RET_TRUE ) 
				return false;
		//Fushing before the server should fix #2306
		//The scripts fills the clients buffer and the server flush
		//the data during the save.
		//Should we flush only non threaded output or force it 
		//to flush on any conditions?

#ifndef _MTNETWORK
		if (g_NetworkOut.isActive() == false) {
#else
		if (g_NetworkManager.isOutputThreaded() == false) {
#endif

#ifdef _DEBUG 
			g_Log.EventDebug("Flushing %lu client(s) output data...\n", g_Serv.StatGet(SERV_STAT_CLIENTS));
#endif

#ifndef _MTNETWORK
			g_NetworkOut.flushAll();
#else
			g_NetworkManager.flushAllClients();
#endif

#ifdef _DEBUG
			g_Log.EventDebug("Done flushing clients output data\n");
#endif
		}
			
		fForceImmediate = (Args.m_iN1 != 0);
		bSaved = SaveTry(fForceImmediate);
	}
	catch ( const CGrayError& e )
	{
		g_Log.CatchEvent( &e, "Save FAILED" );
		Broadcast("Save FAILED. " GRAY_TITLE " is UNSTABLE!");
		m_FileData.Close();	// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "Save FAILED" );
		Broadcast("Save FAILED. " GRAY_TITLE " is UNSTABLE!");
		m_FileData.Close();	// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
	g_Serv.r_Call((bSaved?"f_onserver_save_ok":"f_onserver_save_fail"), &g_Serv, &Args);
	return bSaved;
}

void CWorld::SaveStatics()
{
	ADDTOCALLSTACK("CWorld::SaveStatics");
	try
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_New();

		CScript m_FileStatics;
		if ( !OpenScriptBackup(m_FileStatics, g_Cfg.m_sWorldBaseDir, "statics", m_iSaveCountID) )
			return;
		r_Write(m_FileStatics);

		Broadcast( g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSTATICSAVE) );
#ifndef _MTNETWORK
		if (g_NetworkOut.isActive() == false)
			g_NetworkOut.flushAll();
#else
		if (g_NetworkManager.isOutputThreaded() == false)
			g_NetworkManager.flushAllClients();
#endif

		//	loop through all sectors and save static items
		CSector *pSector;
		CItem *pItem, *pNext;
		for ( int m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] )
				continue;

			for ( int d = 0; d < g_MapList.GetSectorQty(m); d++ )
			{
				pSector = GetSector(m, d);
				if ( !pSector )
					continue;

				pItem = static_cast<CItem *>(pSector->m_Items_Inert.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( !pItem->IsAttr(ATTR_STATIC) || pItem->IsType(IT_MULTI_CUSTOM) )
						continue;

					pItem->r_WriteSafe(m_FileStatics);
				}

				pItem = static_cast<CItem *>(pSector->m_Items_Timer.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( !pItem->IsAttr(ATTR_STATIC) || pItem->IsType(IT_MULTI_CUSTOM) )
						continue;
					
					pItem->r_WriteSafe(m_FileStatics);
				}
			}
		}

		m_FileStatics.WriteSection( "EOF" );
		m_FileStatics.Close();
		g_Log.Event(LOGM_SAVE, "Statics data saved (%s)\n", static_cast<LPCTSTR>(m_FileStatics.GetFilePath()));
	}
	catch (const CGrayError& e)
	{
		g_Log.CatchEvent(&e, "Statics save FAILED");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)
	{
		g_Log.CatchEvent(NULL, "Statics save FAILED");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
}

/////////////////////////////////////////////////////////////////////

bool CWorld::LoadFile(LPCTSTR pszLoadName)	// Load world from script
{
	g_Log.Event(LOGM_INIT, "Loading %s%s\n", static_cast<LPCTSTR>(pszLoadName), GRAY_SCRIPT);

	CScript s;
	if ( !s.Open(pszLoadName, OF_READ|OF_TEXT|OF_DEFAULTMODE) )
		return false;

	// Find the size of the file.
	DWORD lLoadSize = s.GetLength();
	int iLoadStage = 0;

	CScriptFileContext ScriptContext( &s );

	// Read the header stuff first.
	CScriptObj::r_Load( s );

	while ( s.FindNextSection())
	{
		if (! ( ++iLoadStage & 0x1FF ))	// don't update too often
		{
			g_Serv.PrintPercent( s.GetPosition(), lLoadSize );
		}

		try
		{
			g_Cfg.LoadResourceSection(&s);
		}
		catch ( const CGrayError& e )
		{
			g_Log.CatchEvent(&e, "Load Exception line %d " GRAY_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			g_Log.CatchEvent(NULL, "Load Exception line %d " GRAY_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	if ( s.IsSectionType( "EOF" ))
	{
		// The only valid way to end.
		s.Close();
		return( true );
	}

	g_Log.Event( LOGM_INIT|LOGL_CRIT, "No [EOF] marker. '%s' is corrupt!\n", static_cast<LPCTSTR>(s.GetFilePath()));
	return( false );
}


void CWorld::LoadWorld() // Load world from script
{
	EXC_TRY("LoadWorld");
	// Auto change to the most recent previous backup !
	// Try to load a backup file instead ?
	// NOTE: WE MUST Sync these files ! CHAR and WORLD !!!

	CGString sStaticsName;
	sStaticsName.Format("%s" GRAY_FILE "statics", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));

	CGString sWorldName;
	sWorldName.Format("%s" GRAY_FILE "world", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));

	CGString sMultisName;
	sMultisName.Format("%s" GRAY_FILE "multis", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));

	CGString sCharsName;
	sCharsName.Format("%s" GRAY_FILE "chars", static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));

	CGString sDataName;
	sDataName.Format("%s" GRAY_FILE "data",	static_cast<LPCTSTR>(g_Cfg.m_sWorldBaseDir));

	long iPrevSaveCount = m_iSaveCountID;
	for (;;)
	{
		LoadFile(sDataName);
		LoadFile(sStaticsName);
		LoadFile(sMultisName);
		if ( LoadFile(sWorldName) )
		{
			if ( LoadFile(sCharsName) )
				return;
		}

		// If we could not open the file at all then it was a bust!
		if ( m_iSaveCountID == iPrevSaveCount ) break;

		// Reset everything that has been loaded
		m_Stones.RemoveAll();
		m_ObjStatusUpdates.RemoveAll();
		m_Parties.DeleteAll();
		m_GMPages.DeleteAll();

		if ( m_Sectors )
		{
			for ( unsigned int s = 0; s < m_SectorsQty; s++ )
			{
				// Remove everything from the sectors
				m_Sectors[s]->Close();
			}
		}

		CloseAllUIDs();
		m_Clock.Init();
		m_UIDs.SetCount(8 * 1024);

		// Get the name of the previous backups.
		CGString sArchive;
		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'w', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sWorldName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sWorldName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'c', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sCharsName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sCharsName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'm', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sMultisName ))
		{
			break;
		}
		sMultisName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'd', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sDataName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sDataName = sArchive;
	}

	EXC_CATCH;
}


void CWorld::LoadAll() // Load world from script
{
	// start count. (will grow as needed)
	m_UIDs.SetCount(8 * 1024);
	m_Clock.Init();		// will be loaded from the world file.

	// Load all the accounts.
	g_Accounts.Account_LoadAll(false);

	// Try to load the world and chars files .
	LoadWorld();

	m_timeStartup = GetCurrentTime();
	m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time.

	// Set all the sector light levels now that we know the time.
	// This should not look like part of the load. (CTRIG_EnvironChange triggers should run)
	size_t iCount;
	for ( unsigned int s = 0; s < m_SectorsQty; s++ )
	{
		EXC_TRYSUB("Load");
		CSector *pSector = m_Sectors[s];

		if ( pSector != NULL )
		{
			if ( !pSector->IsLightOverriden() )
				pSector->SetLight(-1);

			// Is this area too complex ?
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

	// Set the current version now.
	r_SetVal("VERSION", GRAY_VERSION);	// Set m_iLoadVersion
}

/////////////////////////////////////////////////////////////////

void CWorld::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CWorld::r_Write");
	// Write out the safe header.
	s.WriteKey("TITLE", GRAY_TITLE " World Script");
	s.WriteKey("VERSION", GRAY_VERSION);
	s.WriteKeyVal("PREVBUILD", GRAY_VER_BUILD);
	s.WriteKeyVal( "TIME", GetCurrentTime().GetTimeRaw() );
	s.WriteKeyVal( "SAVECOUNT", m_iSaveCountID );
	s.Flush();	// Force this out to the file now.
}

bool CWorld::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CWorld::r_GetRef");
	if ( ! strnicmp( pszKey, "LASTNEW", 7 ))
	{
		if ( ! strnicmp( pszKey+7, "ITEM", 4 ))
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewItem.ItemFind();
			return( true );
		}
		if ( ! strnicmp( pszKey+7, "CHAR", 4 ))
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewChar.CharFind();
			return( true );
		}
	}
	return( false );
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

LPCTSTR const CWorld::sm_szLoadKeys[WC_QTY+1] =	// static
{
	"PREVBUILD",
	"SAVECOUNT",
	"TIME",
	"TITLE",
	"VERSION",
	NULL
};

bool CWorld::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CWorld::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		pszKey += 6;
		if (( *pszKey == 'S' ) || ( *pszKey == 's' ))	//	SERV.GMPAGES
		{
			pszKey++;
			if ( *pszKey != '\0' )
				return false;

			sVal.FormatVal(m_GMPages.GetCount());
		}
		else if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPARATORS(pszKey);
			size_t index = Exp_GetVal(pszKey);
			if ( index >= m_GMPages.GetCount() )
				return false;

			SKIP_SEPARATORS(pszKey);
			CGMPage *pPage = static_cast<CGMPage *>(m_GMPages.GetAt(index));
			if ( pPage == NULL )
				return false;

			if ( !strcmpi(pszKey, "HANDLED") )
			{
				CClient *pClient = pPage->FindGMHandler();
				if ( pClient != NULL && pClient->GetChar() != NULL )
					sVal.FormatHex(pClient->GetChar()->GetUID());
				else
					sVal.FormatVal(0);
				return true;
			}
			else
				return pPage->r_WriteVal(pszKey, sVal, pSrc);
		}
		else
			sVal.FormatVal(0);
		return true;
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case WC_PREVBUILD:
			sVal.FormatVal(m_iPrevBuild);
			break;
		case WC_SAVECOUNT: // "SAVECOUNT"
			sVal.FormatVal( m_iSaveCountID );
			break;
		case WC_TIME:	// "TIME"
			sVal.FormatLLVal( GetCurrentTime().GetTimeRaw() );
			break;
		case WC_TITLE: // 	"TITLE",
			sVal = (GRAY_TITLE " World Script");
			break;
		case WC_VERSION: // "VERSION"
			sVal = GRAY_VERSION;
			break;
		default:
			return( false );
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CWorld::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	LPCTSTR	pszKey = s.GetKey();
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case WC_PREVBUILD:
			m_iPrevBuild = s.GetArgVal();
			break;
		case WC_SAVECOUNT: // "SAVECOUNT"
			m_iSaveCountID = s.GetArgVal();
			break;
		case WC_TIME:	// "TIME"
			if ( ! g_Serv.IsLoading() )
			{
				DEBUG_WARN(( "Setting TIME while running is BAD!\n" ));
			}
			m_Clock.InitTime( s.GetArgLLVal());
			break;
		case WC_VERSION: // "VERSION"
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
	// Respawn dead story NPC's
	g_Serv.SetServerMode(SERVMODE_RestockAll);
	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); ++s )
		{
			CSector	*pSector = GetSector(m, s);

			if ( pSector )
				pSector->RespawnDeadNPCs();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::Restock()
{
	ADDTOCALLSTACK("CWorld::Restock");
	// Recalc all the base items as well.
	g_Serv.SetServerMode(SERVMODE_RestockAll);

	for ( size_t i = 0; i < COUNTOF(g_Cfg.m_ResHash.m_Array); ++i )
	{
		for ( size_t j = 0; j < g_Cfg.m_ResHash.m_Array[i].GetCount(); ++j )
		{
			CResourceDef * pResDef = g_Cfg.m_ResHash.m_Array[i][j];
			if ( pResDef == NULL || ( pResDef->GetResType() != RES_ITEMDEF ))
				continue;

			CItemBase * pBase = dynamic_cast<CItemBase *>(pResDef);
			if ( pBase != NULL )
				pBase->Restock();
		}
	}

	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); ++s )
		{
			CSector	*pSector = GetSector(m, s);
			if ( pSector != NULL )
				pSector->Restock();
		}
	}

	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::ResyncMultiRegions()
{
	ADDTOCALLSTACK("CWorld::ResyncMultiRegions");
	// Loop through all multi items on world to reload the multi region.
	// This must be called after server resync, when dynamic regions from multis
	// already placed on world got replaced by static regions loaded from scripts
	CSector	*pSector = NULL;
	CItem *pItem = NULL;
	CItemMulti *pMulti = NULL;

	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		for ( int d = 0; d < g_MapList.GetSectorQty(m); d++ )
		{
			pSector = g_World.GetSector(m, d);
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
	m_ObjStatusUpdates.RemoveAll();
	m_Parties.DeleteAll();
	m_GMPages.DeleteAll();

	if ( m_Sectors != NULL )
	{
		//	free memory allocated by sectors
		for ( unsigned int s = 0; s < m_SectorsQty; s++ )
		{
			// delete everything in sector
			m_Sectors[s]->Close();
		}
		// do this in two loops because destructors of items 
		// may access server sectors
		for ( unsigned int s = 0; s < m_SectorsQty; s++ )
		{
			// delete the sectors
			delete m_Sectors[s];
			m_Sectors[s] = NULL;
		}

		delete[] m_Sectors;
		m_Sectors = NULL;
		m_SectorsQty = 0;
	}

	memset(g_MapList.m_maps, 0, sizeof(g_MapList.m_maps));

	if ( g_MapList.m_pMapDiffCollection != NULL )
	{
		delete g_MapList.m_pMapDiffCollection;
		g_MapList.m_pMapDiffCollection = NULL;
	}

	CloseAllUIDs();

	m_Clock.Init();	// no more sense of time.
}

void CWorld::GarbageCollection()
{
	ADDTOCALLSTACK("CWorld::GarbageCollection");
	g_Log.Flush();
	g_Serv.SetServerMode(SERVMODE_Loading);
	GarbageCollection_UIDs();
	g_Serv.SetServerMode(SERVMODE_Run);
	g_Log.Flush();
}

void CWorld::Speak( const CObjBaseTemplate * pSrc, LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK("CWorld::Speak");
	if ( !pszText || !pszText[0] )
		return;

	bool fSpeakAsGhost = false;	// I am a ghost ?
	if ( pSrc )
	{
		if ( pSrc->IsChar() )
		{
			const CChar *pCharSrc = static_cast<const CChar *>(pSrc);
			ASSERT(pCharSrc);
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();

			if ( wHue == HUE_TEXT_DEF && pCharSrc->m_pNPC )
				wHue = pCharSrc->m_SpeechHue;
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	//CGString sTextUID;
	//CGString sTextName;	// name labelled text.
	CGString sTextGhost; // ghost speak.

	// For things
	bool fCanSee = false;
	CChar * pChar = NULL;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next(), fCanSee = false, pChar = NULL)
	{
		if ( ! pClient->CanHear( pSrc, mode ))
			continue;

		TCHAR * myName = Str_GetTemp();

		LPCTSTR pszSpeak = pszText;
		pChar = pClient->GetChar();

		if ( pChar != NULL )
		{
			fCanSee = pChar->CanSee(pSrc);

			if ( fSpeakAsGhost && !pChar->CanUnderstandGhost() )
			{
				if ( sTextGhost.IsEmpty() )
				{
					sTextGhost = pszText;
					for ( int i = 0; i < sTextGhost.GetLength(); i++ )
					{
						if ( sTextGhost[i] != ' ' &&  sTextGhost[i] != '\t' )
							sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
					}
				}
				pszSpeak = sTextGhost;

				static const SOUND_TYPE sm_GhostSounds[] = { 0x17E, 0x17F, 0x180, 0x181, 0x182 };
				pClient->addSound(sm_GhostSounds[Calc_GetRandVal(COUNTOF(sm_GhostSounds))], pSrc);
			}
			
			if ( !fCanSee && pSrc )
			{
				//if ( sTextName.IsEmpty() )
				//{
				//	sTextName.Format("<%s>", pSrc->GetName());
				//}
				//myName = sTextName;
				if ( !*myName )
					sprintf(myName, "<%s>", pSrc->GetName());
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			//if ( sTextUID.IsEmpty() )
			//{
			//	sTextUID.Format("<%s [%lx]>", pSrc->GetName(), static_cast<DWORD>(pSrc->GetUID()));
			//}
			//myName = sTextUID;
			if ( !*myName )
				sprintf(myName, "<%s [%lx]>", pSrc->GetName(), static_cast<DWORD>(pSrc->GetUID()));
		}
		if (*myName)
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font, false, myName );
		else
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font );
	}
}

void CWorld::SpeakUNICODE( const CObjBaseTemplate * pSrc, const NCHAR * pwText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CWorld::SpeakUNICODE");
	bool fSpeakAsGhost = false;
	if ( pSrc && pSrc->IsChar() )
	{
		// Are they dead ? Garble the text. unless we have SpiritSpeak
		const CChar * pCharSrc = dynamic_cast <const CChar*> (pSrc);
		if ( pCharSrc )
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
		else
			pSrc = NULL;
	}

	if ( !pSrc )
		mode = TALKMODE_BROADCAST;

	NCHAR wTextUID[MAX_TALK_BUFFER];	// uid labelled text.
	wTextUID[0] = '\0';
	NCHAR wTextName[MAX_TALK_BUFFER];	// name labelled text.
	wTextName[0] = '\0';
	NCHAR wTextGhost[MAX_TALK_BUFFER]; // ghost speak.
	wTextGhost[0] = '\0';

	// For things
	bool fCanSee = false;
	CChar * pChar = NULL;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next(), fCanSee = false, pChar = NULL)
	{
		if ( ! pClient->CanHear( pSrc, mode ))
			continue;

		const NCHAR * pwSpeak = pwText;
		pChar = pClient->GetChar();
		
		if ( pChar != NULL )
		{
			// Cansee?
			fCanSee = pChar->CanSee( pSrc );

			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost())
			{
				if ( wTextGhost[0] == '\0' )	// Garble ghost.
				{
					size_t i;
					for ( i = 0; i < MAX_TALK_BUFFER - 1 && pwText[i]; ++i )
					{
						if ( pwText[i] != ' ' && pwText[i] != '\t' )
							wTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						else
							wTextGhost[i] = pwText[i];
					}
					wTextGhost[i] = '\0';
				}
				pwSpeak = wTextGhost;

				static const SOUND_TYPE sm_GhostSounds[] = { 0x17E, 0x17F, 0x180, 0x181, 0x182 };
				pClient->addSound(sm_GhostSounds[Calc_GetRandVal(COUNTOF(sm_GhostSounds))], pSrc);
			}
			
			// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( wTextName[0] == '\0' )
				{
					CGString sTextName;
					sTextName.Format("<%s>", pSrc->GetName());
					int iLen = CvtSystemToNUNICODE( wTextName, COUNTOF(wTextName), sTextName, -1 );
					if ( wTextGhost[0] != '\0' )
					{
						for ( size_t i = 0; wTextGhost[i] != '\0' && iLen < MAX_TALK_BUFFER; i++, iLen++ )
						{
							wTextName[iLen] = wTextGhost[i];
						}
					}
					else
					{
						for ( size_t i = 0; pwText[i] != 0 && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
						{
							wTextName[iLen] = pwText[i];
						}
					}
					wTextName[iLen] = '\0';
				}
				pwSpeak = wTextName;
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( wTextUID[0] == '\0' )
			{
				TCHAR * pszMsg = Str_GetTemp();
				sprintf(pszMsg, "<%s [%lx]>", pSrc->GetName(), static_cast<DWORD>(pSrc->GetUID()));
				int iLen = CvtSystemToNUNICODE( wTextUID, COUNTOF(wTextUID), pszMsg, -1 );
				for ( size_t i = 0; pwText[i] && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
				{
					wTextUID[iLen] = pwText[i];
				}
				wTextUID[iLen] = '\0';
			}
			pwSpeak = wTextUID;
		}

		pClient->addBarkUNICODE( pwSpeak, pSrc, wHue, mode, font, lang );
	}
}

void CWorld::Broadcast(LPCTSTR pMsg) // System broadcast in bold text
{
	ADDTOCALLSTACK("CWorld::Broadcast");
	Speak( NULL, pMsg, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD );
}

void __cdecl CWorld::Broadcastf(LPCTSTR pMsg, ...) // System broadcast in bold text
{
	ADDTOCALLSTACK("CWorld::Broadcastf");
	TemporaryString sTemp;
	va_list vargs;
	va_start(vargs, pMsg);
	_vsnprintf(sTemp, sTemp.realLength(), pMsg, vargs);
	va_end(vargs);
	Broadcast(sTemp);
}

//////////////////////////////////////////////////////////////////
// Game time.

DWORD CWorld::GetGameWorldTime( CServTime basetime ) const
{
	ADDTOCALLSTACK("CWorld::GetGameWorldTime");
	// basetime = TICK_PER_SEC time.
	// Get the time of the day in GameWorld minutes
	// 8 real world seconds = 1 game minute.
	// 1 real minute = 7.5 game minutes
	// 3.2 hours = 1 game day.
	return( static_cast<unsigned long>(basetime.GetTimeRaw() / g_Cfg.m_iGameMinuteLength) );
}

CServTime CWorld::GetNextNewMoon( bool bMoonIndex ) const
{
	ADDTOCALLSTACK("CWorld::GetNextNewMoon");
	// "Predict" the next new moon for this moon
	// Get the period
	DWORD iSynodic = bMoonIndex ? FELUCCA_SYNODIC_PERIOD : TRAMMEL_SYNODIC_PERIOD;

	// Add a "month" to the current game time
	DWORD iNextMonth = GetGameWorldTime() + iSynodic;

	// Get the game time when this cycle will start
	DWORD iNewStart = static_cast<DWORD>(iNextMonth - static_cast<double>(iNextMonth % iSynodic));

	// Convert to TICK_PER_SEC ticks
	CServTime time;
	time.InitTime( iNewStart * g_Cfg.m_iGameMinuteLength );
	return(time);
}

unsigned int CWorld::GetMoonPhase (bool bMoonIndex) const
{
	ADDTOCALLSTACK("CWorld::GetMoonPhase ");
	// bMoonIndex is FALSE if we are looking for the phase of Trammel,
	// TRUE if we are looking for the phase of Felucca.

	// There are 8 distinct moon phases:  New, Waxing Crescent, First Quarter, Waxing Gibbous,
	// Full, Waning Gibbous, Third Quarter, and Waning Crescent

	// To calculate the phase, we use the following formula:
	//				CurrentTime % SynodicPeriod
	//	Phase = 	-----------------------------------------     * 8
	//			              SynodicPeriod
	//

	DWORD dwCurrentTime = GetGameWorldTime();	// game world time in minutes

	if (!bMoonIndex)
	{
		// Trammel
		return( IMULDIV( dwCurrentTime % TRAMMEL_SYNODIC_PERIOD, 8, TRAMMEL_SYNODIC_PERIOD ));
	}
	else
	{
		// Luna2
		return( IMULDIV( dwCurrentTime % FELUCCA_SYNODIC_PERIOD, 8, FELUCCA_SYNODIC_PERIOD ));
	}
}

void CWorld::OnTick()
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::OnTick");
	// Do this once per tick.
	// 256 real secs = 1 GRAYhour. 19 light levels. check every 10 minutes or so.

	if ( g_Serv.IsLoading() || !m_Clock.Advance() )
		return;

	if ( m_timeSector <= GetCurrentTime())
	{
		// Only need a SECTOR_TICK_PERIOD tick to do world stuff.
		m_timeSector = GetCurrentTime() + SECTOR_TICK_PERIOD;	// Next hit time.
		m_Sector_Pulse ++;
		int	m, s;

		for ( m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] ) continue;

			for ( s = 0; s < g_MapList.GetSectorQty(m); s++ )
			{
				EXC_TRYSUB("Tick");

				CSector	*pSector = GetSector(m, s);
				if ( !pSector )
					g_Log.EventError("Ticking NULL sector %d on map %d\n", s, m);
				else
					pSector->OnTick( m_Sector_Pulse );

				EXC_CATCHSUB("Sector");
			}
		}

		// process objects that need status updates
		// these objects will normally be in containers which don't have any period OnTick method
		// called (whereas other items can receive the OnTickStatusUpdate() call via their normal
		// tick method).
		// note: ideally, a better solution to accomplish this should be found if possible
		if (m_ObjStatusUpdates.GetCount() > 0)
		{
			EXC_TRYSUB("Tick");

			// loop backwards to avoid possible infinite loop if a status update is triggered
			// as part of the status update (e.g. property changed under tooltip trigger)
			size_t i = m_ObjStatusUpdates.GetCount();
			while ( i > 0 )
			{
				CObjBase * pObj = m_ObjStatusUpdates.GetAt(--i);
				if (pObj != NULL)
					pObj->OnTickStatusUpdate();
			}

			m_ObjStatusUpdates.RemoveAll();

			EXC_CATCHSUB("StatusUpdates");
		}

		m_ObjDelete.DeleteAll();	// clean up our delete list.
	}

	EXC_TRYSUB("Tick");
	m_TimedFunctions.OnTick();
	EXC_CATCHSUB("TimerFunction");

	if ( !m_bSaveNotificationSent && (m_timeSave - (10 * TICK_PER_SEC) <= GetCurrentTime()) )
	{
		Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE_NOTIFY ) );
		m_bSaveNotificationSent = true;
	}
	if ( m_timeSave <= GetCurrentTime())
	{
		// Auto save world
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;
		g_Log.Flush();
		Save( false );
	}
	if ( m_timeRespawn <= GetCurrentTime())
	{
		// Time to regen all the dead NPC's in the world.
		m_timeRespawn = GetCurrentTime() + (20*60*TICK_PER_SEC);
		RespawnDeadNPCs();
	}
	if ( g_Cfg.m_iTimerCall && (m_timeCallUserFunc < GetCurrentTime()) )
	{
		m_timeCallUserFunc = GetCurrentTime() + g_Cfg.m_iTimerCall;
		CScriptTriggerArgs Args(g_Cfg.m_iTimerCall / (60 * TICK_PER_SEC));
		g_Serv.r_Call("f_onserver_timer", &g_Serv, &Args);
	}
}

CSector *CWorld::GetSector(int map, int i)	// gets sector # from one map
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetSector");

	// if the map is not supported, return empty sector
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] )
		return NULL;

	if ( i >= g_MapList.GetSectorQty(map) )
	{
		g_Log.EventError("Unsupported sector #%d for map #%d specified\n", i, map);
		return NULL;
	}

	int base = 0;
	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		if ( m == map )
		{
			if ( g_MapList.GetSectorQty(map) < i )
				return NULL;

			return m_Sectors[base + i];
		}

		base += g_MapList.GetSectorQty(m);
	}
	return NULL;
}
