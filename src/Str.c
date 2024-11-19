#include "Str.h"
#include "NanoConfig.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

String CreateString(const char* string){
    String stringToReturn;
    int i = 0;
    while(string[i]){
        if(i >= DEFAULT_STRING_ALLOC_LENGTH) {
            fprintf(stderr, "CreateString(char* string): string too large to be initialized");
            break;
        }
        stringToReturn.m_data[i] = string[i];
        i++;
    }
    stringToReturn.m_data[i] = '\0';
    stringToReturn.m_size = i;

    return stringToReturn;
}

void InitString(String* srcString, const char* string){
    int i = 0;
    while(string[i]){
        if(i >= DEFAULT_STRING_ALLOC_LENGTH) {
            fprintf(stderr, "InitString(String* srcString, const char* string): string too large to be initialized");
            break;
        }
        srcString->m_data[i] = string[i];
        i++;
    }
    srcString->m_data[i] = '\0';
    srcString->m_size = i;
}

void InitHeapString(HeapString* srcString, const char* string){
    srcString->m_pData = (char*)calloc(DEFAULT_STRING_ALLOC_LENGTH, sizeof(char));
    srcString->m_size = 0;

    int i = 0;
    while(string[i]){
        if(i >= DEFAULT_STRING_ALLOC_LENGTH) {
            fprintf(stderr, "InitHeapString(HeapString* srcString, char* string): string too large to be initialized");
            break;
        }
        srcString->m_pData[i] = string[i];
        i++;
    }
    srcString->m_size = i;
}

HeapString AllocHeapString(const char* string){
    HeapString stringToReturn;
    stringToReturn.m_pData = (char*)calloc(DEFAULT_STRING_ALLOC_LENGTH, sizeof(char));
    stringToReturn.m_size = 0;

    int i = 0;
    while(string[i]){
        if(i >= DEFAULT_STRING_ALLOC_LENGTH) {
            fprintf(stderr, "AllocHeapString(char* string): string too large to be initialized");
            break;
        }
        stringToReturn.m_pData[i] = string[i];
        i++;
    }
    stringToReturn.m_size = i;

    return stringToReturn;
}

String* AppendToString(String* srcString, const char* stringToAppend){
    if(srcString->m_size == 0){
    } else {
        srcString->m_size = AppendToRawString(srcString->m_data, stringToAppend);
    }

    return srcString;
}

HeapString* AppendToHeapString(HeapString* srcString, const char* stringToAppend){
    if(srcString->m_size == 0){
        CleanUpString(srcString);
        srcString->m_pData = (char*)calloc(DEFAULT_STRING_ALLOC_LENGTH, sizeof(char));
        srcString->m_size = 0;
    }
    srcString->m_size = AppendToRawString(srcString->m_pData, stringToAppend);

    return srcString;
}

void CleanUpString(HeapString* stringToClean){
    if(stringToClean->m_pData){
        free(stringToClean->m_pData);
        stringToClean->m_size = 0;
    }
}

int FindRawString(const char* srcString, const char* stringToFind){
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

int FindLastRawString(const char* srcString, const char* stringToFind){
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

void RawSubString(char* srcString, int offset, int size){
    char* substr;
    for(int i = offset; i<(offset + size); i++){
        srcString[i-offset] = srcString[i];
    }
    srcString[size]='\0';
}

void SubString(String* srcString, int offset, int size){
    int upperLimit = size > srcString->m_size ? srcString->m_size : size;
    for(int i = 0; i < upperLimit; i++){
        srcString->m_data[i] = srcString->m_data[i+offset];
    }
    srcString->m_data[upperLimit] = '\0';
}

// It is up to the caller to make sure src string has enough memory to append the new string
int AppendToRawString(const char* srcString, const char* stringToAdd){
    int index = 0;
    char* ptr1 = (char*)srcString;
    char* ptr2 = (char*)stringToAdd;
    while(*ptr1){
        ptr1++;
        index++;
    }

    while(*ptr2){
        *ptr1 = *ptr2;
        ptr1++;
        ptr2++;
        index++;
    }
    *ptr1 = '\0';

    return index;
}
