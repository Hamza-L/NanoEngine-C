#include <stdio.h>
#include <stdlib.h>

#include "MemManager.h"
#include "NanoBuffers.h"
#include "NanoEngine.h"
#include "NanoUtility.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"

void CreateVertexData(NanoEngine* nanoEngine, MeshObject* meshObject){
    int numVertices = 4;
    int numIndices = 6;
    Vertex vertices[4] = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                          {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                          {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
                          {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}};
    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    AllocateMeshObjectMemory(&nanoEngine->m_meshMemAllocator, vertices, numVertices, indices, numIndices, meshObject);
    /* CreateMeshObject(vertices, numVertices, */
    /*                  indices, numIndices, */
    /*                  meshObject); */
}

int main(int argc, char *argv[]) {

    SetVar(argv[0]);
    if(argc > 1){
        fprintf(stderr, "argument passed in\n");
        if(strcmp(argv[1], "-FSC") == 0){
            fprintf(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }
    fprintf(stderr, "ARG0: %s\n", GetArg0());

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);

    MeshObject object;
    CreateVertexData(&nanoEngine, &object);
    SendMeshObjectToGPUMemory(&nanoEngine.m_Renderer, &object);

    RunEngine(&nanoEngine);

    CleanUpMeshObject(&nanoEngine.m_Renderer, &object);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
