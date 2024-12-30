#ifndef NANOUTILITY_H_
#define NANOUTILITY_H_

#include <stdio.h>
#include <stdlib.h>
#include <NanoError.h>
#include "Str.h"

#define PRINTFLUSH(content, ...) printf(content, __VA_ARGS__); fflush(stdout)
#define VK_CHECK_THROW(func, msg) if(func != VK_SUCCESS){fprintf(stderr, "%s", msg); abort();}

size_t SizeOf(const char* array[]);

void WriteBinaryFile(const char* filename, void* buffer, long int bufferSize);

char* ReadBinaryFile(const char* filename, uint32_t* sizeOfBuffer);

int clamp(int val, int max, int min);

String PrependCWD(const char* relativePath);

bool IsFileInPath(const char* filePath, const char* ifFileNotFoundMsg);

#endif // NANOUTILITY_H_
