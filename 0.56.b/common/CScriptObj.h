//
// CScriptObj.h
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CSCRIPTOBJ_H
#define _INC_CSCRIPTOBJ_H
#pragma once

	#include "CScript.h"
	#include "../sphere/threads.h"

	class CChar;
	class CScriptTriggerArgs;
	class CScriptObj;

	enum TRIGRUN_TYPE
	{
		TRIGRUN_SECTION_EXEC,	// Just execute this. (first line already read!)
		TRIGRUN_SECTION_TRUE,	// execute this section normally.
		TRIGRUN_SECTION_FALSE,	// Just ignore this whole section.
		TRIGRUN_SINGLE_EXEC,	// Execute just this line or blocked segment. (first line already read!)
		TRIGRUN_SINGLE_TRUE,	// Execute just this line or blocked segment.
		TRIGRUN_SINGLE_FALSE,	// ignore just this line or blocked segment.
	};

	enum TRIGRET_TYPE	// trigger script returns.
	{
		TRIGRET_RET_FALSE = 0,	// default return. (script might not have been handled)
		TRIGRET_RET_TRUE = 1,
		TRIGRET_RET_DEFAULT,	// we just came to the end of the script.
		TRIGRET_ENDIF,
		TRIGRET_ELSE,
		TRIGRET_ELSEIF,
		TRIGRET_RET_HALFBAKED,
		TRIGRET_QTY,
	};

	enum PLEVEL_TYPE		// Priv levels.
	{
		PLEVEL_Guest = 0,		// 0 = This is just a guest account. (cannot PK)
		PLEVEL_Player,			// 1 = Player or NPC.
		PLEVEL_Counsel,			// 2 = Can travel and give advice.
		PLEVEL_Seer,			// 3 = Can make things and NPC's but not directly bother players.
		PLEVEL_GM,				// 4 = GM command clearance
		PLEVEL_Dev,				// 5 = Not bothererd by GM's
		PLEVEL_Admin,			// 6 = Can switch in and out of gm mode. assign GM's
		PLEVEL_Owner,			// 7 = Highest allowed level.
		PLEVEL_QTY,
	};

	class CTextConsole
	{
		// A base class for any class that can act like a console and issue commands.
		// CClient, CChar, CServer, CFileConsole
	protected:
		int OnConsoleKey( CGString & sText, TCHAR nChar, bool fEcho );
	public:
		static const char *m_sClassName;
		// What privs do i have ?
		virtual PLEVEL_TYPE GetPrivLevel() const = 0;
		virtual LPCTSTR GetName() const = 0;	// ( every object must have at least a type name )
		virtual CChar * GetChar() const;	// are we also a CChar ? dynamic_cast ?

		virtual void SysMessage( LPCTSTR pszMessage ) const = 0;	// Feed back message.
		int VSysMessage( LPCTSTR pszFormat, va_list args ) const
		{
			TemporaryString pszTemp;
			size_t ilen = _vsnprintf( pszTemp, pszTemp.realLength(), pszFormat, args );
			SysMessage( pszTemp );
			return( ilen );
		}
		int _cdecl SysMessagef( LPCTSTR pszFormat, ... ) const
		{
			va_list vargs;
			va_start( vargs, pszFormat );
			int iRet = VSysMessage( pszFormat, vargs );
			va_end( vargs );
			return( iRet );
		}
	};

	class CScriptObj
	{
		// This object can be scripted. (but might not be)

	#define SKIP_SEPARATORS(p)	while ( *(p)=='.' ) { (p)++; }	// || ISWHITESPACE(*(p))
	#define SKIP_ARGSEP(p)	while ( *(p)== ',' || isspace(*p) ){ (p)++; }
	#define SKIP_IDENTIFIERSTRING(p) while( _ISCSYM(*p) ){ (p)++; }

		static LPCTSTR const sm_szScriptKeys[];
		static LPCTSTR const sm_szLoadKeys[];
		static LPCTSTR const sm_szVerbKeys[];

	private:
		TRIGRET_TYPE OnTriggerForLoop( CScript &s, int iType, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult );
	public:
		static const char *m_sClassName;
		TRIGRET_TYPE OnTriggerScript( CScript &s, LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL );
		virtual TRIGRET_TYPE OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL )
		{
			UNREFERENCED_PARAMETER(pszTrigName);
			UNREFERENCED_PARAMETER(pSrc);
			UNREFERENCED_PARAMETER(pArgs);
			return( TRIGRET_RET_DEFAULT );
		}
		bool OnTriggerFind( CScript & s, LPCTSTR pszTrigName );
		TRIGRET_TYPE OnTriggerRun( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pReturn );
		TRIGRET_TYPE OnTriggerRunVal( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

		virtual LPCTSTR GetName() const = 0;	// ( every object must have at least a type name )

		// Flags = 1 = html
		int ParseText( TCHAR * pszResponse, CTextConsole * pSrc, int iFlags = 0, CScriptTriggerArgs * pArgs = NULL );

		virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
		virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
		virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
		bool r_Call( LPCTSTR pszFunction, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * psVal = NULL, TRIGRET_TYPE * piRet = NULL );

		bool r_SetVal( LPCTSTR pszKey, LPCTSTR pszVal )
		{
			CScript s( pszKey, pszVal );
			bool result = r_LoadVal( s );
			return result;
		}
		virtual bool r_LoadVal( CScript & s );
		virtual bool r_Load( CScript & s );

		virtual ~CScriptObj()
		{
		}
	};

	class CScriptTriggerArgs : public CScriptObj
	{
		// All the args an event will need.
		static LPCTSTR const sm_szLoadKeys[];
	public:
		static const char *m_sClassName;
		int							m_iN1;		// "ARGN" or "ARGN1" = a modifying numeric arg to the current trigger.
		int							m_iN2;		// "ARGN2" = a modifying numeric arg to the current trigger.
		int							m_iN3;		// "ARGN3" = a modifying numeric arg to the current trigger.

		CScriptObj *				m_pO1;		// "ARGO" or "ARGO1" = object 1
												// these can go out of date ! get deleted etc.

		CGString					m_s1;		// ""ARGS" or "ARGS1" = string 1
		CGString					m_s1_raw;	// RAW, used to build argv in runtime

		CGPtrTypeArray	<LPCTSTR>	m_v;

		CVarDefMap 					m_VarsLocal;// "LOCAL.x" = local variable x
		CVarFloat					m_VarsFloat;// "FLOAT.x" = float local variable x
		CLocalObjMap				m_VarObjs;	// "REFx" = local object x

	public:
		void Init( LPCTSTR pszStr );

		CScriptTriggerArgs() :
			m_iN1(0),  m_iN2(0), m_iN3(0)
		{
			m_pO1 = NULL;
		}

		CScriptTriggerArgs( LPCTSTR pszStr );

		CScriptTriggerArgs( CScriptObj * pObj ) :
			m_iN1(0),  m_iN2(0), m_iN3(0), m_pO1(pObj)
		{
		}
		CScriptTriggerArgs( int iVal1 ) :
			m_iN1(iVal1),  m_iN2(0), m_iN3(0)
		{
			m_pO1 = NULL;
		}
		CScriptTriggerArgs( int iVal1, int iVal2, int iVal3 = 0 ) :
			m_iN1(iVal1), m_iN2(iVal2), m_iN3(iVal3)
		{
			m_pO1 = NULL;
		}
		CScriptTriggerArgs( int iVal1, int iVal2, CScriptObj * pObj ) :
			m_iN1(iVal1), m_iN2(iVal2), m_iN3(0), m_pO1(pObj)
		{
		}

		bool r_Verb( CScript & s, CTextConsole * pSrc );
		bool r_LoadVal( CScript & s );
		bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
		bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
		LPCTSTR GetName() const
		{
			return "ARG";
		}
	};

	class CFileObj : public CScriptObj
	{
		private:
				CFileText * sWrite;
				bool bAppend;
				bool bCreate;
				bool bRead;
				bool bWrite;
				TCHAR * tBuffer;
				CGString * cgWriteBuffer;
				// ----------- //
				static LPCTSTR const sm_szLoadKeys[];
				static LPCTSTR const sm_szVerbKeys[];

		private:
				void SetDefaultMode(void);
				bool FileOpen( LPCTSTR sPath );
				TCHAR * GetReadBuffer(bool);
				CGString * GetWriteBuffer(void);

		public:
				static const char *m_sClassName;
				CFileObj();
				~CFileObj();

		public:
				bool OnTick();
				int FixWeirdness();
				bool IsInUse();
				void FlushAndClose();

				virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
				virtual bool r_LoadVal( CScript & s );
				virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
				virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

				LPCTSTR GetName() const
				{
					return "FILE_OBJ";
				}
	};

	class CFileObjContainer : public CScriptObj
	{
		private:
			std::vector<CFileObj *> sFileList;
			int iFilenumber;
			int iGlobalTimeout;
			int iCurrentTick;
			// ----------- //
			static LPCTSTR const sm_szLoadKeys[];
			static LPCTSTR const sm_szVerbKeys[];

		private:
			CFileObj * GetObjectAt( int );
			void ResizeContainer( int );

		public:
			static const char *m_sClassName;
			CFileObjContainer();
			~CFileObjContainer();

		public:
			int GetFilenumber(void);
			void SetFilenumber(int);

		public:
			bool OnTick();
			int FixWeirdness();

			virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
			virtual bool r_LoadVal( CScript & s );
			virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
			virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

			LPCTSTR GetName() const
			{
				return "FILE_OBJCONTAINER";
			}
	};

#endif	// _INC_CSCRIPTOBJ_H
