#include "NanoError.h"
#include <stdio.h>
#include <stdlib.h>

void __Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
    if (!expr)
    {
        fprintf(stderr,"Assert failed:\t %s\n", msg);
        fprintf(stderr,"Expected:\t %s\n", expr_str);
        fprintf(stderr,"Source:\t\t %s, line %d\n",file, line);
        abort();
    }
}
