#ifdef USE_SCRIPT_COMPILER

#include "../graysvr.h"
#include "version.h"
#pragma warning(disable:4096)
#include "zlib/zlib.h"
#pragma warning(default:4096)
#include "CScriptCompiler.h"

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
		dirp = opendir(mask); \
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
	strcpylen(section->sectionName, secname, sizeof(section->sectionName)-1);	\
	memset(section->reserved, 0, sizeof(section->reserved))

//
//	Tables
//

#define	SECTION0(a,b)
#define	SECTIONA(a,b)
#define	SECTIONN(a,b)
#define	SECTIONNA(a,b)
#define INGOREDKEYWORD(_x_)

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

//	ignored keywords
LPCTSTR const CScriptCompiler::sm_szKeywordsIgnored[] =
{
	#undef INGOREDKEYWORD
	#define INGOREDKEYWORD(_x_) _x_,
	"",
	#include "../tables/CScriptCompiler.tbl"
	#undef INGOREDKEYWORD
	#define INGOREDKEYWORD(_x_)
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
	char	zMask[COMPILER_MAXFILEPATH];
	char	zFileName[COMPILER_MAXFILEPATH];
	long	lFind;

	strcpy(zMask, dirName);
#ifdef _WIN32
	strcat(zMask, "*.*");
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
		head.zPrefix[8] = 0;	// to ensure bug-free on invalid file given
		fclose(f);

		//	file is either corrupt -or- not successfuly compiled
		if ( strcmp(head.zPrefix, COMPILER_BINARY_MARK) || !head.bValid )
			return false;

		//	file is modifyed?
		if (( head.lTimestamp != timeModif ) || ( head.lSize != size ))
			return false;

		//	generated by too new compiler?
		if ( !COMPILER_ALLOW_VERSION_MISMATCH && ( head.iVersion > SPHERE_VER_NUM ))
			return false;

		return true;
	}
	return false;
}

void CScriptCompiler::Init()
{
	rawDataCompiled = (BYTE*)calloc(COMPILER_MAX_BUFER, 1);
	GetFilelist(COMPILER_SOURCE_DIR, COMPILER_SOURCE_EXT, m_lScripts);
	GetFilelist(COMPILER_BINARY_DIR, COMPILER_BINARY_EXT, m_lBinaries);
}

