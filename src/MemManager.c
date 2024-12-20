#include "MemManager.h"
#include "NanoConfig.h"
#include "NanoError.h"
#include "NanoBuffers.h"
#include "NanoImage.h"
#include "cglm/mat4.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void InitMeshHostMemory(MeshHostMemory* meshHostMemory, uint32_t InitialMemSize){
    meshHostMemory->vertexMemory = (Vertex*)calloc(sizeof(Vertex), InitialMemSize);
    meshHostMemory->numVertices = 0;
    meshHostMemory->indexMemory = (uint32_t*)calloc(sizeof(uint32_t), InitialMemSize);
    meshHostMemory->numIndices = 0;
    meshHostMemory->numMemMeshObjects = 0;
    meshHostMemory->isInitialized = true;
}

void InitImageHostMemory(ImageHostMemory* imageHostMemory, uint32_t InitialMemSize){
    imageHostMemory->ImageMemory = (char*)malloc(sizeof(char) * InitialMemSize);
    imageHostMemory->numImageMemObjects = 0;
    imageHostMemory->isInitialized = true;
}

void AllocateMeshMemoryObject(MeshHostMemory* meshHostMemory, Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, MeshObject* meshObject){
    if(!meshHostMemory->isInitialized){
        fprintf(stderr, "memoryAllocator not initialized. Cannot allocated MeshObjectMemory/n");
        DEBUG_BREAK;
    }

    MeshMemoryObject* memMeshObj = &meshHostMemory->memMeshObjects[meshHostMemory->numMemMeshObjects];

    memMeshObj->vertexMemStart = &meshHostMemory->vertexMemory[meshHostMemory->numVertices];
    memMeshObj->indexMemStart = &meshHostMemory->indexMemory[meshHostMemory->numIndices];
    memMeshObj->vertexMemSize = sizeof(Vertex)*numVertices;
    memMeshObj->indexMemSize = sizeof(uint32_t)*numIndices;

    meshHostMemory->numMemMeshObjects++;

    memcpy(memMeshObj->vertexMemStart, vertices, sizeof(Vertex)*numVertices);
    memcpy(memMeshObj->indexMemStart, indices, sizeof(uint32_t)*numIndices);

    meshHostMemory->numVertices += numVertices;
    meshHostMemory->numIndices += numIndices;

    meshObject->meshMemory = *memMeshObj;
    glm_mat4_identity(meshObject->model);
}

ImageMemoryObject* GetAllocateImageMemoryObject(ImageHostMemory* imageHostMemory, uint32_t imageDataMemSize){
    if(!imageHostMemory->isInitialized){
        fprintf(stderr, "memoryAllocator not initialized. Cannot get allocated ImageObjectMemory/n");
        DEBUG_BREAK;
    }

    ImageMemoryObject* memMeshObj = &imageHostMemory->imageMemObjects[imageHostMemory->numImageMemObjects];

    memMeshObj->imageData = &imageHostMemory->ImageMemory[imageHostMemory->imageMemSize];
    memMeshObj->imageMemSize = imageDataMemSize;

    imageHostMemory->numImageMemObjects++;
    imageHostMemory->imageMemSize += imageDataMemSize;

    return memMeshObj;
}

void AllocateImageMemoryObject(ImageHostMemory* imageHostMemory, char* imageData, uint32_t imageDataMemSize, NanoImage* imageObject){
    if(!imageHostMemory->isInitialized){
        fprintf(stderr, "memoryAllocator not initialized. Cannot allocated ImageObjectMemory/n");
        DEBUG_BREAK;
    }

    ImageMemoryObject* memMeshObj = &imageHostMemory->imageMemObjects[imageHostMemory->numImageMemObjects];

    memMeshObj->imageData = &imageHostMemory->ImageMemory[imageHostMemory->imageMemSize];
    memMeshObj->imageMemSize = imageDataMemSize;

    imageHostMemory->numImageMemObjects++;
    imageHostMemory->imageMemSize += imageDataMemSize;

    memcpy(memMeshObj->imageData, imageData, imageDataMemSize);

    imageObject->imageMemory = memMeshObj;
}

void CleanUpMeshHostMemory(MeshHostMemory* meshHostMemory){
    free(meshHostMemory->vertexMemory);
    free(meshHostMemory->indexMemory);
    meshHostMemory->isInitialized = false;
    memset(meshHostMemory->memMeshObjects, 0, sizeof(MeshMemoryObject) * MAX_MEMORY_MESH_OBJECT);
    meshHostMemory->numMemMeshObjects = 0;
    meshHostMemory->numIndices = 0;
    meshHostMemory->numVertices = 0;
}

void CleanUpImageHostMemory(ImageHostMemory* imageHostMemory){
    free(imageHostMemory->ImageMemory);
    memset(imageHostMemory->imageMemObjects, 0, sizeof(ImageMemoryObject) * MAX_NUM_ALLOCATED_IMAGES);
    imageHostMemory->numImageMemObjects = 0;
    imageHostMemory->isInitialized = false;
}
