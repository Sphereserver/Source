#include "../graysvr/graysvr.h"
#include "../common/grayver.h"
#include "../common/CScriptCompiler.h"
#pragma warning(disable:4096)
#include "../common/zlib/zlib.h"
#pragma warning(default:4096)

//
//	Macroses
//


#ifdef _WIN32

	#include <io.h>
	static struct _finddata_t fileinfo;

	#define GETFILESIZE fileinfo.size
	#define GETFILETIME fileinfo.time_write
	#define CLOSEFIND(a) _findclose(a)
	#define IFGETFILESTAT(name) \
		long lFind = _findfirst(name, &fileinfo); \
		if ( lFind != -1 )
	#define  WHILEFINDNEXT \
		while ( !_findnext(lFind, &fileinfo) )
	#define FINDFIRST(mask) \
		lFind = _findfirst(mask, &fileinfo); \
		if ( lFind == -1 ) return false
	#define FINDFILEINFO
	#define FINDFILENAME fileinfo.name
	#define FINDISDIR ( fileinfo.attrib&_A_SUBDIR )

#else	// UNIX

	#define __USE_BSD		//	we need dirent->d_type to know if found record is directory
							//	this is accessable for linuxes only this way

	#include <unistd.h>
	#include <dirent.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	static struct dirent * fileinfo;
	static DIR * dirp;

	#define GETFILESIZE fileStat.st_size
	#define GETFILETIME fileStat.st_mtime
	#define CLOSEFIND(a)
	#define IFGETFILESTAT(name) \
		struct stat fileStat; \
		if ( stat(name, &fileStat) != -1 )
	#define  WHILEFINDNEXT \
		while ( fileinfo != NULL )
	#define FINDFIRST(mask) \
		dirp = opendir(szFileDir); \
		if ( !dirp ) return false
	#define FINDFILEINFO \
		fileinfo = readdir(dirp); \
		if ( !fileinfo ) break;
	#define FINDFILENAME fileinfo->d_name
	#define FINDISDIR ( fileinfo->d_type == DT_DIR )

#endif

#define	SKIP_WHITESPACE(p)	while (( *p == ' ' ) || ( *p == '\t' )) p++;
#define	SKIP_WHITESPACER(p)	while (( *p == ' ' ) || ( *p == '\t' )) p--;
#define SKIP_CHAR(p, c)		while ( *p == c ) p++;
#define	SKIP_CHARR(p, c)	while ( *p == c ) p--;
#define	ADD_DEFINE(source, target)	\
	{	\
		char	*pX = source; \
		while ( *pX && ( *pX != ' ' ) && ( *pX != '\t' )) pX++; \
		if ( pX ) \
		{ \
			*(pX++) = 0; \
			SKIP_WHITESPACE(pX); \
			target.SetStr(source, false, pX); \
		} else target.SetStr(source, false, ""); \
	}
#define INIT_SECTION(secname)	\
	section->sectionLines = 0;	\
	strcpylen(section->sectionName, secname, COUNTOF(section->sectionName)-1);	\
	memset(section->reserved, 0, sizeof(section->reserved))

//
//	Tables
//

#define	SECTION0(a,b)
#define	SECTIONA(a,b)
#define	SECTIONN(a,b)
#define	SECTIONNA(a,b)

//	sm_szSections0

enum SC_SECTIONS0_TYPE
{
	#undef SECTION0
	#define SECTION0(a,b) SC_SECTIONS0_##a,
	SC_SECTIONS0_0 = 0,
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTION0
	#define	SECTION0(a,b)
	SC_SECTION0_QTY,
};

LPCTSTR const CScriptCompiler::sm_szSections0[SC_SECTION0_QTY+1] =
{
	#undef SECTION0
	#define SECTION0(a,b) b,
	"",
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTION0
	#define	SECTION0(a,b)
	NULL,
};

//	sm_szSectionsArgsOnly

enum SC_SECTIONSA_TYPE
{
	#undef SECTIONA
	#define SECTIONA(a,b) SC_SECTIONSA_##a,
	SC_SECTIONSA_0 = 1024,
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONA
	#define	SECTIONA(a,b)
	SC_SECTIONA_QTY,
};

