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

int main(int argc, char *argv[]) {
    /* NanoShader nanoShader; */
    /* InitShader(&nanoShader, "./src/shader/shader.vert"); */
    /* CompileShader(nullptr, &nanoShader, true); */
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


    /* String testString; */
    /* HeapString testHeapString; */
    /* InitString(&testString, "Hello"); */
    /* InitHeapString(&testHeapString, "Hello"); */

    /* AppendToString(&testString, " World!\n"); */
    /* AppendToHeapString(&testHeapString, " World!\n"); */

    /* printf("String: %s", testString.m_data); */
    /* printf("HeapString: %s", testHeapString.m_pData); */
    return EXIT_SUCCESS;
}
