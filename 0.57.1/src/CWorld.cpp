#include "graysvr.h"
#include "common/version.h"
#include "network/network.h"

#if !defined( _WIN32 )
#include <sys/time.h>
#endif

LPCTSTR GetReasonForGarbageCode(int iCode)
{
	LPCTSTR pStr;
	switch ( iCode )
	{
		default:
			pStr = "Unknown";
			break;

		case 0x1102:
			pStr = "Disconnected char not acting as such";
			break;

		case 0x1103:
		case 0x1203:
			pStr = "Ridden npc not acting as such";
			break;
			
		case 0x1104:
		case 0x1204:
			pStr = "Ridden npc without mount item";
			break;
			
		case 0x1105:
			pStr = "Ridden npc with mislinked mount item";
			break;
			
		case 0x1106:
			pStr = "Disconnected npc neither dead nor ridden";
			break;

		case 0x1108:
			pStr = "Char not on a valid P";
			break;

		case 0x2104:
			pStr = "Disconnected item";
			break;
			
		case 0x2105:
			pStr = "Item not on a valid P";
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
			pStr = "GM Robe not in place / Deathshroud not on dead char";
			break;

		case 0x2220:
			pStr = "Trade window memory not equipped in the correct layer";
			break;

		case 0x2221:
			pStr = "Client linger memory not equipped in the correct layer";
			break;

		case 0x2222:	
			pStr = "Memory not equipped / not on the memory layer / without color";
			break;

		case 0x2226:
			pStr = "Mount memory not equipped in the correct layer";
			break;
			
		case 0x2227:
			pStr = "Hair/Beard item not equipped / not in a corpse / not in a vendor box";
			break;
			
		case 0x2229:
			pStr = "Game piece not in a game board";
			break;
			
		case 0x2230:
			pStr = "Item equipped in trade window layer but it isn't a trade window";
			break;
			
		case 0x2231:
			pStr = "Item equipped in memory layer but it isn't a memory";
			break;
			
		case 0x2233:
			pStr = "Item equipped in mount memory layer but it isn't a mount memory";
			break;
			
		case 0x2234:
			pStr = "Item equipped in client linger layer but it isn't a client linger memory";
			break;
			
		case 0x2235:
			pStr = "Item equipped in murder memory layer but it isn't a murder memory";
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

//////////////////////////////////////////////////////////////////
// -CTimedFunctionHandler

void CTimedFunctionHandler::OnTick()
{
	m_isBeingProcessed = true;

	EXC_TRY("Tick");
	m_curTick++;

	if ( m_curTick >= TICK_PER_SEC )
	{
		m_curTick = 0;
	}

	int tick = m_curTick;
	std::vector<TimedFunction *>::iterator it;

	if ( m_timedFunctions[tick].size() > 0 )
	{
		for( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
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
				it++;
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
	EXC_CATCH;
}

void CTimedFunctionHandler::Erase( UID uid )
{
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); ) 
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
				it++;
			}
		}
	}
}

void CTimedFunctionHandler::Add( UID uid, int numSeconds, LPCTSTR funcname )
{
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
	static char tempBuffer[1024];
	static TimedFunction *tf = NULL;

	if ( !pszName )
	{
		return -1;
	}

	if ( strcmpi( pszName, "CurTick" ) == 0 )
	{
		if ( isdigit(pszVal[0] ) )
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
		int iArgs = Str_ParseCmds( tempBuffer, ppVal, COUNTOF( ppVal ), " ,\t" );
		if ( iArgs == 3 )
		{
			if ( isdigit( ppVal[0][0] ) && isdigit( ppVal[1][0] ) && isdigit( ppVal[2][0] ) )
			{
				int tick = ATOI(ppVal[0]);
				int uid = ATOI(ppVal[1]);
				int elapsed = ATOI(ppVal[2]);
				int isNew = 0;
				if ( tf == NULL )
				{
					tf=new TimedFunction;
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
					g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE );
				}
			}
			else
			{
				g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s\n", SPHERE_FILE, pszName, pszVal );
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
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE );
		}
	}

	return( 0 );
}