LPCTSTR const CScriptCompiler::sm_szSectionsArgsOnly[SC_SECTIONA_QTY+1] =
{
	#undef SECTIONA
	#define SECTIONA(a,b) b,
	"",
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONA
	#define	SECTIONA(a,b)
	NULL,
};

//	sm_szSectionsNormal

enum SC_SECTIONSN_TYPE
{
	#undef SECTIONN
	#define SECTIONN(a,b) SC_SECTIONSN_##a,
	SC_SECTIONSN_0 = 2048,
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONN
	#define	SECTIONN(a,b)
	SC_SECTIONN_QTY,
};

LPCTSTR const CScriptCompiler::sm_szSectionsNormal[SC_SECTIONN_QTY+1] =
{
	#undef SECTIONN
	#define SECTIONN(a,b) b,
	"",
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONN
	#define	SECTIONN(a,b)
	NULL,
};

//	sm_szSectionsNormalArgs

enum SC_SECTIONSNA_TYPE
{
	#undef SECTIONNA
	#define SECTIONNA(a,b) SC_SECTIONNA_##a,
	SC_SECTIONSNA_0 = 3072,
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONNA
	#define	SECTIONNA(a,b)
	SC_SECTIONNA_QTY,
};

LPCTSTR const CScriptCompiler::sm_szSectionsNormalArgs[SC_SECTIONNA_QTY+1] =
{
	#undef SECTIONNA
	#define SECTIONNA(a,b) b,
	"",
	#include "../tables/CScriptCompiler.tbl"
	#undef SECTIONNA
	#define	SECTIONNA(a,b)
	NULL,
};


//
//	Actual code
//

CScriptCompiler::CScriptCompiler()
{
	rawDataCompiled = NULL;
}

CScriptCompiler::~CScriptCompiler()
{
	if ( rawDataCompiled )
	{
		free(rawDataCompiled);
		rawDataCompiled = NULL;
	}
}

bool CScriptCompiler::GetFilelist(char *dirName, char *extens, CGStringList &list)
{
	ADDTOCALLSTACK("CScriptCompiler::GetFilelist");
	char	zMask[COMPILER_MAXFILEPATH];
	char	zFileName[COMPILER_MAXFILEPATH];

	strcpy(zMask, dirName);
#ifdef _WIN32
	strcat(zMask, "*.*");
	long	lFind;
#endif
	FINDFIRST(zMask);
	do
	{
		FINDFILEINFO;

		if ( FINDFILENAME [0] == '.' )		// ignore . and ..
			continue;

		if ( FINDISDIR  )
		{
			strcpy(zMask, dirName);
			strcat(zMask, FINDFILENAME);
			strcat(zMask, "/");
			GetFilelist(zMask, extens, list);
		}
		else if ( strstr(FINDFILENAME, extens) )
		{
			strcpy(zFileName, dirName);
			strcat(zFileName, FINDFILENAME);
			list.AddHead(zFileName);
		}

	} WHILEFINDNEXT;
	CLOSEFIND(lFind);
	return true;
}

bool CScriptCompiler::FillHeadRecord(char *sourceName, TBinaryHeadRec *rec, long *size, time_t *date)
{
	ADDTOCALLSTACK("CScriptCompiler::FillHeadRecord");
	IFGETFILESTAT(sourceName)
	{
		if ( rec )
		{
			rec->lTimestamp = GETFILETIME;
			rec->lSize = GETFILESIZE;
		}
		if ( size )
			*size = GETFILESIZE;
		if ( date )
			*date = GETFILETIME;
		CLOSEFIND(lFind);
		return true;
	}
	return false;
}

