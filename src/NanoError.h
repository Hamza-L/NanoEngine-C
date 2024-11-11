#ifndef NANOERROR_H_
#define NANOERROR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef NDEBUG
#   define ASSERT(Expr, Msg) \
    __Assert(#Expr, Expr, __FILE__, __LINE__, Msg)
#else
#   define ASSERT(Expr, Msg) ;
#endif

typedef enum ERR ERR;
typedef enum ERRLevel ERRLevel;

enum ERRLevel {
    NANO_FATAL,
    NANO_WARNING,
    NANO_INFO,
    NANO_DEBUG,
};

enum ERR { OK, NOT_INITIALIZED, NOT_FOUND, WRONG_ARGUMENT, UNDEFINED, INVALID };

inline void __Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
    if (!expr)
    {
        fprintf(stderr,"Assert failed:\t %s\n", msg);
        fprintf(stderr,"Expected:\t %s\n", expr_str);
        fprintf(stderr,"Source:\t\t %s, line %d\n",file, line);
        abort();
    }
}


#endif // NANOERROR_H_
