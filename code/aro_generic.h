#ifndef __ARO_GENERIC__
#define __ARO_GENERIC__

#include <stdint.h>

#define ASSERTIONS_ENABLED 1
const int DEBUG_BUILD = 1;
#define DEBUG_BUILD 1

#if ASSERTIONS_ENABLED
#define assert(expr)  if(expr){} else { reportAssertionFailure(#expr,  __FILE__, __LINE__);  __debugbreak();  } 
#else
#define assert(expr)
#endif

#define file_internal static 
#define local_persist static 
#define global_variable static

#define InvalidCodePath assert(!1==1)
#define megabyte(amnt) amnt*1000000

void reportAssertionFailure(char* expr, char* file, int line);
unsigned int safeTruncateUInt64(uint64_t value);

#endif /* __ARO_GENERIC__ */