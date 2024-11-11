#include "StrUtil.h"
#include <string.h>
#include <stdio.h>

int FindString(const char* srcString, const char* stringToFind){
    int index = 0;
    char* ptr1 = (char*)srcString;
    char* ptr2 = (char*)stringToFind;

    if(strlen(srcString) < strlen(stringToFind))
        return -1;

    while(ptr1){
        while(*ptr1 && *ptr1 == *ptr2){
            ptr1++;
            ptr2++;
            if(!*ptr2)
                return index;
            if(!*ptr1)
                return -1;
        }
        ptr1++;
        ptr2 = (char*)stringToFind;
        index++;
    }
    return -1;
}

int FindLastString(const char* srcString, const char* stringToFind){
    int srcStringLength = strlen(srcString);
    int stringToFindLength = strlen(stringToFind);
    int index = srcStringLength;

    char* ptr1 = (char*)srcString + srcStringLength - 1;
    char* ptr2 = (char*)stringToFind + stringToFindLength - 1;
    if(srcStringLength == stringToFindLength && *ptr1 == *ptr2)
        return 0;

    if(srcStringLength < stringToFindLength)
        return -1;

    while(ptr1 != srcString){
        while(*ptr1 == *ptr2){
            index--;
            if(ptr2 == stringToFind)
                return index;
            ptr1--;
            ptr2--;
            if(ptr1 == srcString)
                return -1;
        }
        ptr1--;
        ptr2 = (char*)stringToFind + stringToFindLength - 1;
        index--;
    }
    return -1;
}

void SubString(char* srcString, int offset, int size){
    char* substr;
    for(int i = offset; i<(offset + size); i++){
        srcString[i-offset] = srcString[i];
    }
    srcString[size]='\0';
}

// It is up to the caller to make sure src string has enough memory to append the new string
char* AppendString(const char* srcString, const char* stringToAdd){
    int index = 0;
    char* ptr1 = (char*)srcString;
    char* ptr2 = (char*)stringToAdd;
    while(*ptr1){
        ptr1++;
    }

    while(*ptr2){
        *ptr1 = *ptr2;
        ptr1++;
        ptr2++;
    }
    *ptr1 = '\0';

    return ptr1;
}
