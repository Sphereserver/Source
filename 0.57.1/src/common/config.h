#ifndef CONFIG_H
#define CONFIG_H

/***************************************************************************
 *
 *
 *	class Config				stores configurable settings
 *
 *
 ***************************************************************************/

class CGString;
class CTextConsole;

class Config
{
public:
	struct Record
	{
		const char	*m_name;
		const char	*m_descr;
		long		m_val;
	};

	long get(const char *name);			// gets the value of the record
	void loadFrom(const char *fname);	// loads settings from external file

	bool scriptGet(const char *key, CGString &sVal);
	bool scriptSet(const char *key, const char *args, CTextConsole *pSrc);

protected:
	void load();

	static Record	*m_config;			// config records
	static long		m_max;				// number of config records
};

extern Config config;

#endif
