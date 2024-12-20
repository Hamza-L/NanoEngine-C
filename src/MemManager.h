#ifndef MEMMANAGER_H_
#define MEMMANAGER_H_

#include "NanoBuffers.h"
#include "NanoConfig.h"
#include <stdint.h>

typedef struct{
    uint32_t vertexMemStart;
    uint32_t indexMemStart;

    uint32_t vertexMemSize;
    uint32_t indexMemSize;
} MemoryMeshObject;

typedef struct{
    Vertex* VertexMemory;
    uint32_t* indices;

    uint32_t numVertices;
    uint32_t numIndices;

    MemoryMeshObject* memMeshObjects;
    uint32_t numMemMeshObjects;

} MemoryAllocator;

#endif // MEMMANAGER_H_
