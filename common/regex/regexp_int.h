/* Internal regexp header */
#include "regexp.h"

#ifdef REGEXP_UNICODE
#define LIT(a) L##a
typedef unsigned short UCHAR_TYPE;
#else
#define LIT(a) a
typedef unsigned char UCHAR_TYPE;
#endif

/* NOTE: this structure is completely opaque. */
struct tag_regexp {
    int regnsubexp;			/* Internal use only. */
	CHAR_TYPE regstart;			/* Internal use only. */
	CHAR_TYPE reganch;			/* Internal use only. */
	CHAR_TYPE *regmust;			/* Internal use only. */
	int regmlen;			/* Internal use only. */
	CHAR_TYPE program[1];		/* Unwarranted chumminess with compiler. */
};


#ifdef REGEXP_UNICODE
#include <wchar.h>
#include <wctype.h>
#define cstrlen wcslen
#define cstrcspn wcscspn
#define cstrstr wcsstr
#define cstrchr wcschr
#define cstrncpy wcsncpy
#define cstrncmp wcsncmp
#define cstrspn wcsspn
#define cisalnum iswalnum
#define cisalpha iswalpha
#define cisblank iswblank
#define ciscntrl iswcntrl
#define cisdigit iswdigit
#define cisgraph iswgraph
#define cislower iswlower
#define cisprint iswprint
#define cispunct iswpunct
#define cisspace iswspace
#define cisupper iswupper
#define cisxdigit iswxdigit
#define ctolower towlower
#else
#include <ctype.h>
#define cstrlen strlen
#define cstrcspn strcspn
#define cstrstr strstr
#define cstrchr strchr
#define cstrncpy strncpy
#define cstrncmp strncmp
#define cstrspn strspn
#define cisalnum isalnum
#define cisalpha isalpha
#define cisblank isblank
#define ciscntrl iscntrl
#define cisdigit isdigit
#define cisgraph isgraph
#define cislower islower
#define cisprint isprint
#define cispunct ispunct
#define cisspace isspace
#define cisupper isupper
#define cisxdigit isxdigit
#define ctolower tolower
#endif

#define REGEXP_MAXEXP 0x7fff   /* max number of subexpressions */