bool CScriptCompiler::CompileAll()
{
	char	fileName[COMPILER_MAXFILEPATH];
	char	*p = fileName;
	char	*p1;
	bool	rc = true;

	for ( int i = 0; i < m_lScripts.GetCount(); i++ )
	{
		strcpy(fileName, ((CGString*)m_lScripts.GetAt(i))->GetPtr());
		*strchr(fileName, '.') = 0;		// cut extension
		while ( p = strchr(p, '/') )	// cut off path
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

	sprintf(zBinaryFile, "%s%s" COMPILER_BINARY_EXT, COMPILER_BINARY_DIR, scriptName);

	if ( IsAlreadyCompiled(zScriptFile, zBinaryFile) )
		return true;

	FILE	*in, *out;
	TBinaryHeadRec	head;

	in = fopen(zScriptFile, "r");
	if ( !in )
	{
notfound:
		g_Log.Error("Unable to compile script '%s'. File '%s' not found.\n", scriptName, zScriptFile);
		return false;
	}
	out = fopen(zBinaryFile, "w");
	if ( !out )
	{
		g_Log.Error("Unable to create compiled version for '%s'. Please check if binaries dir '' is accessable.\n",
			scriptName, COMPILER_BINARY_DIR);
		fclose(in);
		return false;
	}

	strcpy(head.zPrefix, COMPILER_BINARY_MARK);
	head.iVersion = SPHERE_VER_NUM;
	FillHeadRecord(zScriptFile, &head);
	head.lCodeSize = 0;
	head.lPackedSize = 0;
	head.bPacked = 0;
	head.bValid = 0;
	memset(head.reserved, 0, sizeof(head.reserved));

	if ( head.lSize >= COMPILER_MAX_BUFER )
	{
		g_Log.Error("Script '%s' too large (%d KBytes). Maximal allowed script could be %d KBytes.\n",
			scriptName, (head.lSize/1024), (COMPILER_MAX_BUFER/1024));
		fclose(in);
		fclose(out);
		return false;
	}

	fwrite(&head, sizeof(head), 1, out);

	char *bufer = (char *)CompileBufer(in, head.lCodeSize);

	//	getting NULL means that we have already reported some error
	//	in CompileBufer func, no need to do it twice
	if ( !bufer )
	{
		fclose(in);
		fclose(out);
		return false;
	}

	//	no need to pack the result if no code was generated
	if ( head.lCodeSize && COMPILER_PACK_BINARY )
	{
		z_uLong mBuferCLen = z_compressBound( head.lCodeSize );
		BYTE * mCBufer = new BYTE[mBuferCLen];

		int error = z_compress2(mCBufer, &mBuferCLen, (BYTE *)bufer, head.lCodeSize, Z_BEST_COMPRESSION);

		if ( error != Z_OK )
		{
			g_Log.Warn("Unable to compress the compiled version for '%s'. Using uncompressed version.\n",
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

void *CScriptCompiler::CompileBufer(FILE *in, long &lCompiledSize)
{
	lCompiledSize = 0;

	char	*z;
	char	*line, *p1;
	TBinarySection	*section = (TBinarySection	*)rawDataCompiled;
	int		linenum;

	z = CThread::Thread()->TempString(COMPILER_MAXLINESINSIDE);
	TEMPSTRING(z1);

	try
	{
		VariableList	defines;		//	list of all #define KEY VAL values
		bool	commented = false;		//	block is commented by / * * / comment?
		bool	commentblock = false;	//	block is commented [COMMENT xxxx]
		bool	ifdeffalse = false;		//	block is denied by #ifdef #endif block?
		long	sectionID = -1;			//	block (section) internal id

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

				//	search for \ tag in the end of the line. if exists, the line is splitted to
				//	several lines, so join them all.
				if ( COMPILER_SPLIT_LINES && ( *p1 == '\\' ))
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
						g_Log.Error("Resulting line too large %d chars from %d allowed. The least of line #%d truncated.\n",
							curlen+addlen, COMPILER_MAXLINESINSIDE * SCRIPT_MAX_LINE_LEN, linenum);
						z1[max(0, (COMPILER_MAXLINESINSIDE * SCRIPT_MAX_LINE_LEN) - curlen - 2)] = 0;
					}

					strcat(line, z1);
					goto Preparse;
				}
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
							g_Log.Error("Precompiler cannot open definitions file '%s' included from line #%d.\n", line, linenum);
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

			for ( int defname = 0; defname < defines.GetCount(); defname++ )
			{
				const VariableList::Variable *pVar = defines.GetAt(defname);
				char *pStr;

				if ( pVar && ( pStr = strstr(line, pVar->GetKey()) ))	//	should be replaced?
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

			if (( *line == '[' ) ||
				( COMPILER_FUNCTIONS_C && ( !strnicmp(line, "function", 8 )) )
				)
			{
				//	We obey the old defaults - stop reading on EOF
				if ( COMPILER_EOF_SECTION && !strnicmp(line, "[eof]", 5) )
					break;

				commentblock = false;
				sectionID = 0;

				char	*p = &line[strlen(line) - 1];
				SKIP_CHARR(p, ']');
				if ( COMPILER_FUNCTIONS_C )		// ignore () at the function declaration
				{
					SKIP_CHARR(p, ')');
					SKIP_CHARR(p, '(');
				}
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
				if ( COMPILER_OLD_COMMENTS && !strcmpi(line, "comment") )
				{
commentThisSection:
					commentblock = true;
					continue;
				}

				//	I got some block without name [ADVANCE], [DEFMESSAGES], etc
				if ( !*p )
				{
					section->sectionID = FindTableSorted(line, sm_szSections0, COUNTOF(sm_szSections0)-1);
					if ( section->sectionID < 0 )
					{
						g_Log.Warn("On line #%d. Section %s is either unknown or syntax is incorrect. The whole section ignored.\n", linenum, line);
						goto commentThisSection;
					}
				} // if ( section with no name )
				else
				{
					//	section with arguments?
					section->sectionID = FindTableSorted(line, sm_szSectionsArgsOnly, COUNTOF(sm_szSectionsArgsOnly)-1);
					if ( section->sectionID >= 0 )
					{
						INIT_SECTION(p);
					} // if section + args
					else
					{
						//	section with name?
						section->sectionID = FindTableSorted(line, sm_szSectionsNormal, COUNTOF(sm_szSectionsNormal)-1);

						if ( section->sectionID < 0 )
						{
							g_Log.Warn("On line #%d. Section %s is unknown. The whole section ignored.\n", linenum, line);
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
							int index = FindTableSorted(p1, sm_szSectionsNormalArgs, COUNTOF(sm_szSectionsNormalArgs)-1);
							if ( index > 0 )
								section->sectionID = index;
						}

					}
				}

			} // if [section]

			else if (
				!strnicmp(line, "ON=", 3) ||
				( COMPILER_TRIGGER_SHORT && ( *line == '@' ) )
				)
			{
				if ( commentblock )
					continue;

				if ( section->sectionID < 0 )
				{
					g_Log.Warn("On line %d. Trigger '%s' outside the section detected. The whole section ignored.\n", linenum, line);
					commentblock = true;
					continue;
				}

				if ( !strnicmp(line, "ON=", 3) )
					line += 3;
				else if ( COMPILER_TRIGGER_SHORT && ( line[1] >= '0' ) && ( line[1] <= '9' ))
					line++;	// skip @ in short notation @1 instead of ON=1

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
					g_Log.Warn("On line %d. Line '%s' outside the section detected. The whole section ignored.\n", linenum, line);
					commentblock = true;
					continue;
				}

				//	split line into LVALUE OP RVALUE (command operation arguments)
				char			*lvalue;
				char			*rvalue;
				OPERATION_TYPE	op = OP_QTY;
				char			*p = line;
				int				brackets = 0;

				try
				{
					while ( *p )
					{
						char	c = tolower(*p);

						//	start of a subquery
						if ( *p == '<' )
						{
							if (( tolower(*(p+1)) < 'a' ) || ( tolower(*(p+1)) > 'z' ))
								throw (long)1;
							brackets++;
						}
						//	end of a subquery
						else if ( *p == '>' )
							brackets--;
						//	left part of the line?
						else if ( !brackets &&
							( c != '.' ) && ( c != '_' ) && ( c != ',' ) && ( c != '(' ) && ( c != ')' ) &&
							(( c < 'a' ) || ( c > 'z')) && (( c < '0' ) || ( c > '9' ))
							)
						{
							char	oper[3];
							memset(oper, 0, sizeof(oper));

							char	cc = tolower(*(p+1));
							oper[0] = cc;
							//	is the operation an operation combined?
							if (
								( cc != ' ' ) && ( cc != '\t' ) &&
								(( cc < 'a' ) || ( cc > 'z' )) && (( cc < '0' ) || ( cc > '9' )) &&
								( cc != '<' ) && ( cc != '(' ) && ( cc != '"' ) &&
								( cc != '{' ) && ( cc != '-' ) && ( cc != '!' )
								)
							{
								oper[1] = *(p+1);
							}

							*p = 0;
							p += strlen(oper);
							SKIP_WHITESPACE(p);

							//	get l-value part
							OPERATION_TYPE type = OP_QTY;
							if ( !strcmp(oper, "=") || !strcmp(oper, "\t") || !strcmp(oper, " "))
								type = OP_DEFAULT;
							else if ( !strcmp(oper, "+=") )
								type = OP_PLUS;
							else if ( !strcmp(oper, "-=") )
								type = OP_MINUS;
							else if ( !strcmp(oper, "*=") )
								type = OP_MULTIPLY;
							else if ( !strcmp(oper, "|=") )
								type = OP_OR;
							else if ( !strcmp(oper, "&=") )
								type = OP_AND;
							else if ( !strcmp(oper, "/=") )
								type = OP_DIVIDE;
							else if ( !strcmp(oper, "%=") )
								type = OP_DIVPART;
							else if ( !strcmp(oper, ".=") )
								type = OP_CONCAT;
							else if ( !strcmp(oper, "-=") )
								type = OP_XOR;
							else if ( !strcmp(oper, "++") )
								type = OP_PLUSPLUS;
							else if ( !strcmp(oper, "--") )
								type = OP_MINUSMINUS;

							if ( type == OP_QTY )
								throw (long)2;

							if ( FindTableSorted(line, sm_szKeywordsIgnored, COUNTOF(sm_szKeywordsIgnored)-1) != -1 )
								throw (long)3;

							lvalue = line;
							rvalue = p;
							op = type;
							break;
						}
						p++;
					} // while

					if ( brackets )
						throw (long)4;

					//	we do not have any operators here, just a simple line
					if ( op == OP_QTY )
					{
						op = OP_DEFAULT;
						lvalue = line;
						rvalue = "";
					}

					//	TODO:
					//	add this line to the set of lines
					g_Log.Debug("%5d %s (%d) %s\n", linenum, lvalue, (int)op, rvalue);
				} // try
				catch ( long l )
				{
					switch ( l )
					{
					case 1:
						g_Log.Error("Unexpected '<' in line %d, line ignored.", linenum);
						break;
					case 2:
						g_Log.Error("Cannot detect operation on line %d, line ignored.", linenum);
						break;
					case 3:
						// useless commands like axis staff
						break;
					case 4:
						g_Log.Error("Command brackets '<>' not closed on line %d, line ignored.", linenum);
						break;
					}
				}
				catch (...)
				{
					g_Log.Error("Exception compiling line %d, line ignored.", linenum);
				}
			} // else (line)
		} // while !feof
	} // try
	catch (...)
	{
		fclose(in);
		g_Log.Error("Unknown exception caught compiling line #%d.\n", linenum);
		return NULL;
	}
	fclose(in);
	return rawDataCompiled;
}

bool CScriptCompiler::Execute(char *scriptName)
{
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
		g_Log.Error("Cannot open compiled script '%s'.\n", scriptName);
		return false;
	}

	fread(&head, sizeof(head), 1, script);
	//	we do not need extra checks here, since the validation
	//	was already checked in IsAlreadyCompiled()
	if ( head.iVersion > SPHERE_VER_NUM )
	{
		g_Log.Error("Script '%s' is generated by newer compiler version. Recompilation recommended.\n", scriptName);
		if ( !COMPILER_ALLOW_VERSION_MISMATCH )
		{
			fclose(script);
			return false;
		}
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

		g_Log.Error("Unable to allocate %d + %d bytes for script '%s'\n", head.lPackedSize, head.lCodeSize, scriptName);
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
			g_Log.Error("Unable to uncompress %d bytes of compressed data to %d bytes buffer for script '%s'\n", head.lPackedSize, head.lCodeSize, scriptName);

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
	//	it just does the following:
	//	1. reads all sections to execute commands in place (line [advance] and other sections)
	//	2. adds labels to the code for executions
	//	3. copyes the unpacked scripts to some memory holder to allow executing portions when needed

	// After executing it, delete.
	delete[] dataToExecute;

	fclose(script);
	delete []rawData;
	if ( rawDataPacked != rawData )
		delete []rawDataPacked;
	return true;
}

#endif