bool CScriptCompiler::IsAlreadyCompiled(char *sourceName, char *binaryName)
{
	ADDTOCALLSTACK("CScriptCompiler::IsAlreadyCompiled");
	IFGETFILESTAT(sourceName)
	{
		time_t	timeModif = GETFILETIME;
		long	size = GETFILESIZE;
		CLOSEFIND(lFind);

		TBinaryHeadRec	head;
		FILE			*f = fopen(binaryName, "r");
		if ( !f )
			return false;
		fread(&head, sizeof(head), 1, f);
		head.zPrefix[COUNTOF(head.zPrefix) - 1] = 0;	// to ensure bug-free on invalid file given
		fclose(f);

		//	file is either corrupt -or- not successfuly compiled
		if ( strcmp(head.zPrefix, COMPILER_BINARY_MARK) || !head.bValid )
			return false;

		//	file is modifyed?
		if (( head.lTimestamp != timeModif ) || ( head.lSize != size ))
			return false;

#if COMPILER_ALLOW_VERSION_MISMATCH == 0
		//	generated by too new compiler?
		if ( head.iVersion > GRAY_VER_NUM )
			return false;
#endif

		return true;
	}
	return false;
}

void CScriptCompiler::Init()
{
	ADDTOCALLSTACK("CScriptCompiler::Init");
	rawDataCompiled = (BYTE*)calloc(COMPILER_MAX_BUFER, 1);
	GetFilelist(COMPILER_SOURCE_DIR, COMPILER_SOURCE_EXT, m_lScripts);
	GetFilelist(COMPILER_BINARY_DIR, COMPILER_BINARY_EXT, m_lBinaries);
}

bool CScriptCompiler::CompileAll()
{
	ADDTOCALLSTACK("CScriptCompiler::CompileAll");
	char	fileName[COMPILER_MAXFILEPATH];
	char	*p = fileName;
	char	*p1;
	bool	rc = true;

	for ( int i = 0; i < m_lScripts.GetCount(); i++ )
	{
		strcpy(fileName, ((CGString*)m_lScripts.GetAt(i))->GetPtr());
		*strchr(fileName, '.') = 0;		// cut extension
		while ( (p = strchr(p, '/')) != NULL )	// cut off path
		{
			p1 = p;
			p++;
		}
		if ( !CompileFile(p) )
			rc = false;
	}
	return rc;
}

bool CScriptCompiler::CompileFile(char *scriptName)
{
	ADDTOCALLSTACK("CScriptCompiler::CompileFile");
	char	zScriptFile[COMPILER_MAXFILEPATH];
	char	zBinaryFile[COMPILER_MAXFILEPATH];
	int		i;

	strcpy(zScriptFile, scriptName);
	strcat(zScriptFile, COMPILER_SOURCE_EXT);
	for ( i = 0; i < m_lScripts.GetCount(); i++ )
	{
		if ( strstr(((CGString*)m_lScripts.GetAt(i))->GetPtr(), zScriptFile) )
		{
			strcpy(zScriptFile, ((CGString*)m_lScripts.GetAt(i))->GetPtr());
			break;
		}
	}

	if ( i == m_lScripts.GetCount() )		//	source file not found
		goto notfound;

	strcpy(zBinaryFile, COMPILER_BINARY_DIR);
	strcat(zBinaryFile, scriptName);
	strcat(zBinaryFile, COMPILER_BINARY_EXT);

	if ( IsAlreadyCompiled(zScriptFile, zBinaryFile) )
		return true;

	FILE	*in, *out;
	TBinaryHeadRec	head;

	in = fopen(zScriptFile, "r");
	if ( !in )
	{
notfound:
		g_Log.EventError("Unable to compile script '%s'. File not found.\n", scriptName);
		return false;
	}
	out = fopen(zBinaryFile, "w");
	if ( !out )
	{
		g_Log.EventError("Unable to create compiled version for '%s'. Please check if binaries dir '' is accessable.\n",
			scriptName, COMPILER_BINARY_DIR);
		fclose(in);
		return false;
	}

	strcpy(head.zPrefix, COMPILER_BINARY_MARK);
	head.iVersion = GRAY_VER_NUM;
	FillHeadRecord(zScriptFile, &head);
	head.lCodeSize = 0;
	head.lPackedSize = 0;
	head.bPacked = 0;
	head.bValid = 0;
	memset(head.reserved, 0, sizeof(head.reserved));

	if ( head.lSize >= COMPILER_MAX_BUFER )
	{
		g_Log.EventError("Script '%s' too large (%d KBytes). Maximal allowed script could be %d KBytes.\n",
			scriptName, (head.lSize/1024), (COMPILER_MAX_BUFER/1024));
		fclose(in);
		fclose(out);
		return false;
	}

	fwrite(&head, sizeof(head), 1, out);

	char *bufer = (char *)CompileBuffer(in, head.lCodeSize);

	//	getting NULL means that we have already reported some error
	//	in CompileBufer func, no need to do it twice
	if ( !bufer )
	{
		fclose(in);
		fclose(out);
		return false;
	}

#if COMPILER_PACK_BINARY != 0
	//	no need to pack the result if no code was generated
	if ( head.lCodeSize )
	{
		z_uLong mBuferCLen = z_compressBound( head.lCodeSize );
		BYTE * mCBufer = new BYTE[mBuferCLen];

		int error = z_compress2(mCBufer, &mBuferCLen, (BYTE *)bufer, head.lCodeSize, Z_BEST_COMPRESSION);

		if ( error != Z_OK )
		{
			g_Log.EventWarn("Unable to compress the compiled version for '%s'. Using uncompressed version.\n",
			scriptName, COMPILER_BINARY_DIR);
			head.bPacked = 0;
			head.lPackedSize = 0;
		}
		else
		{
			memset(bufer, 0, head.lCodeSize); // This is not necessary i think.
			memcpy(bufer, mCBufer, mBuferCLen);
			head.lPackedSize = mBuferCLen;
			head.bPacked = 1;
		}

		if ( mCBufer )
			delete[] mCBufer;
	}
#endif

	//	write the compiled bufer
	fwrite(bufer, 1, (head.bPacked ? head.lPackedSize : head.lCodeSize), out);

	//	refresh the header
	head.bValid = 1;
	fseek(out, 0, SEEK_SET);
	fwrite(&head, sizeof(head), 1, out);

	fclose(in);
	fclose(out);
	return true;
}

