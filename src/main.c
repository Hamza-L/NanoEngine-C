#include <stdint.h>
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

void CreateSquare(float offset[2], float size[2], float color[4], Vertex vertices[4], uint32_t indices[6]){

    Vertex vert[4] = {{{ offset[0]          , offset[1] - size[1], 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
                      {{ offset[0] + size[0], offset[1] - size[1], 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
                      {{ offset[0] + size[0], offset[1]          , 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
                      {{ offset[0]          , offset[1]          , 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}}};

    uint32_t ind[6] = { 0, 1, 2, 2, 3, 0 };

    memcpy(vertices, vert, sizeof(Vertex)*4);
    memcpy(indices, ind, sizeof(uint32_t)*6);
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

    MeshObject object1;
    Vertex vertices[4] = {0};
    uint32_t indices[6] = {0};
    float offset[2] = {-0.5f, 0.5f};
    float size[2] = {0.5f, 0.5f};
    float color1[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    CreateSquare( offset, size, color1, vertices, indices);
    AllocateMeshObjectMemory(&nanoEngine.m_meshMemory.meshHostMemory, vertices, 4, indices, 6, &object1);

    MeshObject object2;
    offset[0] = 0.5f; offset[1] = 0.5f;
    float color2[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    CreateSquare( offset, size, color2, vertices, indices);
    AllocateMeshObjectMemory(&nanoEngine.m_meshMemory.meshHostMemory, vertices, 4, indices, 6, &object2);

    SendAllocatedMeshMemoryToGPUMemory(&nanoEngine.m_Renderer, &nanoEngine.m_meshMemory);
    /* SendMeshObjectToGPUMemory(&nanoEngine.m_Renderer, &object); */

    RunEngine(&nanoEngine);

    /* CleanUpMeshObject(&nanoEngine.m_Renderer, &object); */
    CleanUpMeshMemory(&nanoEngine.m_Renderer, &nanoEngine.m_meshMemory);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
