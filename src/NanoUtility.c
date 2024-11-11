#include "NanoUtility.h"

size_t SizeOf(const char* const* array){
    size_t size = 0;
    while(array[size]){
        size++;
    }
    return size;
}

void WriteBinaryFile(const char* filename, void* buffer, long int bufferSize) {
    FILE* file;

    if ((file = fopen(filename, "wb")) == NULL ) {
        fprintf(stderr, "failed to create file!\n");
        goto close_file;
    }

    int numOfElementsToWrite = 1;
    if(fwrite(buffer, bufferSize, numOfElementsToWrite, file) != numOfElementsToWrite){
        fprintf(stderr, "failed to write file!\n");
        goto close_file;
    }

close_file:
    if(fclose(file) == EOF){
        fprintf(stderr, "failed to close the file!\n");
    }
}

char* ReadBinaryFile(const char* filename) {
    FILE* file;
    char* buffer;
    long int fileSize = 0;

    if ((file = fopen(filename, "rb")) == NULL ) {
        fprintf(stderr, "failed to open file!\n");
        goto close_file;
    }

    if(fseek(file, 0, SEEK_END) != 0){ // get the file size by seeking the end of file
        fprintf(stderr, "failed to seek eof!\n");
        goto close_file;
    } else {
        fileSize = ftell(file); // get file size
        if(fileSize == -1){
            fprintf(stderr, "failed to ftell the file pointer location!\n");
            goto close_file;
        }
        fseek(file, 0, SEEK_SET); // get back to the start of the file
    }

    buffer = (char*)malloc(sizeof(char)*fileSize);

    int numOfElementsToRead = 1;
    if(fread(buffer, fileSize, numOfElementsToRead, file) != numOfElementsToRead){
        fprintf(stderr, "failed to read file!\n");
        goto close_file;
    }

close_file:
    if(fclose(file) == EOF){
        fprintf(stderr, "failed to close the file!\n");
    }

    return buffer;
}
