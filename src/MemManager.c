#include "MemManager.h"
#include "NanoError.h"
#include "NanoBuffers.h"
#include "cglm/mat4.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void InitMeshAllocator(MeshMemoryAllocator* memMeshAllocator, uint32_t InitialMemSize){
    memMeshAllocator->vertexMemory = (Vertex*)calloc(sizeof(Vertex), InitialMemSize);
    memMeshAllocator->numVertices = 0;
    memMeshAllocator->indexMemory = (uint32_t*)calloc(sizeof(uint32_t), InitialMemSize);
    memMeshAllocator->numIndices = 0;
    memMeshAllocator->numMemMeshObjects = 0;
    memMeshAllocator->isInitialized = true;
}

void AllocateMeshObjectMemory(MeshMemoryAllocator* memMeshAllocator, Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, struct MeshObject* meshObject){
    if(!memMeshAllocator->isInitialized){
        fprintf(stderr, "memoryAllocator not initialized. Cannot allocated MeshObjectMemory/n");
        DEBUG_BREAK;
    }

    MeshMemoryObject* memMeshObj = &memMeshAllocator->memMeshObjects[memMeshAllocator->numMemMeshObjects];

    memMeshObj->vertexMemStart = &memMeshAllocator->vertexMemory[memMeshAllocator->numVertices];
    memMeshObj->indexMemStart = &memMeshAllocator->indexMemory[memMeshAllocator->numIndices];
    memMeshObj->vertexMemSize = sizeof(Vertex)*numVertices;
    memMeshObj->indexMemSize = sizeof(uint32_t)*numIndices;

    memMeshAllocator->numMemMeshObjects++;

    memcpy(memMeshObj->vertexMemStart, vertices, sizeof(Vertex)*numVertices);
    memcpy(memMeshObj->indexMemStart, indices, sizeof(uint32_t)*numIndices);

    memMeshAllocator->numVertices += numVertices;
    memMeshAllocator->numIndices += numIndices;

    meshObject->meshMemory = *memMeshObj;
    glm_mat4_identity(meshObject->model);
}
