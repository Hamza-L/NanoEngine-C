#include "NanoUtility.h"
#include "NanoConfig.h"
#include "Str.h"
#include <stdlib.h>
#include "string.h"

size_t SizeOf(const char** array){
    size_t size = 0;
    while(*array[size]){
        size++;
    }
    return size;
}

void WriteBinaryFile(const char* filename, void* buffer, long int bufferSize) {
    FILE* file;

    if ((file = fopen(filename, "wb")) == NULL ) {
        LOG_MSG(stderr, "failed to create file!\n");
        goto close_file;
    }

    int numOfElementsToWrite = 1;
    if(fwrite(buffer, bufferSize, numOfElementsToWrite, file) != numOfElementsToWrite){
        LOG_MSG(stderr, "failed to write file!\n");
        goto close_file;
    }

close_file:
    if(fclose(file) == EOF){
        LOG_MSG(stderr, "failed to close the file!\n");
    }
}

char* ReadBinaryFile(const char* filename, uint32_t* sizeOfBuffer) {
    FILE* file = NULL;
    char *buffer = NULL;

    char cwd[512];

    if ((file = fopen(filename, "rb")) == NULL ) {
        LOG_MSG(stderr, "failed to open file at: %s\n", filename);
        return buffer;
    }

    if(fseek(file, 0, SEEK_END) != 0){ // get the file size by seeking the end of file
        LOG_MSG(stderr, "failed to seek eof!\n");
        goto close_file;
    } else {
        *sizeOfBuffer = ftell(file); // get file size
        if(*sizeOfBuffer == -1){
            LOG_MSG(stderr, "failed to ftell the file pointer location!\n");
            goto close_file;
        }
        fseek(file, 0, SEEK_SET); // get back to the start of the file
    }

    buffer = (char*)malloc(sizeof(char) * (*sizeOfBuffer));

    int numOfElementsToRead = 1;
    if(fread(buffer, *sizeOfBuffer, numOfElementsToRead, file) != numOfElementsToRead){
        LOG_MSG(stderr, "failed to read file at %s\n", filename);
        goto close_file;
    }

close_file:
    if(fclose(file) == EOF){
        LOG_MSG(stderr, "failed to close the file!\n");
    }


    return buffer;
}

int clamp(int val, int max, int min){
    ASSERT(max < min, "Clamp used with min value larger than max\n");
    val = val > max ? max : val;
    val = val < min ? min : val;
    return val;
}

String PrependCWD(const char* relativePath){
    String absPath = CreateString(GetArg0());
    AppendToString(&absPath, relativePath);
    return absPath;
}

bool IsFileInPath(const char* filePath, const char* ifFileNotFoundMsg){

    FILE* file;
    bool sourceFileExists = true;

    if ((file = fopen(filePath, "r")) == NULL) {
        LOG_MSG(stderr, "%s\n", ifFileNotFoundMsg);
        sourceFileExists = false;
    } else { //if shader source code does exist
        if(fclose(file) == EOF){
            LOG_MSG(stderr, "failed to close the file!\n");
        }
    }

    return sourceFileExists;
}
