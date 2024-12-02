#ifndef NANOBUFFERS_H_
#define NANOBUFFERS_H_

#include "cglm/types.h"
#include <vulkan/vulkan.h>
#include <stdint.h>

#define DATA_MEMBER_PER_VERTEX 3

typedef struct NanoRenderer NanoRenderer;

typedef struct Vertex Vertex;
typedef struct UniformBufferObject UniformBufferObject;
typedef struct NanoVkBufferMemory NanoVkBufferMemory;
typedef struct NanoVkImageMemory NanoVkImageMemory;
typedef struct Mesh Mesh;

// DATA_MEMBER_PER_VERTEX = number of members in vertex struct
struct Vertex{
    vec3 pos;
    vec3 color;
    vec2 uv;
};

struct UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct NanoVkBufferMemory{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    void* bufferMemoryMapped;
};

struct NanoVkImageMemory{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    void* imageMemoryMapped;
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
void GetAttributeDescriptions(VkVertexInputAttributeDescription vertexInputBindingDescription[DATA_MEMBER_PER_VERTEX]);

NanoVkBufferMemory CreateBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, uint32_t dataSize);
NanoVkImageMemory CreateImageBuffer(NanoRenderer* nanoRenderer, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProperties);
NanoVkBufferMemory CreateVertexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize);
NanoVkBufferMemory CreateIndexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize);
void CreateUniformBuffersWithMappedMem(NanoRenderer* nanoRenderer, NanoVkBufferMemory UniformMemoryToInitialize[], uint32_t numBuffers);
void CleanUpBuffer(NanoRenderer* nanoRenderer, NanoVkBufferMemory* bufferMem);

// copy utility
void CopyBuffer(NanoRenderer* nanoRenderer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void CopyBufferToImage(NanoRenderer* nanoRenderer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

#endif // NANOBUFFERS_H_