void *CScriptCompiler::CompileBuffer(FILE *in, unsigned long &lCompiledSize)
{
	ADDTOCALLSTACK("CScriptCompiler::CompileBuffer");
	lCompiledSize = 0;

	char	*z = Str_GetTemp(), *z1 = Str_GetTemp();
	char	*line, *p1;
	TBinarySection	*section = (TBinarySection	*)rawDataCompiled;
	int		linenum = 0;

	try
	{
		CVarDefMap	defines;		//	list of all #define KEY VAL values
		bool	commented = false;		//	block is commented by / * * / comment?
		bool	commentblock = false;	//	block is commented [COMMENT xxxx]
		bool	ifdeffalse = false;		//	block is denied by #ifdef #endif block?
		//long	sectionID = -1;			//	block (section) internal id

		while ( !feof(in) )
		{
			*z = 0;
			fgets(line = z, SCRIPT_MAX_LINE_LEN, in);
			linenum++;

			//
			//	Step #1
			//	Preparse line
Preparse:
			SKIP_WHITESPACE(line);

			//	strip short (// and long c-style comments / * * /)
			//	take care not to trigger this for in-string values "some/*thing//someval"
			p1 = line;
			int quotas = 0;

			while ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' ))
			{
				if ( commented )			//	long comments C++ style
				{							//	/ * * / search for ending tag
					if (( *p1 == '*' ) && ( *(p1+1) == '/' ))
					{
						p1 += 2;
						line = p1;
						commented = false;
						continue;
					}
				}
				else
				{
					if ( *p1 == '"' )
						quotas++;
					else if ( !(quotas % 2) )	//	possible place to start a comment?
					{
						if (( *p1 == '/' ) && ( *(p1+1) == '/' ))	// comments?
							break;									//	yes
						if (( *p1 == '/' ) && ( *(p1+1) == '*' ))	// long comments?
						{
							commented = true;
							p1 += 2;
							continue;
						}
					}
				}

				p1++;
			}
			*p1 = 0;
			p1--;

			//	re-strip empty space from both sides of the line
			SKIP_WHITESPACE(line);
			if ( p1 > line )
			{
				SKIP_WHITESPACER(p1);
				*(p1+1) = 0;

#if COMPILER_SPLIT_LINES != 0
				//	search for \ tag in the end of the line. if exists, the line is splitted to
				//	several lines, so join them all.
				if ( *p1 == '\\' )
				{
					*p1 = 0;
					p1--;

					*z1 = 0;
					fgets(z1, SCRIPT_MAX_LINE_LEN, in);

					//	check the resulting length
					int curlen = strlen(line);
					int addlen = strlen(z1);
					if ( addlen + curlen > COMPILER_MAXLINESINSIDE * SCRIPT_MAX_LINE_LEN )
					{
						g_Log.EventError("Resulting line too large %d chars from %d allowed. The least of line #%d truncated.\n",
							curlen+addlen, COMPILER_MAXLINESINSIDE * SCRIPT_MAX_LINE_LEN, linenum);
						z1[maximum(0, (COMPILER_MAXLINESINSIDE * SCRIPT_MAX_LINE_LEN) - curlen - 2)] = 0;
					}

					strcat(line, z1);
					goto Preparse;
				}
#endif
			}

			if ( line >= p1 )
				continue;

			//
			//	Step #2
			//	Follow pre-processor directives
			//	Note: nested #if[n]def/#else/#endif blocks are not (yet) supported

			if ( *line == '#' )
			{
				line++;

				//	#config filename (reads a text file in a format KEY VALUE as defnames)
				if ( !ifdeffalse && !strnicmp(line, "config", 6) )
				{
					line += 6;
					SKIP_WHITESPACE(line);
					if ( *line )
					{
						FILE	*conf = fopen(line, "rt");
						if ( conf )
						{
							while ( !feof(conf) )
							{
								fgets(p1 = z1, SCRIPT_MAX_LINE_LEN, conf);
								SKIP_WHITESPACE(p1);
								if (( tolower(*p1) < 'a' ) || ( tolower(*p1) > 'z' ))
									continue;
								ADD_DEFINE(p1, defines);
							}
							fclose(conf);
						}
						else
						{
							g_Log.EventError("Precompiler cannot open definitions file '%s' included from line #%d.\n", line, linenum);
						}
					}
				}
				//	#define KEY [VALUE] (records the KEY=VALUE as a defname)
				else if ( !ifdeffalse && !strnicmp(line, "define", 6) )
				{
					line += 6;
					SKIP_WHITESPACE(line);
					ADD_DEFINE(line, defines);
				}
				//	#ifdef KEY
				else if ( !strnicmp(line, "ifdef", 5) )
				{
					line += 6;
					SKIP_WHITESPACE(line);
					ifdeffalse = (defines.GetKey(line) == NULL);
				}
				//	#ifndef KEY
				else if ( !strnicmp(line, "ifndef", 6) )
				{
					line += 6;
					SKIP_WHITESPACE(line);
					ifdeffalse = (defines.GetKey(line) != NULL);
				}
				//	#else
				else if ( !strnicmp(line, "else", 4) )
					ifdeffalse = !ifdeffalse;
				//	#endif
				else if ( !strnicmp(line, "endif", 5) )
					ifdeffalse = false;

				continue;
			} // if ( line is '#xxx' )

			if ( ifdeffalse )	//	denied by #ifdef/#ifndef/#else
				continue;

			//
			//	Step #3
			//	Replace defnames to it's values
			//	NOTE: Each one is processed only once, so #define A A should be safe

			for ( size_t defname = 0; defname < defines.GetCount(); defname++ )
			{
				const CVarDefCont *pVar = defines.GetAt(defname);
				if ( !pVar )
					continue;

				char * pStr = strstr(line, pVar->GetKey());
				if ( pStr != NULL )	//	should be replaced?
				{
					const char *l = line;
					int	i = 0;
					while ( l < pStr )					//	left part
						z1[i++] = *(l++);

					l = pVar->GetValStr();				//	replaced part
					while ( *l )
						z1[i++] = *(l++);

					l = pStr + strlen(pVar->GetKey());	//	right
					while ( *l )
						z1[i++] = *(l++);
					z1[i] = 0;

					strcpy(z, z1);
					line = z;
				}
			} // for (#define replacement)

			//
			//	Step #4
			//	Parse code - split it into blocks - sections, events and lines

#if COMPILER_FUNCTIONS != 0
			if ( *line == '[' || !strnicmp(line, "function", 8 ))
#else
			if ( *line == '[' )
#endif
			{
				//	We obey the old defaults - stop reading on EOF
#if COMPILER_EOF_SECTION != 0
				if ( !strnicmp(line, "[eof]", 5) )
					break;
#endif

				commentblock = false;
				//sectionID = 0;

				char	*p = &line[strlen(line) - 1];
				SKIP_CHARR(p, ']');
#if COMPILER_FUNCTIONS_C != 0
				// ignore () at the function declaration
				SKIP_CHARR(p, ')');
				SKIP_CHARR(p, '(');
#endif
				*(p+1) = 0;

				if ( *line == '[' )
					line++;

				p = line;
				while ( *p && ( tolower(*p) >= 'a' ) && ( tolower(*p) <= 'z' ))
					p++;
				//	extract line = "sectionname"
				if ( *p )
				{
					*p = 0;
					p++;
					SKIP_WHITESPACE(p);
				}

				//	[COMMENT] comments the whole block till the next block
#if COMPILER_OLD_COMMENTS != 0
				if ( !strcmpi(line, "comment") )
				{
					goto commentThisSection;
				}
#endif

				//	I got some block without name [ADVANCE], [DEFMESSAGES], etc
				if ( !*p )
				{
					section->sectionID = FindTable(line, sm_szSections0, COUNTOF(sm_szSections0)-1);
					if ( section->sectionID < 0 )
					{
						g_Log.EventError("Compile warning on line #%d. Section %s is either unknown or syntax is incorrect. The whole section ignored.\n", linenum, line);
						
commentThisSection:
						commentblock = true;
						continue;
					}
				} // if ( section with no name )
				else
				{
					//	section with arguments?
					section->sectionID = FindTable(line, sm_szSectionsArgsOnly, COUNTOF(sm_szSectionsArgsOnly)-1);
					if ( section->sectionID >= 0 )
					{
						INIT_SECTION(p);
					} // if section + args
					else
					{
						//	section with name?
						section->sectionID = FindTable(line, sm_szSectionsNormal, COUNTOF(sm_szSectionsNormal)-1);

						if ( section->sectionID < 0 )
						{
							g_Log.EventError("Compile warning on line #%d. Section %s is unknown. The whole section ignored.\n", linenum, line);
							goto commentThisSection;
						}

						//	extract section name->p, possible arguments ->p1
						p1 = p;
						while ( *p && (
							(( tolower(*p1) >= 'a' ) && ( tolower(*p1) <= 'z' )) ||
							(( *p1 >= '0' ) && ( *p1 <= '9' )) ||
							( *p1 == '_' )
							))
							p1++;
						if ( *p1 )
						{
							*p1 = 0;
							p1++;
							SKIP_WHITESPACE(p1);
						}

						INIT_SECTION(p);

						//	possible an extended section block? if so - replace the sectionID by real one
						if ( *p1 && ( section->sectionID == SC_SECTIONSN_DIALOG ))
						{
							int index = FindTable(p1, sm_szSectionsNormalArgs, COUNTOF(sm_szSectionsNormalArgs)-1);
							if ( index > 0 )
								section->sectionID = index;
						}

					}
				}

			} // if [section]

#if COMPILER_TRIGGER_SHORT != 0
			else if ( !strnicmp(line, "ON=", 3) && *line == '@' )
#else
			else if ( !strnicmp(line, "ON=", 3) )
#endif
			{
				if ( commentblock )
					continue;

				if ( section->sectionID < 0 )
				{
					g_Log.EventError("Compiler warning on line %d. Trigger '%s' outside the section detected. The whole section ignored.\n", linenum, line);
					commentblock = true;
				}

				if ( !strnicmp(line, "ON=", 3) )
					line += 3;
#if COMPILER_TRIGGER_SHORT != 0
				else if (( line[1] >= '0' ) && ( line[1] <= '9' ))
					line++;	// skip @ in short notation @1 instead of ON=1
#endif

				//	TODO:
				//	add trigger start to the list

			} // if ON=xxx

			//	normal text/code line
			else
			{
				if ( commentblock )
					continue;

				if ( section->sectionID < 0 )
				{
					g_Log.EventError("Compiler warning on line %d. Line '%s' outside the section detected. The whole section ignored.\n", linenum, line);
					commentblock = true;
				}

				//	TODO:
				//	operate on line
/*
				char *left,*right;
				OPERATION_TYPE op;
				switch ( CScriptLine::LoadLine(line, &left, &right, &op) )
				{
				case 1:	//	just garbage lines like axis staff (ignore)
					break;
				case 0:
					{
						//	TODO:
						//	add this line to the set of lines
						DEBUG_ERR(("%5d %s (%d) %s\n", linenum, left, (int)op, right));
						break;
					}
				case -1:
					Error(linenum, "exception occured, line skipped");
					break;
				case -2:
					Error(linenum, "unexpected '<', line skipped");
					break;
				case -3:
					Error(linenum, "invalid operation %s specifyed, line skipped", left);
					break;
				case -4:
					Warning(linenum, "command %s is depricated, remove it from script safely", left);
					break;
				case -5:
					Error(linenum, "command brackets <> are not closed, line skipped");
					break;
				default:
					Error(linenum, "unknown or unexpected error, line skipped");
				}
*/
			}

		} // while !feof

		//	TODO:
		//	recheck bufer searching for duplicates - sections with same sectionID and sectionName

	} // try
	catch (...)
	{
		fclose(in);
		g_Log.EventError("Unknown exception caught compiling line #%d.\n", linenum);
		return NULL;
	}
	fclose(in);
	return rawDataCompiled;

/*

int CScriptLine::LoadLine(char *line, char **left, char **right, OPERATION_TYPE *op)
{
	ADDTOCALLSTACK("CScriptLine::LoadLine");
	if ( !left || !right || !op || !line || !*line )
		return -1;	// internal error

	char	*p = line;
	int		brackets = 0;
	try
	{
		while ( *p )						//	extract COMMAND OPERATION ARGUMENTS
		{
			char c = tolower(*p);
			if ( *p == '<' )				//	start of a subquery?
			{
				if (( tolower(*(p+1)) < 'a' ) || ( tolower(*(p+1)) > 'z' ))
					return -2;	//	< used for unknown purpose
				brackets++;
			}
			else if ( *p == '>' )
				brackets--;
			else if ( !brackets &&			//	left part of the line?
				( c != '.' ) && ( c != '_' ) && ( c != ',' ) &&
				( c != '(' ) && ( c != ')' ) &&
				(( c < 'a' ) || ( c > 'z' )) &&
				(( c < '0' ) || ( c > '9' ))
				)
			{
				//	found the operation, save it
				char	operation[3];
				memset(operation, 0, sizeof(operation));

				char cc = tolower(*(p+1));
				if (( cc != ' ' ) && ( cc != '\t' ) &&
					(( cc < 'a' ) || ( cc > 'z' )) &&
					(( cc < '0' ) || ( cc > '9' )) &&
					( cc != '<' ) && ( cc != '(' ) && ( cc != '"' ) &&
					( cc != '{' ) && ( cc != '-' ) && ( cc != '!' )
					)
				{							// complicated operation (like += |= etc)
					if ( *(p+2) == '=' )		//	" x=" (extra space before a command)
					{
						operation[0] = cc;
						operation[1] = '=';
						p++;
					}
					else
					{
						operation[0] = c;
						operation[1] = *(p+1);
					}
				}
				else
					operation[0] = c;

				*p = 0;

				p += strlen(operation);
				SKIP_WHITESPACE(p);

				//	parse the operation (get list of the supported ones)
				OPERATION_TYPE type = OP_QTY;
				if ( !strcmp(operation, "=") || !strcmp(operation, "\t") || !strcmp(operation, " "))
					type = OP_DEFAULT;
				else if ( !strcmp(operation, "+=") )
					type = OP_PLUS;
				else if ( !strcmp(operation, "-=") )
					type = OP_MINUS;
				else if ( !strcmp(operation, "*=") )
					type = OP_MULTIPLY;
				else if ( !strcmp(operation, "|=") )
					type = OP_OR;
				else if ( !strcmp(operation, "&=") )
					type = OP_AND;
				else if ( !strcmp(operation, "/=") )
					type = OP_DIVIDE;
				else if ( !strcmp(operation, "%=") )
					type = OP_DIVPART;
				else if ( !strcmp(operation, ".=") )
					type = OP_CONCAT;
				else if ( !strcmp(operation, "-=") )
					type = OP_XOR;
				else if ( !strcmp(operation, "++") )
					type = OP_PLUSPLUS;
				else if ( !strcmp(operation, "--") )
					type = OP_MINUSMINUS;

				if ( type == OP_QTY )
				{
					*left = operation;
					return -3;	//	invalid operation
				}

				if ( !strcmpi(line, "version") )
				{
					*left = line;
					return -4;	//	deprecated command
				}

				*left = line;
				*right = p;
				*op = type;
				goto lastcheck;
			}
			p++;
		}

		if ( brackets )
			return -5;	//	command brackets <> not closed

		//	if we do not meet any = or like that, this means that we have just
		//	"command " syntax, so simulate empty command
		*op = OP_DEFAULT;
		*left = line;
		*right = "";
	}
	catch (...)
	{
		return -1;
	}
lastcheck:
	if ( !strcmpi(line, "category") || !strcmpi(line, "subsection") || !strcmpi(line, "description") )
		return 1;
	return 0;
}


*/
}

