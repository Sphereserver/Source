#ifndef SCRIPT_COMPILER_H
#define SCRIPT_COMPILER_H
#ifdef USE_SCRIPT_COMPILER

#define COMPILER_MAXFILEPATH	256
#define	COMPILER_MAXLINESINSIDE	8

// scripts
#define	COMPILER_SOURCE_EXT		".scp"
	//	configurable settings
#define	COMPILER_SOURCE_DIR		"/projects/sphere/Release/scripts/"

// compiled
#define	COMPILER_BINARY_MARK	"BinSCP"
#define	COMPILER_BINARY_EXT		".bin"
	//	configurable settings
#define	COMPILER_BINARY_DIR		"/projects/sphere/Release/compiled/"

// compilator
#define COMPILER_MAX_BUFER		0x800000L	// 8 Mb of compiled bufer - file could not be larger
	//	configurable settings
#define	COMPILER_PACK_BINARY	0
#define	COMPILER_FUNCTIONS_C	1			// allow C-style functions declaration "function xx" instead of "[function xx]"
#define COMPILER_TRIGGER_SHORT	1			// allow short triggers @Login (istead of "ON=@Login")
#define	COMPILER_SPLIT_LINES	1			// allow splitting line into different using \ character at the end
#define	COMPILER_EOF_SECTION	1			// allow [EOF] section to trigger end-of-file
#define	COMPILER_OLD_COMMENTS	1			// allow [COMMENT] (old comment style)

// execution
	//	configurable settings
#define	COMPILER_ALLOW_VERSION_MISMATCH	1

#define	CCHAR	const char *

class CScriptObject
{
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	CScriptObject();
	virtual ~CScriptObject();
	virtual CCHAR GetName() const = 0;

	//	script triggeted functions
	virtual void onConstruct();					// constructed from script
	virtual void onDestruct();					// no longer in namespace -> clean something
	virtual bool get(CCHAR pKey, CGString &sVal, CTextConsole *pSrc);	// get method
	virtual bool set(CCHAR pKey, CCHAR pArg, CTextConsole *pSrc);		// set method
	virtual TRIGRET_TYPE trigger(CCHAR pTrigName, CTextConsole *pSrc, CScriptTriggerArgs *pArgs = NULL) = 0;
};

//
//	TODO:
// for future versions add supports for servers with compiled scripts only. it means that they will
// have NO script sources at all (good on hosting, etc - better security) but still executable
// something like flag RecomileNever=1 with empty files with same names in scripts dir
//

class CScriptCompiler
{
public:
	static const char *m_sClassName;
	enum OPERATION_TYPE
	{
		OP_DEFAULT = 0,			// =
		OP_PLUS,				// +=
		OP_MINUS,				// -=
		OP_MULTIPLY,			// *=
		OP_OR,					// |=
		OP_AND,					// &=
		OP_DIVIDE,				// /=
		OP_DIVPART,				// %=
		OP_CONCAT,				// .=
		OP_XOR,					// ^=
		OP_PLUSPLUS,			// ++
		OP_MINUSMINUS,			// --
		//...space up to 30 other operators. we need to standartize this buffer for
		// at least some backwards binary compatibility. Do we really need this?
		OP_QTY,
		OP_COMMANDLINE = 30,	// this is a command line like [DEFNAME xxx] ID=0124
	};

	struct TBinaryHeadRec	// head of the binary script
	{
		char	zPrefix[8];		// [  8] "BinSCP"
		long	iVersion;		// [ 12] version number compiled that file
		time_t	lTimestamp;		// [ 16] timestamp of the original script
		long	lSize;			// [ 24] size of the original script
		long	lCodeSize;		// [ 28] size of the code in unpacked type
		long	lPackedSize;	// [ 32] size of the code in packed type
		char	bPacked;		// [ 33] the code is packed?
		char	bValid;			// [ 34] the code is valid? 0 if compilation is not finished
		char	reserved[186];	// [256] reserved for the future contents
	};
	struct TBinarySection	// section in the binary script
	{
		long	sectionID;		// [  4] ID of the section
		long	sectionLines;	// [  8] number of code lines in the section
		char	sectionName[48];// [ 56] name of the section (if applicable)
		long	sectionTriggers;// [ 60] in-section triggers definitions amount
		char	reserved[26];	// [ 96] reserved for the future centents
	};
	// TODO: ????
	struct TBinaryTrigRec	// trigger part of the section
	{
		char	name[24];		// [ 24] trigger name
		long	triggerLines;	// [ 28] number of code lines in the trigger
		char	reserved[8];	// [ 32] reserved for the future contents
	};
	// how to save triggers parts ON=@Login, ON=12,15
	// TODO: ????
	struct TBinaryLineRec	// line of the binary script
	{
		int		llen;			//	length of l-value
		int		rlen;			//	length of r-value
		char	operation;		//	operation
	};
	// next goes the llen + rlen bytes,

	CScriptCompiler();
	~CScriptCompiler();

	void Init();								// initializes context (list of files)

	bool CompileAll();							// compiles all files
	bool CompileFile(char *scriptName);			// * compiles file -> binfile
	bool Execute(char *scriptName);				// * executes the compiled binary to generate structures

protected:
	bool GetFilelist(char *dirName, char *extens, CGStringList &list);
	bool IsAlreadyCompiled(char *sourceName, char *binaryName);
	bool FillHeadRecord(char *sourceName, TBinaryHeadRec *rec = NULL, long *size = NULL, time_t *date = NULL);

	void *CompileBufer(FILE *in, long &lCompiledSize);

protected:
	CGStringList	m_lScripts;
	CGStringList	m_lBinaries;
	BYTE	*rawDataCompiled;

	static LPCTSTR const sm_szSections0[];			//	sections with no arguments
	static LPCTSTR const sm_szSectionsArgsOnly[];	//	sections with no name, but argument
	static LPCTSTR const sm_szSectionsNormal[];		//	sections with name
	static LPCTSTR const sm_szSectionsNormalArgs[];	//	sections with name and argument
	static LPCTSTR const sm_szKeywordsIgnored[];	//	keywords that are ignored (like axis staff, etc)
};

#endif
#endif
