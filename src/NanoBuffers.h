#ifndef NANOBUFFERS_H_
#define NANOBUFFERS_H_

#include "cglm/types.h"
#include "vulkan/vulkan.h"
#include "NanoRenderer.h"
#include <stdint.h>

#define DATA_PER_VERTEX 2

typedef struct Vertex Vertex;
typedef struct NanoVkBufferMemory NanoVkBufferMemory;
typedef struct Mesh Mesh;

// DATA_PER_VERTEX = number of members in vertex struct
struct Vertex{
    vec2 pos;
    vec3 color;
};

struct NanoVkBufferMemory{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
};

struct Mesh{
    Vertex* pVertexData;
    uint32_t* pIndexData;
    uint32_t vertexDataSize;
    uint32_t indexDataSize;
    NanoVkBufferMemory vertexMemory;
    NanoVkBufferMemory indexMemory;
};


void GetVertexBindingDescription(VkVertexInputBindingDescription* pVertexInputBindingDescription);
void GetAttributeDescriptions(VkVertexInputAttributeDescription vertexInputBindingDescription[DATA_PER_VERTEX]);

NanoVkBufferMemory CreateBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, uint32_t dataSize);
NanoVkBufferMemory CreateVertexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize);
NanoVkBufferMemory CreateIndexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize);
void CleanUpBuffer(NanoRenderer* nanoRenderer, NanoVkBufferMemory* bufferMem);

#endif // NANOBUFFERS_H_