void CTimedFunctionHandler::r_Write( CScript & s )
{
	s.WriteKeyFormat( "CurTick", "%i", m_curTick );
	for ( int tick = 0; tick < TICK_PER_SEC; tick++ )
	{
		std::vector<TimedFunction *>::iterator it;
		for( it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); it++ )
		{
			TimedFunction* tf = *it;
			if ( tf->uid.IsValidUID() )
			{
				s.WriteKeyFormat( "TimerFCall", "%s", tf->funcname );
				s.WriteKeyFormat( "TimerFNumbers", "%i,%i,%i", tick, tf->uid.GetObjUID(), tf->elapsed );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldSearch

CWorldSearch::CWorldSearch(const CPointMap &pt, int iDist, const CObjBase *skipObject) :
	m_pt(pt), m_iDist(iDist), m_skipObject(skipObject)
{
	// define a search of the world.
	m_fAllShow = false;
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
	// Move search into nearby CSector(s) if necessary

	if ( ! m_iDist )
		return false;

	while (true)
	{
		m_pSector = m_rectSector.GetSector(m_iSectorCur++);
		if ( m_pSector == NULL )
			return false;	// done searching.
		if ( m_pSectorBase == m_pSector )
			continue;	// same as base.
		m_pObj = NULL;	// start at head of next Sector.
		return true;
	}

	return false;	// done searching.
}

CItem * CWorldSearch::GetItem()
{
	while (true)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = static_cast <CObjBase*> ( m_pSector->m_Items_Inert.GetHead());
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
				m_pObj = static_cast <CObjBase*> ( m_pSector->m_Items_Timer.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_pObj == m_skipObject ) continue;
		if ( m_fAllShow )
		{
			if ( m_pt.GetDistBase( m_pObj->GetTopPoint()) <= m_iDist )
				return( static_cast <CItem *> ( m_pObj ));
		}
		else
		{
			if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
				return( static_cast <CItem *> ( m_pObj ));
		}
	}
}

CChar * CWorldSearch::GetChar()
{
	while (true)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = static_cast <CObjBase*> ( m_pSector->m_Chars_Active.GetHead());
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
				m_pObj = static_cast <CObjBase*> ( m_pSector->m_Chars_Disconnect.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return NULL;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_pObj == m_skipObject ) continue;
		if ( m_fAllShow )
		{
			if ( m_pt.GetDistBase( m_pObj->GetTopPoint()) <= m_iDist )
				return( static_cast <CChar *> ( m_pObj ));
		}
		else
		{
			if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
				return( static_cast <CChar *> ( m_pObj ));
		}
	}
}

DWORD CWorld::GetUIDCount() const
{
	return m_UIDs.GetCount();
}

CObjBase *CWorld::FindUID(DWORD dwIndex) const
{
	return (( dwIndex > 0 ) && ( dwIndex < GetUIDCount() )) ? m_UIDs[dwIndex] : NULL;
}

void CWorld::FreeUID(DWORD dwIndex)
{
	// Can't free up the UID till after the save !
	m_UIDs[dwIndex] = NULL;
}

DWORD CWorld::AllocUID(DWORD dwIndex, CObjBase * pObj)
{
	DWORD dwCountTotal = m_UIDs.GetCount();

	if (( dwIndex >= dwCountTotal ) || !dwCountTotal )		//	resize the UID array to fit the needed size
	{
		m_UIDs.SetCount((dwIndex  + 0x1000) &~ 0xfff);
		dwIndex = max(dwIndex, 1);
#ifdef _DEBUG
		g_Log.Debug("Resizing UID array from %d elements to %d.\n", dwCountTotal, m_UIDs.GetCount());
#endif
	}
	if ( !dwIndex )											//	search the empty UID to use
	{
		if (( m_FreeOffset < 0 ) || ( m_FreeOffset >= FREE_UIDS_SIZE ))
			return AllocUID(dwCountTotal, pObj);

		//	search the array holding the free UIDs in order to determine first free element
		for ( ; ( m_FreeOffset < FREE_UIDS_SIZE ) && ( m_FreeUIDs[m_FreeOffset] ); m_FreeOffset++ )
		{
			DWORD checkUid = m_FreeUIDs[m_FreeOffset];
			if ( !m_UIDs[checkUid] )
			{
				dwIndex = checkUid;
				m_FreeOffset++;
				break;
			}
		}

		if (( m_FreeOffset >= FREE_UIDS_SIZE ) || !m_FreeUIDs[m_FreeOffset] )
		{
			//	we did not find an empty slot - use silly allocation method for a while
			m_FreeOffset = -1;
#ifdef _DEBUG
			g_Log.Debug("No free UID slots are left, using dummy allocation technique till next garbage collection.\n");
#endif
		}
	}

	CObjBase *pObjPrv = m_UIDs[dwIndex];
	if ( pObjPrv )
	{
		try
		{
			g_Log.Error("UID conflict delete 0%d, '%s'\n", dwIndex, pObjPrv->GetName());
			delete pObjPrv;
		}
		catch (...) {}
	}
	m_UIDs[dwIndex] = pObj;
	return dwIndex;
}

int CWorld::FixObjTry( CObjBase * pObj, int iUID )
{
	// RETURN: 0 = success.
	if ( !pObj )
		return 0x7102;

	if ( iUID )
	{
		if (( pObj->GetUID() & UID_O_INDEX_MASK ) != iUID )
		{
			// Miss linked in the UID table !!! BAD
			// Hopefully it was just not linked at all. else How the hell should i clean this up ???
			g_Log.Error("UID 0%x, '%s', Mislinked\n", iUID, pObj->GetName());
			return 0x7101;
		}
	}
	return pObj->FixWeirdness();
}

int CWorld::FixObj( CObjBase * pObj, int iUID )
{
	// Attempt to fix problems with this item, ignore any children it may have for now.
	// RETURN: 0 = success.

	int iResultCode = 0;

	try
	{
		if ( !( iResultCode = FixObjTry(pObj,iUID) ) )
			return 0;
	}
	catch ( CError &e )
	{
		g_Log.Catch( &e, "FixObj" );
	}
	catch (...)
	{
		g_Log.Catch(NULL, "FixObj");
	}

	if ( !iResultCode )
		iResultCode = 0xffff;

	try
	{
		iUID = pObj->GetUID();

		// is it a real error ?
		if ( pObj->IsItem())
		{
			CItem * pItem = dynamic_cast <CItem*>(pObj);
			if ( pItem && pItem->IsType(IT_EQ_MEMORY_OBJ) )
			{
				pObj->Delete();
				return iResultCode;
			}
		}

		g_Log.Error("UID=0%x, id=0%x '%s', Invalid code=%0x (%s)\n",
			iUID, pObj->GetBaseID(), pObj->GetName(), iResultCode, GetReasonForGarbageCode(iResultCode));
		if ( iResultCode == 0x1203 || iResultCode == 0x1103 )
		{
			CChar * pChar = dynamic_cast <CChar*>(pObj);
			if (pChar)
			{
				pChar->Skill_Start(NPCACT_RIDDEN);
			}
		}
		else
		{
			pObj->Delete();
		}
	}
	catch ( CError &e )	// catch all
	{
		g_Log.Catch(&e, "UID=0%x, Asserted cleanup", iUID);
	}
	return iResultCode;
}

//////////////////////////////////////////////////////////////////
// -CWorld

CWorld::CWorld()
{
	m_fSaveParity = false;		// has the sector been saved relative to the char entering it ?

	m_FreeUIDs = (DWORD*)calloc(FREE_UIDS_SIZE, sizeof(DWORD));
	m_FreeOffset = -1;

	m_iSaveCountID = 0;
	m_iSaveStage = 0;
	m_timeSector.Init();
	m_timeRespawn.Init();
	m_timeStartup.Init();
	m_timeCallUserFunc.Init();

	m_Sectors = NULL;
	m_SectorsQty = 0;

	InitTime(0);
}

void CWorld::Init()
{
	EXC_TRY("Init");

	if ( m_Sectors )	//	disable changes on-a-fly
		return;

	g_MapList.Init();
	//	initialize all sectors
	int	sectors = 0;
	int m = 0;
	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;
		sectors += g_MapList.GetSectorQty(m);
	}

	m_Sectors = (CSector**)malloc(sectors * sizeof(CSector*));
	TEMPSTRING(z);
	TEMPSTRING(z1);
	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		sprintf(z1, " %d=%d", m, g_MapList.GetSectorQty(m));
		strcat(z, z1);
		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = new CSector;
			pSector->Init(s, m);
			m_Sectors[m_SectorsQty++] = pSector;
		}
	}

	g_Log.Init("Allocating map sectors:%s\n", z);
	EXC_CATCH;
}

