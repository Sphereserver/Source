#include "config.h"
#include "../graysvr.h"

Config config;

/***************************************************************************
 *
 *
 *	class Config				stores configurable settings
 *
 *
 ***************************************************************************/

//	NOTE: due to some bug, there are cases when g_Config.get() function can be called BEFORE
//	the constructor of it is executed. Using static states really fix the probem
Config::Record *Config::m_config = NULL;
long Config::m_max(0);

void Config::load()
{
	//	count number of config lines
#define CONFIG(_a_,_b_,_c_) m_max++;
#include "../tables/config.tbl"
#undef CONFIG

	m_config = (Record *)malloc(m_max * sizeof(Record));

	//	fill all values with defaults
	int index = 0;
#define CONFIG(_a_,_b_,_c_) \
	m_config[index].m_name = _a_; \
	m_config[index].m_val = _b_; \
	m_config[index].m_descr = _c_; \
	index++;
#include "../tables/config.tbl"
#undef CONFIG
}

long Config::get(const char *name)
{
	if ( !m_max )
		load();

	for ( long l = 0; l < m_max; l++ )
	{
#ifdef _WIN32
		//	here intentionally compare const char * via ==, since the compiler is configured to merge constants in one
		//	NOTE: this is true only if using constant string as an argument, not for scripts or console
		if ( name == m_config[l].m_name )
#else
		if ( !strcmpi(name,m_config[l].m_name) )
#endif
		{
			return m_config[l].m_val;
		}
	}
	g_Log.Warn("Config setting for '%s' was not found.\n", name);
	return 0;
}

bool Config::scriptGet(const char *key, CGString &sVal)
{
	for ( long l = 0; l < m_max; l++ )
	{
		if ( !strcmpi(key, m_config[l].m_name) )
		{
			sVal.FormatVal(m_config[l].m_val);
			return true;
		}
	}
	sVal.FormatVal(0);
	return true;
}

bool Config::scriptSet(const char *key, const char *args, CTextConsole *pSrc)
{
	if ( !strcmpi(key, "list") )
	{
		char prevGroup[32];

		pSrc->SysMessagef("%25s :: %10s :: %s\n",
			"Name (case-sensitive)", "Value", "Description");
		for ( long l = 0; l < m_max; l++ )
		{
			if ( !l || strnicmp(prevGroup, m_config[l].m_name, strlen(prevGroup)) )
			{
				strcpylen(prevGroup, m_config[l].m_name, 30);

				char *p = prevGroup;
				while ( *p && ( *p != '.' ))
				{
					*p -= ' ';
					p++;
				}
				*p = 0;
				pSrc->SysMessagef("---[ %s ]---\n", prevGroup);
			}

			pSrc->SysMessagef("%25s :: %10d :: %s\n",
				m_config[l].m_name, m_config[l].m_val, m_config[l].m_descr);
		}
	}
	else
	{
		for ( long l = 0; l < m_max; l++ )
		{
			if ( !strcmpi(key, m_config[l].m_name) )
			{
				m_config[l].m_val = ATOI(args);
				return true;
			}
		}
	}

	return true;
}

void Config::loadFrom(const char *fname)
{
	FILE	*f;
	char	z[512];

	f = fopen(fname, "rt");
	if ( !f )
	{
		g_Log.Error("Cannot open file '%s' for reading.\n", fname);
		return;
	}

	while ( !feof(f) )
	{
		z[0] = 0;
		fgets(z, sizeof(z)-1, f);

		//	accept only lines starting with letter
		if ( z[0] && ( z[0] >= 'a' ) && ( z[0] <= 'z' ))
		{
			char *val = z;
			while ( *val && *val != '=' )
				val++;

			//	format should be setting=value
			if ( *val == '=' )
			{
				*val = 0;
				val++;

				//	apply the setting
				long l;
				for ( l = 0; l < m_max; l++ )
				{
					if ( !strcmpi(z, m_config[l].m_name) )
					{
						m_config[l].m_val = ATOI(val);
						break;
					}
				}
				if ( l == m_max )
					g_Log.Warn("Setting '%s' not found, thus not set\n", z);
			}
		}
	}
	fclose(f);
}