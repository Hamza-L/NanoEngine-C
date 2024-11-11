#ifndef NANOUTILITY_H_
#define NANOUTILITY_H_

#include <stdio.h>
#include <NanoError.h>

#define PRINTFLUSH(content, ...) printf(content, __VA_ARGS__); fflush(stdout)
#define VK_CHECK_THROW(func, msg) if(func != VK_SUCCESS){throw std::runtime_error(msg);}

size_t SizeOf(const char* const* array);

void WriteBinaryFile(const char* filename, void* buffer, long int bufferSize);

char* ReadBinaryFile(const char* filename);

#endif // NANOUTILITY_H_