CWorld::~CWorld()
{
}

///////////////////////////////////////////////
// Loading and Saving.

bool CWorld::SaveStage()
{
	// RETURN: true = continue; false = done.

	EXC_TRY("SaveStage");
	bool	bRc = true;
	int		i;

	if ( m_iSaveStage == -1 )						// Step -1: open files for save
	{
		EXC_SET("init");
		g_Log.Event(LOGM_SAVE, "World save started.\n");
		GarbageCollection();

		TEMPSTRING(z);
		for ( i = 0; i < 5; i++ )
		{
			const char *ftype;
			CScript *script;
			switch ( i )
			{
			case 0: ftype = "data"; script = &m_FileData; break;
			case 1: ftype = "world"; script = &m_FileWorld; break;
			case 2: ftype = "chars"; script = &m_FilePlayers; break;
			case 3: ftype = "accu"; script = &m_FileAccounts; break;
			case 4: ftype = "statics"; script = &m_FileStatics; break;
			}

			sprintf(z, "%s" SPHERE_FILE "%s.tmp", (LPCTSTR)g_Cfg.m_sWorldBaseDir, ftype);
			if ( !script->Open(z, OF_WRITE) )
			{
				m_FileData.Close();
				m_FileWorld.Close();
				m_FilePlayers.Close();
				m_FileAccounts.Close();
				m_FileStatics.Close();
				return false;
			}

			r_Write(*script);
		}
		m_fSaveParity = !m_fSaveParity;
	}
	else if ( m_iSaveStage < m_SectorsQty )			// Step 0..N: save sector-by-sector
	{
		EXC_SET("save sector");

		if ( m_Sectors[m_iSaveStage] )
			m_Sectors[m_iSaveStage]->r_Write();
	}
	else if ( m_iSaveStage == m_SectorsQty )		// Step N: write VARs, REGIONs, GMPAGEs
	{
		EXC_SET("variables");

		m_FileData.WriteSection( "TIMERF" );
		g_World.m_TimedFunctions.r_Write(m_FileData);
	
		m_FileData.WriteSection("GLOBALS");
		int	i;

		for ( i = 0; i < g_Exp.m_VarGlobals.GetCount(); i++ )
		{
			VariableList::Variable *pVar = g_Exp.m_VarGlobals.GetAt(i);
			m_FileData.WriteKey(pVar->GetKey(), pVar->GetValStr());
		}

		EXC_SET("regions");
		for ( i = 0; i < g_Cfg.m_RegionDefs.GetCount(); i++ )
		{
			CRegionBase *pRegion = dynamic_cast <CRegionBase*> (g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			CRegionWorld *pRegionWorld = dynamic_cast <CRegionWorld*>(pRegion);

			m_FileData.WriteSection("WS %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		EXC_SET("gm pages");
		CGMPage *pPage = static_cast <CGMPage*>(m_GMPages.GetHead());
		for ( ; pPage != NULL; pPage = pPage->GetNext())
		{
			pPage->r_Write(m_FileData);
		}
	}
	else if ( m_iSaveStage == m_SectorsQty+1 )		// Step N+1: save accounts
	{
		EXC_SET("accounts");

		for ( i = 0; i < g_Accounts.Account_GetCount(); i++ )
		{
			CAccountRef pAccount = g_Accounts.Account_Get(i);
			if ( pAccount )
				pAccount->r_Write(m_FileAccounts);
		}

		EXC_SET("moved chars");
		vector<long>::iterator it;
		UID	uid;
		for ( it = m_LostPlayers.begin(); it != m_LostPlayers.end(); it++ )
		{
			uid = *it;
			CChar	*pChar = uid.CharFind();
			if ( pChar )
			{
				if ( pChar->m_pPlayer )
					pChar->r_WriteParity(m_FilePlayers);
				else
					pChar->r_WriteParity(m_FileWorld);
			}
		}
		m_LostPlayers.clear();
	}
	else if ( m_iSaveStage == m_SectorsQty+2 )		// Step N+2: finilize save
	{
		EXC_SET("finilizing");

		//	detect backup name
		int iCount = m_iSaveCountID;
		int iGroup = 0;
		while ( iGroup++ < config.get("core.backup.level") )
		{
			if ( iCount & 7 )
				break;
			iCount >>= 3;
		}

		//	move the saved scripts to its places
		TEMPSTRING(z);
		TEMPSTRING(z1);
		TEMPSTRING(z2);
		for ( i = 0; i < 5; i++ )
		{
			const char *ftype;
			CScript *script;
			switch ( i )
			{
			case 0: ftype = "data"; script = &m_FileData; break;
			case 1: ftype = "world"; script = &m_FileWorld; break;
			case 2: ftype = "chars"; script = &m_FilePlayers; break;
			case 3: ftype = "accu"; script = &m_FileAccounts; break;
			case 4: ftype = "statics"; script = &m_FileStatics; break;
			}

			sprintf(z, "%s" SPHERE_FILE "b%d%d%c%s", (LPCTSTR)g_Cfg.m_sWorldBaseDir, iGroup, iCount&0x07, ftype[0], SPHERE_SCRIPT);
			sprintf(z1, "%s" SPHERE_FILE "%s%s", (LPCTSTR)g_Cfg.m_sWorldBaseDir, ftype, SPHERE_SCRIPT);
			sprintf(z2, "%s" SPHERE_FILE "%s.tmp", (LPCTSTR)g_Cfg.m_sWorldBaseDir, ftype);

			if ( script != &m_FileAccounts )
				script->WriteSection("EOF");
			script->Close();

			//	move current save to the backup, temporary data to current save
			remove(z);
			if ( rename(z1, z) )
				g_Log.Error("Save unable to create a '%s' backup file '%s' for save.\n", ftype, z);
			if ( rename(z2, z1) )
				g_Log.Error("FATAL: Save unable to create a '%s' save file '%s'. The save IS NOT VALID.\n", ftype, z1);
		}

		// job's done
		g_Log.Event(LOGM_SAVE, "World save completed. Old save renamed to '%d%d'.\n", iGroup, iCount&0x07);
		// try to maintain exactly-same-period of saves if not too fast occuring
		if ( m_timeSave <= (GetCurrentTime() + g_Cfg.m_iSavePeriod / 10) )
			m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;
		m_iSaveCountID++;
		m_iSaveStage = INT_MAX;
		return false;
	}

	m_iSaveStage++;
	return bRc;

	EXC_CATCH;

	m_iSaveStage++;	// to avoid loops, we need to skip the current operation in world save
	return false;
}

/////////////////////////////////////////////////////////////////////

bool CWorld::LoadFile(LPCTSTR pszLoadName, bool fError) // Load world from script
{
	CScript s;
	if ( !s.Open(pszLoadName) )
	{
		if ( fError )
			g_Log.Event(LOGM_INIT|LOGL_ERROR, "Can't Load %s\n", pszLoadName);
		return false;
	}
	g_Log.Init("Loading %s...\n", pszLoadName);

	// Find the size of the file.
	DWORD lLoadSize = s.GetLength();
	int iLoadStage = 0;

	CScriptFileContext ScriptContext(&s);

	// Read the header stuff first.
	CScriptObj::r_Load(s);

	while ( s.FindNextSection() )
	{
		if ( !( ++iLoadStage & 0x1FF) )	// don't update too often
		{
			g_Serv.PrintPercent(s.GetPosition(), lLoadSize);
		}

		try
		{
			g_Cfg.LoadResourceSection(&s);
		}
		catch ( CError &e )
		{
			g_Log.Catch(&e, "Load Exception line %d.\n", s.GetContext().m_iLineNum);
		}
	}

	if ( s.IsSectionType("EOF") )
	{
		// The only valid way to end.
		s.Close();
		return true;
	}

	g_Log.Event( LOGM_INIT|LOGL_CRIT, "No [EOF] marker. '%s' is corrupt!\n", s.GetFilePath());
	return false;
}


bool CWorld::LoadAll() // Load world from script
{
	m_UIDs.SetCount(8 * 1024);
	m_Clock_PrevSys = GetSystemClock();
	InitTime(0);

	EXC_TRY("LoadAll");
	EXC_SET("accounts");

	// Load all the accounts.
	CScript s;
	TEMPSTRING(z);
	sprintf(z, "%s" SPHERE_FILE "accu", g_Cfg.m_sWorldBaseDir);
	if ( !s.Open(z, OF_READ|OF_TEXT) )
	{
		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Can't open account file '%s'\n", s.GetFilePath());
		return false;
	}

	CScriptFileContext	ScriptContext(&s);
	while ( s.FindNextSection() )
	{
		g_Accounts.Account_Load(s.GetKey(), s);
	}

	// Try to load the world and chars files .
	EXC_SET("world");
	CGString fdata, fstatic, fworld, fchars;
	fdata = fstatic = fworld = fchars = g_Cfg.m_sWorldBaseDir;
	fdata += SPHERE_FILE "data";
	fstatic += SPHERE_FILE "statics";
	fworld += SPHERE_FILE "world";
	fchars += SPHERE_FILE "chars";

	LoadFile(fdata, false);
	LoadFile(fstatic, false);
	if ( !LoadFile(fworld) || !LoadFile(fchars) )
	{

		g_Log.Event(LOGL_FATAL, "Cannot load world and chars save files!\n");
		return false;
	}

	m_timeStartup = GetCurrentTime();
	m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time.

	// Set all the sector light levels now that we know the time.
	// This should not look like part of the load. (CTRIG_EnvironChange triggers should run)
	EXC_SET("sectors");
	for ( int i = 0; i < m_SectorsQty; i++ )
	{
		CSector *pSector = m_Sectors[i];
		int cnt;
		if ( pSector )
		{
			if ( !pSector->IsLightOverriden() )
				pSector->SetLight(-1);

			if ( (cnt = pSector->GetItemComplexity()) > g_Cfg.m_iMaxSectorComplexity )
				g_Log.Event(LOGL_WARN, "%d items at %s. Sector #%d too complex!\n", cnt, pSector->GetBasePoint().WriteUsed(), i);

			if ( (cnt = pSector->GetCharComplexity()) > g_Cfg.m_iMaxCharComplexity )
				g_Log.Event(LOGL_WARN, "%d chars at %s. Sector #%d too complex!\n", cnt, pSector->GetBasePoint().WriteUsed(), i);
		}
	}

	EXC_SET("garbage");
	GarbageCollection();

	// Set the current version now.
	r_SetVal("VERSION", SPHERE_VERSION);
	return true;

	EXC_CATCH;
	return false;
}

/////////////////////////////////////////////////////////////////

void CWorld::r_Write( CScript & s )
{
	if ( &s == &m_FileAccounts )
		m_FileAccounts.Printf("// " SPHERE_TITLE " %s accounts file\n// NOTE: This file cannot be edited while the server is running.\n", g_Serv.GetName());
	else
	{
		s.WriteKey("TITLE", SPHERE_TITLE " World Script");
		s.WriteKey("VERSION", SPHERE_VERSION);
		s.WriteKeyVal("TIME", GetCurrentTime().GetTimeRaw());
		s.WriteKeyVal("SAVECOUNT", m_iSaveCountID);
	}
	s.Flush();
}

bool CWorld::r_GetRef(LPCTSTR &pszKey, CScriptObj * &pRef)
{
	if ( !strnicmp(pszKey, "LASTNEW", 7) )
	{
		if ( !strnicmp(pszKey + 7, "ITEM", 4) )
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewItem.ItemFind();
			return true;
		}
		else if ( !strnicmp(pszKey + 7, "CHAR", 4) )
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewChar.CharFind();
			return true;
		}
	}
	return false;
}

enum WC_TYPE
{
	WC_SAVECOUNT,
	WC_TIME,
	WC_TITLE,
	WC_VERSION,
	WC_QTY,
};

LPCTSTR const CWorld::sm_szLoadKeys[WC_QTY+1] =	// static
{
	"SAVECOUNT",
	"TIME",
	"TITLE",
	"VERSION",
	NULL,
};

bool CWorld::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY("WriteVal");

	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		CGMPage	*pPage;
		pszKey += 6;
		if (( *pszKey == 'S' ) || ( *pszKey == 's' ))	//	SERV.GMPAGES
			sVal.FormatVal(m_GMPages.GetCount());
		else if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPARATORS(pszKey);
			int iQty = Exp_GetVal(pszKey);
			if (( iQty < 0 ) || ( iQty >= m_GMPages.GetCount() )) return false;
			SKIP_SEPARATORS(pszKey);
			pPage = static_cast <CGMPage*> (m_GMPages.GetAt(iQty));
			if ( !pPage ) return false;

			if ( !strnicmp(pszKey, "HANDLED", 7) )
			{
				CClient *pClient = pPage->FindGMHandler();
				if ( pClient ) sVal.FormatVal(pClient->GetChar()->GetUID());
				else sVal.FormatVal(0);
				return true;
			}
			else return (pPage->r_WriteVal(pszKey, sVal, pSrc));
		}
		else sVal.FormatVal(0);
		return true;
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case WC_SAVECOUNT: // "SAVECOUNT"
			sVal.FormatVal( m_iSaveCountID );
			break;
		case WC_TIME:	// "TIME"
			sVal.FormatVal( GetCurrentTime().GetTimeRaw() );
			break;
		case WC_TITLE: // 	"TITLE",
			sVal = (SPHERE_TITLE " World Script");
			break;
		case WC_VERSION: // "VERSION"
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