bool CScriptCompiler::Execute(char *scriptName)
{
	ADDTOCALLSTACK("CScriptCompiler::Execute");
	TBinaryHeadRec	head;
	char zFileName[COMPILER_MAXFILEPATH];
	FILE	*script;
	char	*rawDataPacked = NULL, *rawData = NULL, *dataToExecute = NULL;

	strcpy(zFileName, COMPILER_BINARY_DIR);
	strcat(zFileName, scriptName);
	strcat(zFileName, COMPILER_BINARY_EXT);

	script = fopen(scriptName, "r");
	if ( !script )
	{
		g_Log.EventError("Cannot open compiled script '%s'.\n", scriptName);
		return false;
	}

	fread(&head, sizeof(head), 1, script);
	//	we do not need extra checks here, since the validation
	//	was already checked in IsAlreadyCompiled()
	if ( head.iVersion > GRAY_VER_NUM )
	{
		g_Log.EventError("Script '%s' is generated by newer, possible incompatible compiler. Recompilation recommended.\n", scriptName);
#if COMPILER_ALLOW_VERSION_MISMATCH == 0
		fclose(script);
		return false;
#endif
	}

	rawData = (char *)new BYTE[head.lCodeSize];
	if ( head.bPacked )
		rawDataPacked = (char *)new BYTE[head.lPackedSize];

	if (( head.bPacked && !rawDataPacked ) || !rawData )
	{
		if ( rawDataPacked )
			delete []rawDataPacked;
		if ( rawData )
			delete []rawData;

		g_Log.EventError("Memory allocation error %d + %d bytes for script '%s'\n", head.lPackedSize, head.lCodeSize, scriptName);
		fclose(script);
		return false;
	}

	if ( head.bPacked )
		rawDataPacked = rawData;
	fread(rawDataPacked, 1, head.bPacked ? head.lPackedSize : head.lCodeSize, script);

	// The data we'll execute.
	dataToExecute = (char *)new BYTE[head.lCodeSize];

	if ( head.bPacked )
	{
		z_uLongf uncompressedSize = head.lCodeSize;
		int error = z_uncompress((BYTE *)dataToExecute, &uncompressedSize, (BYTE *)rawDataPacked, head.lPackedSize);
		if (( head.lCodeSize != uncompressedSize ) || ( error != Z_OK ))
		{
			g_Log.EventError("Unable to uncompress %d bytes of compressed data to %d bytes buffer for script '%s'\n", head.lPackedSize, head.lCodeSize, scriptName);

			delete[] dataToExecute;

			fclose(script);
			delete []rawData;
			if ( rawDataPacked != rawData )
				delete []rawDataPacked;
			return false;
		}
	}
	else
	{
		memcpy(dataToExecute, rawDataPacked, head.lCodeSize);
	}

	//	TODO:
	//	read and execute the data inside dataToExecute

	// After executing it, delete.
	delete[] dataToExecute;

	fclose(script);
	delete []rawData;
	if ( rawDataPacked != rawData )
		delete []rawDataPacked;
	return true;
}
