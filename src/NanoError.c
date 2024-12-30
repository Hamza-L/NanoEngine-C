#include "NanoError.h"
#include <stdio.h>
#include <stdlib.h>

void __Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
    if (!expr)
    {
        LOG_MSG(stderr,"Assert failed:\t %s\n", msg);
        LOG_MSG(stderr,"Expected:\t %s\n", expr_str);
        LOG_MSG(stderr,"Source:\t\t %s, line %d\n",file, line);
        abort();
    }
}