bool CWorld::r_LoadVal( CScript &s )
{
	EXC_TRY("LoadVal");
	LPCTSTR	pszKey = s.GetKey();
	LPCTSTR pszArgs = s.GetArgStr();
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case WC_SAVECOUNT: // "SAVECOUNT"
			m_iSaveCountID = s.GetArgVal();
			break;
		case WC_TIME:	// "TIME"
			InitTime(s.GetArgVal());
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

void CWorld::GarbageCollection()
{
	// Clean up new objects that are never placed.
	if ( m_ObjNew.GetCount() )
	{
		g_Log.Event(LOGL_ERROR, "%d Lost object deleted\n", m_ObjNew.GetCount());
		m_ObjNew.DeleteAll();
	}
	// clean up our delete list.
	m_ObjDelete.DeleteAll();

	// save empty holes in an array not to allow the huge gaps
	memset(m_FreeUIDs, 0, FREE_UIDS_SIZE*sizeof(DWORD));
	m_FreeOffset = 0;
	for ( DWORD d = 1; d < GetUIDCount(), m_FreeOffset < (FREE_UIDS_SIZE - 1); d++ )
	{
		if ( !FindUID(d) )
			m_FreeUIDs[m_FreeOffset++] = d;
	}
	m_FreeOffset = 0;
}

void CWorld::Speak( const CObjBaseTemplate * pSrc, LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	if ( !pszText || !pszText[0] )
		return;

	bool fSpeakAsGhost = false;	// I am a ghost ?
	if ( pSrc )
	{
		if ( pSrc->IsChar())
		{
			// Are they dead ? Garble the text. unless we have SpiritSpeak
			const CChar * pCharSrc = dynamic_cast <const CChar*> (pSrc);
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	CGString sTextUID;
	CGString sTextName;	// name labeled text.
	CGString sTextGhost; // ghost speak.

	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( !client->CanHear(pSrc, mode) )
			continue;
		TEMPSTRING(myName);

		LPCTSTR pszSpeak = pszText;
		bool fCanSee = false;
		CChar * pChar = client->GetChar();
		if ( pChar != NULL )
		{
			fCanSee = pChar->CanSee( pSrc );	

			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost())
			{
				if ( sTextGhost.IsEmpty())	// Garble ghost.
				{
					sTextGhost = pszText;
					for ( int i=0; i<sTextGhost.GetLength(); i++ )
					{
						if ( sTextGhost[i] != ' ' &&  sTextGhost[i] != '\t' )
						{
							sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						}
					}
				}
				pszSpeak = sTextGhost;
				client->addSound(Calc_GetRandVal2(SOUND_GHOST_1, SOUND_GHOST_5), pSrc);
			}
			
			if ( ! fCanSee && pSrc )
			{
				if ( !*myName )
					sprintf(myName, "<%s>",pSrc->GetName());
			}
		}

		if ( ! fCanSee && pSrc && client->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( !*myName )
				sprintf(myName, "<%s [%lx]>",pSrc->GetName(), pSrc->uid());
		}

		if ( *myName )
			client->addBarkParse(pszSpeak, pSrc, wHue, mode, font, myName);
		else
            client->addBarkParse(pszSpeak, pSrc, wHue, mode, font);
	}
}

void CWorld::SpeakUNICODE( const CObjBaseTemplate * pSrc, const NCHAR * pwText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
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

	NCHAR wTextUID[MAX_TALK_BUFFER];	// uid labeled text.
	wTextUID[0] = '\0';
	NCHAR wTextName[MAX_TALK_BUFFER];	// name labeled text.
	wTextName[0] = '\0';
	NCHAR wTextGhost[MAX_TALK_BUFFER]; // ghost speak.
	wTextGhost[0] = '\0';

	ClientIterator it;
	CClient *client;
	while ( client = it.next() )
	{
		if ( !client->CanHear(pSrc, mode) )
			continue;

		const NCHAR * pwSpeak = pwText;
		bool fCanSee = false;
		CChar *pChar = client->GetChar();
		if ( pChar != NULL )
		{
			// Cansee?
			fCanSee = pChar->CanSee( pSrc );

			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost())
			{
				if ( wTextGhost[0] == '\0' )	// Garble ghost.
				{
					int i;
					for ( i=0; pwText[i] && i < MAX_TALK_BUFFER; i++ )
					{
						if ( pwText[i] != ' ' && pwText[i] != '\t' )
							wTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						else
							wTextGhost[i] = pwText[i];
					}
					wTextGhost[i] = '\0';
				}
				pwSpeak = wTextGhost;
				client->addSound(Calc_GetRandVal2(SOUND_GHOST_1, SOUND_GHOST_5), pSrc);
			}
			
			// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( wTextName[0] == '\0' )
				{
					CGString sTextName;
					sTextName.Format("<%s>", (LPCTSTR) pSrc->GetName());
					int iLen = CvtSystemToNUNICODE( wTextName, COUNTOF(wTextName), sTextName, -1 );
					if ( wTextGhost[0] != '\0' )
					{
						for ( int i=0; wTextGhost[i] && iLen < MAX_TALK_BUFFER; i++, iLen++ )
						{
							wTextName[iLen] = wTextGhost[i];
						}
					}
					else
					{
						for ( int i=0; pwText[i] && iLen < MAX_TALK_BUFFER; i++, iLen++ )
						{
							wTextName[iLen] = pwText[i];
						}
					}
					wTextName[iLen] = '\0';
				}
				pwSpeak = wTextName;
			}
		}

		if ( !fCanSee && pSrc && client->IsPriv(PRIV_HEARALL|PRIV_DEBUG) )
		{
			if ( wTextUID[0] == '\0' )
			{
				TEMPSTRING(pszMsg);
				sprintf(pszMsg, "<%s [%lx]>", pSrc->GetName(), pSrc->uid());
				int iLen = CvtSystemToNUNICODE( wTextUID, COUNTOF(wTextUID), pszMsg, -1 );
				for ( int i=0; pwText[i] && iLen < MAX_TALK_BUFFER; i++, iLen++ )
				{
					wTextUID[iLen] = pwText[i];
				}
				wTextUID[iLen] = '\0';
			}
			pwSpeak = wTextUID;
		}

		client->addBarkUNICODE(pwSpeak, pSrc, wHue, mode, font, lang);
	}
}

