#include "SimpleCommand.h"
#include "graysvr.h"

/***************************************************************************
 *
 *
 *	class SimpleCommand			Provides simple file-oriented command line operations
 *
 *
 ***************************************************************************/

SimpleCommand::SimpleCommand(int argc, char *argv[])
{
	m_argc = argc;
	m_argv = argv;
	m_in = NULL;
	m_out = NULL;
}

long SimpleCommand::run()
{
	bool ok  = true;

	if ( m_argc < 4 )
		ok = false;
	else
	{
		TEMPSTRING(buf);
		sprintf(buf, "Running simple command '%s' with input file '%s'...\n", m_argv[2], m_argv[3]);
		g_Serv.PrintStr(buf);

		if ( !ok ) ;
#ifdef _WIN32
		else if ( !strcmpi(m_argv[2], "peaks") )
			getPeakAndAverage();
#endif
		else if ( !strcmpi(m_argv[2], "region") )
			convertRegionFlagsToString();
		else if ( !strcmpi(m_argv[2], "reformat") )
			reformatScript();
		else
			ok = false;

		if ( ok )
		{
			g_Serv.PrintStr("\nOperation completed. Result was saved in 'out.scp' file!\n\n");
		}
	}

	if ( !ok )
		help();

	return 100;
}

void SimpleCommand::help()
{
	const char *z = 
		"\nCommand line switches for simple file-oriented commands:\n"
#ifdef _WIN32
		"peaks      runs logs via the mask counting maximal and average amount of players\n"
#endif
		"region     change region flags in file from hex to string\n"
		"reformat   changes formatting of script file to be more readable\n"
		"EXAMPLE: sphereSvr @ <command> <inputfile|mask> [<arguments>]\n\n"
		;
	g_Serv.PrintStr(z);
}

bool SimpleCommand::openInOut()
{
	m_in = fopen(m_argv[3], "rt");
	if ( !m_in )
	{
		g_Serv.PrintStr("Cannot open input file for reading!\n");
		return false;
	}
	else
	{
		m_out = fopen("out.scp", "w");
		if ( !m_out )
		{
			g_Serv.PrintStr("Cannot open output file 'out.scp' for writing!\n");
			return false;
		}
	}
	return true;
}

void SimpleCommand::closeInOut()
{
	if ( m_in )		fclose(m_in);
	if ( m_out )	fclose(m_out);
}

void SimpleCommand::convertRegionFlagsToString()
{
	if ( !openInOut() )
		return;

	TEMPSTRING(z);
	while ( !feof(m_in) )
	{
		z[0] = 0;
		fgets(z, 1024, m_in);
		if ( !strnicmp(z, "FLAGS=", 6) )
		{
			long val = ATOI(z + 6);
			strcpy(z, "FLAGS=");
#define ADD(_a_,_b_,_c_)	if ( val & _a_ ) { \
								if ( strlen(z) > 6 ) strcat(z, "|"); \
								strcat(z, _c_); \
							}
#include "tables/flags_region.tbl"
#undef ADD
			strcat(z, "\n");
		}
		fputs(z, m_out);
	}
	closeInOut();
}

