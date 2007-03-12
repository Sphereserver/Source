// Unix test case
// Compile with g++ -ldl -o testdll unixtestcase.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#define __CALLTYPE
typedef void * dllhandle_t;
#ifndef IN
	#define IN
	#define OUT
#endif

#define WRAPPER_RESULT_NOQUERYRES 1
#define WRAPPER_INTERNAL_ERROR -1
#define WRAPPER_API_SPECIFIC_ERROR -(0xBEEF)
#define WRAPPER_UNKNOWN_ERROR -(0xDEDE)
#define WRAPPER_SERVER_LOST_ERROR -(0xDEAD)

#define MAX_FIELD_NAME 512
#define MAX_DATA_LENGTH 4096

struct stFieldContainer
{
	char name[MAX_FIELD_NAME];
};

typedef struct stFieldContainer fieldarray_t;

struct stResultContainer
{
	char data[MAX_DATA_LENGTH];
};

typedef struct stResultContainer resultarray_t;

typedef bool (__CALLTYPE *dfRightver_t)();
typedef bool (__CALLTYPE *dfConnected_t)();

int main()
{
    dllhandle_t dhDatabase = (dllhandle_t) dlopen("./mysqldll.so", RTLD_LAZY);
    if ( dhDatabase )
    {
        dfRightver_t pfRightversion = (dfRightver_t) dlsym(dhDatabase, "db_isrightversion");
        dfConnected_t pfIsconnect = (dfConnected_t) dlsym(dhDatabase, "db_isconnected");
        if ( pfRightversion && pfIsconnect )
        {
            printf("Loaded functions result: %d - %d\n", pfRightversion(), pfIsconnect());
        }
        else
        {
            printf("Cannot load the functions\n");
        }
        
        dlclose(dhDatabase);
    }
    else
    {
        printf("Cannot load the dll\n");
    }
    
    return 0;
}