void __cdecl CWorld::Broadcast(LPCTSTR pMsg, ...) // System broadcast in bold text
{
	va_list vargs;
	TEMPSTRING(sTemp);
	va_start(vargs, pMsg);
	vsprintf(sTemp, pMsg, vargs);
	va_end(vargs);
	Speak( NULL, sTemp, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD );
}

void CWorld::Explode( CChar * pSrc, CPointMap pt, int iDist, int iDamage, WORD wFlags )
{
	// Purple potions and dragons fire.
	// degrade damage the farther away we are. ???

	CItem * pItem = CItem::CreateBase(ITEMID_FX_EXPLODE_3);
	if ( !pItem )
		return;

	pItem->SetAttr(ATTR_MOVE_NEVER|ATTR_CAN_DECAY);
	pItem->SetType(IT_EXPLOSION);
	pItem->m_uidLink = pSrc ? pSrc->uid() : UID_CLEAR;
	pItem->m_itExplode.m_iDamage = iDamage;
	pItem->m_itExplode.m_wFlags = wFlags|DAMAGE_GENERAL|DAMAGE_HIT_BLUNT;
	pItem->m_itExplode.m_iDist = iDist;
	pItem->MoveToDecay(pt, 1);	// almost Immediate Decay

	pItem->Sound(0x207);	// sound is attached to the object so put the sound before the explosion.
}

