#ifndef NANOBUFFERS_H_
#define NANOBUFFERS_H_

#include "MemManager.h"
#include <vulkan/vulkan.h>
#include <stdint.h>

typedef struct NanoRenderer NanoRenderer;//forward decl

typedef struct UniformBufferObjectDynamic UniformBufferObjectDynamic;
typedef struct UniformBufferObject UniformBufferObject;
typedef struct NanoVkBufferMemory NanoVkBufferMemory;
typedef struct NanoVkImageMemory NanoVkImageMemory;
typedef struct MeshObjectPushConstant MeshObjectPushConstant;
typedef struct MeshObject MeshObject;

struct UniformBufferObject {
    mat4 view;
    mat4 proj;
};

struct UniformBufferObjectDynamic {
  mat4 *model;
  uint32_t dynamicAlignment;
};

struct MeshObjectPushConstant {
	int albedoTextureID;
	int normalTextureID;
	int additionalTextureID;
	int additionalTextureID2;
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

typedef struct{
    NanoVkBufferMemory vertexMemory;
    NanoVkBufferMemory indexMemory;
} MeshVKMemory;

typedef struct{
    MeshVKMemory meshVKMemory;
    MeshHostMemory meshHostMemory;

    bool isInitialized;
} MeshMemory;

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
void SendAllocatedMeshMemoryToGPUMemory(NanoRenderer* nanoRenderer, MeshMemory* meshMemory);
void CleanUpMeshObject(NanoRenderer* nanoRenderer, MeshObject* meshObject);
void CleanUpAllMeshVkMemory(NanoRenderer* nanoRenderer, MeshVKMemory* meshMemory);
void CleanUpAllMeshMemory(NanoRenderer* nanoRenderer, MeshMemory* meshMemory);


#endif // NANOBUFFERS_H_
