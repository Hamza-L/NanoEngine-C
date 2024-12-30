#ifndef NANOERROR_H_
#define NANOERROR_H_

#include <stdbool.h>

void __Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg);

#ifndef NDEBUG
    #define ASSERT(Expr, Msg) \
         __Assert(#Expr, Expr, __FILE__, __LINE__, Msg)
    #define LOG_MSG(out, ...) fprintf(out, __VA_ARGS__)
#else
    #define ASSERT(Expr, Msg)
    #define LOG_MSG(out, ...)
#endif

// Debugbreak
#if defined(_MSC_VER)
#define DEBUG_BREAK __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP)
#endif



enum ERRLevel {
    NANO_FATAL,
    NANO_WARNING,
    NANO_INFO,
    NANO_DEBUG,
};

enum ERR { OK, NOT_INITIALIZED, NOT_FOUND, WRONG_ARGUMENT, UNDEFINED, INVALID };

typedef enum ERR ERR;
typedef enum ERRLevel ERRLevel;

#endif // NANOERROR_H_