//////////////////////////////////////////////////////////////////
// Game time.

DWORD CWorld::GetGameWorldTime( CServTime basetime ) const
{
	// basetime = TICK_PER_SEC time.
	// Get the time of the day in GameWorld minutes
	// 8 real world seconds = 1 game minute.
	// 1 real minute = 7.5 game minutes
	// 3.2 hours = 1 game day.
	return( basetime.GetTimeRaw() / (60*TICK_PER_SEC) );
}

void CWorld::OnTick()
{
	// Do this once per tick.
	// 256 real secs = 1 hour. 19 light levels. check every 10 minutes or so.

	if ( g_Serv.IsLoading() || !Advance() )
		return;

#ifdef DEBUGTIMING
	TIME_PROFILE_INIT;
	TIME_PROFILE_START;
#endif

	if ( m_timeSector <= GetCurrentTime() )
	{
		// Only need a SECTOR_TICK_PERIOD tick to do world stuff.
		m_timeSector = GetCurrentTime() + (TICK_PER_SEC/4);	// Next hit time.
		m_Sector_Pulse ++;
		int	m, s;

		for ( m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] ) continue;

			for ( s = 0; s < g_MapList.GetSectorQty(m); s++ )
			{
				CSector	*pSector = GetSector(m, s);
				if ( !pSector )
					continue;

				//EXC_TRYSUB("Tick");

				pSector->OnTick(m_Sector_Pulse);

				//EXC_CATCHSUB("Sector");
			}
		}

		m_ObjDelete.DeleteAll();	// clean up our delete list.
	}

	m_TimedFunctions.OnTick();

	if ( m_timeRespawn <= GetCurrentTime())
	{
		// Time to regenerate all the dead NPC's in the world.
		m_timeRespawn = GetCurrentTime() + (20*60*TICK_PER_SEC);

		// Respawn dead story NPC's
		CThread *thread = CThread::Thread();
		if ( thread )
			thread->CriticalStart();

		for ( int m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] ) continue;

			for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
			{
				CSector	*pSector = GetSector(m, s);
				if ( pSector )
					pSector->RespawnDeadNPCs();
			}
		}
		if ( thread )
			thread->CriticalEnd();
	}
	if ( m_timeCallUserFunc < GetCurrentTime() )
	{
		if ( g_Cfg.m_iTimerCall )
		{
			//	first timed function call should occure after some time on server load, not just on load
			if ( m_timeCallUserFunc.IsTimeValid() )
			{
				CScriptTriggerArgs args(g_Cfg.m_iTimerCall);
				g_Serv.r_Call("f_onserver_timer", &g_Serv, &args);
			}
			m_timeCallUserFunc = GetCurrentTime() + g_Cfg.m_iTimerCall*60*TICK_PER_SEC;
		}
	}