void SimpleCommand::reformatScript()
{
	if ( !openInOut() )
		return;

#define PRINTTABS	for ( int i = 0; i < tabs; i++ ) fputs("\t", m_out)
	TEMPSTRING(zBuf);
	char	*p;
	int		tabs = 0;
	while ( !feof(m_in) )
	{
		*zBuf = 0;
		fgets(zBuf, 4096, m_in);
		p = zBuf;

		while ( *p && (( *p == ' ' ) || ( *p == '\t' )) )
			p++;

		if ( !*p || *p == '\n' || *p == '\r' )
			continue;

		if ( 0 ) ;
		else if ( *p == '[' )
		{
			tabs = 0;
			if ( strnicmp(p, "[eof]", 5) ) fprintf(m_out, "\n%s", p);
		}
		else if ( !strnicmp(p, "ONBUTTON=", 9) )		// onbutton= -> on=
		{
			tabs = 1;
			fprintf(m_out, "\nON=%s", p+9);
		}
		else if ( *p == 'O' && *(p+1) == 'N' && *(p+2) == '=' )
		{
			tabs = 1;
			fprintf(m_out, "\nON=%s", p+3);
		}
		else if ( !strnicmp(p, "while", 5) || !strnicmp(p, "for", 3) || !strnicmp(p, "if", 2) ||
			!strnicmp(p, "do", 2) || !strnicmp(p, "while", 5) || !strnicmp(p, "else", 4) ||
			!strnicmp(p, "elif", 4) || !strnicmp(p, "begin", 5) )
		{
			if ( !strnicmp(p, "el", 2) )
				tabs--;
			PRINTTABS;
			tabs++;
			if ( !strnicmp(p, "elseif", 6) )	// elseif -> elif
			{
				p += 6;
				fputs("elif", m_out);
			}
			fputs(p, m_out);
		}
		else if ( !strnicmp(p, "end", 3) )
		{
			tabs--;
			if ( tabs < 0 )
			{
				fprintf(m_out, "// INVALID tabulation here, extra block closed!\n");
				tabs = 0;
			}
			PRINTTABS;
			if ( !strnicmp(p, "endif", 5) )
				fputs("endif\n", m_out);
			else
				fputs("end\n", m_out);			// end* -> end
		}
		else
		{
			PRINTTABS;
			fprintf(m_out, p);
		}
	}
#undef PRINTTABS
	closeInOut();
}

void SimpleCommand::getPeakAndAverage()
{
#ifdef _WIN32
	static struct _finddata_t fileinfo;
	fileinfo.attrib = _A_NORMAL;
	long lFind = _findfirst(m_argv[3], &fileinfo);
	if ( lFind == -1 )
	{
		g_Serv.PrintStr("Files with this mask have not been found!\n");
		return;
	}
	char path[0x100];
	char fname[0x100];
	char buf[0x400];
	char z[0x100];

	strcpy(path, m_argv[3]);
	{
		char *p = &path[strlen(path)-1];
		strcpy(path, m_argv[3]);
		while (( p > path ) && ( *p != '/' ) && ( *p != '\\' )) p--;
		if ( p > path ) *(p+1) = 0;
	}

	long monthMax = 0;
	double daysMiddle = 0.0;
	double daysCount = 0;
	do
	{
		if ( fileinfo.name[0] == '.' )
			continue;
		FILE *f;
		long maxTotal = 0;
		strcpy(fname, path);
		strcat(fname, fileinfo.name);
		f = fopen(fname, "r");
		double absSumTotal = 0.0;
		double numberOfTotals = 0;
		long lastHH(0), lastMM(0);
		while ( !feof(f) )
		{
			buf[0] = 0;
			fgets(buf, 0x3ff, f);
			if ( buf )
			{
				char *p = strstr(buf, "[Total:");
				if ( p )
				{
					long valueOfTotal = 0;
					p += 7;
					char *p1 = p;
					while ( *p1 && ( *p1 >= '0' ) && ( *p1 <= '9' )) p1++;
					*p1 = 0;
					valueOfTotal = atoi(p);

					//	update MAX
					if ( valueOfTotal > maxTotal )
						maxTotal = valueOfTotal;

					//	update count for middle
					p = buf;
					*(p+2) = 0;
					*(p+5) = 0;
					long hh = atoi(p);
					long mm = atoi(p+3);
					if (( lastHH != hh ) || ( lastMM != mm ))
					{
						absSumTotal += valueOfTotal;
						numberOfTotals++;
						lastHH = hh;
						lastMM = mm;
					}
				}
			}
		}
		fclose(f);
		double middle = (numberOfTotals != 0.0) ? absSumTotal/numberOfTotals : 0.0;
		sprintf(z, "File '%s' had maximal '%d' and average '%2.4f'\n", fileinfo.name, maxTotal, middle);
		g_Serv.PrintStr(z);

		if ( monthMax < maxTotal )
			monthMax = maxTotal;
		daysCount++;
		daysMiddle += middle;
	}
	while ( !_findnext(lFind, &fileinfo));
	_findclose(lFind);

	double middle = (daysCount != 0.0) ? daysMiddle/daysCount : 0.0;
	sprintf(z, "Average mask report result: maximal '%d', average '%2.4f'\n", monthMax, middle);
	g_Serv.PrintStr(z);
#endif
}
