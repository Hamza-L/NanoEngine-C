#ifndef MEMMANAGER_H_
#define MEMMANAGER_H_

#include "NanoConfig.h"
#include "cglm/types.h"
#include <stdint.h>

struct MeshObject;

// DATA_MEMBER_PER_VERTEX = number of members in vertex struct
typedef struct{
    vec3 pos;
    vec4 color;
    vec2 uv;
} Vertex;

typedef struct{
    Vertex* vertexMemStart;
    uint32_t* indexMemStart;

    uint32_t vertexMemSize;
    uint32_t indexMemSize;
} MeshMemoryObject;

typedef struct{
    Vertex* vertexMemory;
    uint32_t* indexMemory;

    uint32_t numVertices;
    uint32_t numIndices;

    MeshMemoryObject memMeshObjects[MAX_MEMORY_MESH_OBJECT];
    uint32_t numMemMeshObjects;

    bool isInitialized;
} MeshMemoryAllocator;

void InitMeshAllocator(MeshMemoryAllocator* memMeshAllocator, uint32_t InitialMemSize);
void AllocateMeshObjectMemory(MeshMemoryAllocator* memMeshAllocator, Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, struct MeshObject* meshObject);

#endif // MEMMANAGER_H_