#ifdef DEBUGTIMING
	TIME_PROFILE_END;
	int	hi = TIME_PROFILE_GET_HI;
	if ( hi > 50 )
		g_Log.Debug("CWorld::OnTick() [ticking sectors] took %d.%d to run\n", hi, TIME_PROFILE_GET_LO);
#endif
}

CSector *CWorld::GetSector(int map, int i)	// gets sector # from one map
{
									// if the map is not supported, return empty sector
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] ) return NULL;

	if ( i >= g_MapList.GetSectorQty(map) )
	{
		g_Log.Error("Unsupported sector #%d for map #%d specifyed.\n", i, map);
		return NULL;
	}

	int base = 0;
	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		if ( m == map )
		{
			if ( g_MapList.GetSectorQty(map) < i ) return NULL;
			return m_Sectors[base + i];
		}
		base += g_MapList.GetSectorQty(m);
	}
	return NULL;
}

CServTime CWorld::GetCurrentTime() const
{
	return m_timeClock;
}

int CWorld::GetTimeDiff( CServTime time ) const
{
	// How long till this event
	// negative = in the past.
	return time.GetTimeDiff(GetCurrentTime()); // Time in TICK_PER_SEC
}

DWORD CWorld::GetSystemClock()
{
	// CLOCKS_PER_SEC is the base units. clock_t should be the type but use DWORD instead.,
#ifdef _WIN32
	return clock();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    DWORD TempTime;
    TempTime = ((((tv.tv_sec - 912818000) * CLOCKS_PER_SEC) +
		tv.tv_usec / CLOCKS_PER_SEC));
	return (TempTime);
#endif
}

void CWorld::InitTime(long lTimeBase)
{
	m_Clock_PrevSys = GetSystemClock();
	m_timeClock.InitTime(lTimeBase);
}

bool CWorld::Advance()
{
	DWORD Clock_Sys = GetSystemClock();	// get the system time.

	int iTimeSysDiff = Clock_Sys - m_Clock_PrevSys;
	iTimeSysDiff = IMULDIV(TICK_PER_SEC, iTimeSysDiff, CLOCKS_PER_SEC);

	if ( !iTimeSysDiff )
		return false;
	else if ( iTimeSysDiff < 0 )	// this is ok for daylight change
	{
		m_Clock_PrevSys = Clock_Sys;
		return false;
	}

	m_Clock_PrevSys = Clock_Sys;

	//	if this overflows, not a harmless
	CServTime Clock_New = m_timeClock + iTimeSysDiff;
	if ( Clock_New < m_timeClock )
	{
		m_timeClock = Clock_New;
		return false;
	}

	m_timeClock = Clock_New;
	return true;
}
