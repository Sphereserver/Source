#ifndef SIMPLE_COMMAND_H
#define SIMPLE_COMMAND_H

#include <stdio.h>

/***************************************************************************
 *
 *
 *	class SimpleCommand			Provides simple file-oriented command line operations
 *
 *
 ***************************************************************************/

class SimpleCommand
{
public:
	SimpleCommand(int argc, char *argv[]);

	long run();										// executes the simple command

	static void help();								// outputs help text

protected:
	int		m_argc;
	char	**m_argv;
	FILE	*m_in;
	FILE	*m_out;

	void convertRegionFlagsToString();
	void reformatScript();
	void getPeakAndAverage();

	bool openInOut();
	void closeInOut();
};

#endif
