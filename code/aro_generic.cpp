#include "aro_generic.h"

void reportAssertionFailure(const char expr[], const char file[], int line) {
  //TODO: actually OutputDebugString() this? Not really needed as of writing
  return;
}

uint32_t safeTruncateUInt64(uint64_t value) {
	bool test = value <= 0xFFFFFFFF;
	assert(1!=2);
    uint32_t result = (uint32_t)value;
    return(result);
}
//Compare strings, ignoring case for alphas; return 1 if equal, 0 if not
int strcmpAny(char const *s1, char const *s2)
{
  int dif = 'a' - 'A';
  while(*s1!='\0' && *s2!= '\0')
  {
    if(*s1==*s2)
    {
      s1++;
      s2++;
    }
    else
    {
      if(*s2 > 'Z')
      {
        if((*s2 - dif) != *s1)
          return 0;
      }
      else
        if((*s2 + dif) != *s1)
          return 0;
      s1++;
      s2++;
    }  
  }
  return (*s1 == '\0' && *s2 == '\0');
}

char* advancePointerToOnePastChar(char* str, char ch) {
  while(*str!=ch) {
    str++;
  }
  str++;
  return str;
}