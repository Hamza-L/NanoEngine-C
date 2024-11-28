#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "NanoEngine.h"
#include "NanoShader.h"
#include "Str.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"

String* testMemFunction(int numElements){
    String* srcMem;
    srcMem = (String*)calloc(numElements, sizeof(String));
    InitString(&srcMem[0], "Hello");
    InitString(&srcMem[1], "World");
    InitString(&srcMem[2], "I'm");
    InitString(&srcMem[3], "Hamza");
    return srcMem;
}

void testFixedMemFunction(String srcMem[], int numElements){
    InitString(&srcMem[0], "Hello");
    InitString(&srcMem[1], "World");
    InitString(&srcMem[2], "I'm");
    InitString(&srcMem[3], "Hamza");
}

int main(int argc, char *argv[]) {
    if(argc > 1){
        fprintf(stderr, "argument passed in\n");
        if(strcmp(argv[1], "-FSC") == 0){
            fprintf(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);
    RunEngine(&nanoEngine);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
