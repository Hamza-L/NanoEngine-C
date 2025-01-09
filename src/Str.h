#ifndef STR_H_
#define STR_H_

#include <stdint.h>
#include "NanoConfig.h"

#define NULL_STR "\0"

struct String{
    char m_data[DEFAULT_STRING_ALLOC_LENGTH];
    uint32_t m_size;
};

struct HeapString{
    char* m_pData;
    uint32_t m_size;
};

typedef struct String String;
typedef struct HeapString HeapString;

// string
String CreateString(const char* string);
void InitString(String* srcString, const char* string);
void InitHeapString(HeapString* srcString, const char* string);
HeapString AllocHeapString(const char* string);
String* AppendToString(String* srcString, const char* stringToAppend);
HeapString* AppendToHeapString(HeapString* srcString, const char* stringToAppend);
void SubString(String* srcString, int offset, int size);
void TrimStringEnding(String* string);
void CleanUpString(HeapString* stringToClean);

// raw string
int FindRawString(const char* srcString, const char* stringToFind);
int FindLastRawString(const char* srcString, const char* stringToFind);
int AppendToRawString(const char* srcString, const char* stringToApped);
int AppendToRawStringAt(const char* srcString, const char* stringToAdd, int index);
void RawSubString(char* srcString, int offset, int size);

#endif // STR_H_
