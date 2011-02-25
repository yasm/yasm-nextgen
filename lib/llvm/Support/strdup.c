#include "llvm/Config/config.h"

#ifndef HAVE_STRDUP
char* strdup(const char* s)
{
    char* result = (char*)malloc(strlen(s)+1);
    if (!result)
        return 0;
    strcpy(result, s);
    return result;
}
#endif
