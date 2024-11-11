#ifndef STRUTIL_H_
#define STRUTIL_H_

int FindString(const char* srcString, const char* stringToFind);
int FindLastString(const char* srcString, const char* stringToFind);
char* AppendString(const char* srcString, const char* stringToFind);
void SubString(char* srcString, int offset, int size);

#endif // STRUTIL_H_
