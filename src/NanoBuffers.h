#ifndef NANOBUFFERS_H_
#define NANOBUFFERS_H_

#include "MemManager.h"
#include <vulkan/vulkan.h>
#include <stdint.h>

#define DATA_MEMBER_PER_VERTEX 3

typedef struct NanoRenderer NanoRenderer;//forward decl

typedef struct UniformBufferObject UniformBufferObject;
typedef struct NanoVkBufferMemory NanoVkBufferMemory;
typedef struct NanoVkImageMemory NanoVkImageMemory;
typedef struct MeshObject MeshObject;

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

struct MeshObject{
    // host memory
    MeshMemoryObject meshMemory;

    // transform
    mat4 model;

    // Vulkan Memory
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

void CreateMeshObject(Vertex* vertex, uint32_t numVertices, uint32_t* indexData, uint32_t numIndices, MeshObject* meshObject);
void SendMeshObjectToGPUMemory(NanoRenderer* nanoRenderer, MeshObject* meshObject);
void CleanUpMeshObject(NanoRenderer* nanoRenderer, MeshObject* meshObject);

#endif // NANOBUFFERS_H_
