#ifndef MEMMANAGER_H_
#define MEMMANAGER_H_

#include "NanoConfig.h"
#include "cglm/types.h"
#include <stdint.h>

struct NanoImage;
struct RenderableObject;

#define DATA_MEMBER_PER_VERTEX 4
typedef struct{
    vec3 pos;
    vec3 normal;
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
} MeshHostMemory;

typedef struct{
    char* imageData;
    uint32_t imageMemSize;
} ImageMemoryObject;

typedef struct{
    char* ImageMemory;
    uint32_t imageMemSize;

    ImageMemoryObject imageMemObjects[MAX_NUM_ALLOCATED_IMAGES];
    uint32_t numImageMemObjects;

    bool isInitialized;
} ImageHostMemory;

void InitMeshHostMemory(MeshHostMemory* meshHostMemory, uint32_t InitialMemSize);
void AllocateMeshMemoryObject(MeshHostMemory* meshHostMemory, Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, MeshMemoryObject* meshObject);
void CleanUpMeshHostMemory(MeshHostMemory* meshHostMemory);

void InitImageHostMemory(ImageHostMemory* imageHostMemory, uint32_t InitialMemSize);
ImageMemoryObject GetAllocateImageMemoryObject(ImageHostMemory* imageHostMemory, uint32_t imageDataMemSize);
void CopyImageDataToAllocatedMemoryObject(ImageHostMemory* imageHostMemory, char* imageData, uint32_t imageDataMemSize, struct NanoImage* imageObject);
void CleanUpImageHostMemory(ImageHostMemory* imageHostMemory);

#endif // MEMMANAGER_H_
