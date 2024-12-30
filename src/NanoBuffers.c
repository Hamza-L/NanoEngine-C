#include "NanoBuffers.h"
#include "MemManager.h"
#include "NanoError.h"
#include "NanoRenderer.h"
#include "NanoVkUtility.h"
#include "vulkan/vulkan_core.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void GetVertexBindingDescription(VkVertexInputBindingDescription* pVertexInputBindingDescription) {
    pVertexInputBindingDescription->binding = 0;
    pVertexInputBindingDescription->stride = sizeof(Vertex);
    pVertexInputBindingDescription->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void GetAttributeDescriptions(VkVertexInputAttributeDescription vertexInputBindingDescription[DATA_MEMBER_PER_VERTEX]) {
    vertexInputBindingDescription[0].binding = 0;
    vertexInputBindingDescription[0].location = 0;
    vertexInputBindingDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputBindingDescription[0].offset = offsetof(Vertex, pos);

    vertexInputBindingDescription[1].binding = 0;
    vertexInputBindingDescription[1].location = 1;
    vertexInputBindingDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputBindingDescription[1].offset = offsetof(Vertex, normal);

    vertexInputBindingDescription[2].binding = 0;
    vertexInputBindingDescription[2].location = 2;
    vertexInputBindingDescription[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputBindingDescription[2].offset = offsetof(Vertex, color);

    vertexInputBindingDescription[3].binding = 0;
    vertexInputBindingDescription[3].location = 3;
    vertexInputBindingDescription[3].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputBindingDescription[3].offset = offsetof(Vertex, uv);
}

// usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT for vertices
NanoVkBufferMemory CreateBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, uint32_t dataSize){
    NanoVkBufferMemory vertexMem = {};

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //this is only going to be accessed by the graphics queue

    if (vkCreateBuffer(nanoRenderer->m_pNanoContext->device, &bufferInfo, NULL, &vertexMem.buffer) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create vertex buffer!\n");
        abort();
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(nanoRenderer->m_pNanoContext->device, vertexMem.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(nanoRenderer, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(nanoRenderer->m_pNanoContext->device, &allocInfo, nullptr, &vertexMem.bufferMemory) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to allocate vertex buffer memory!\n");
        abort();
    }

    vkBindBufferMemory(nanoRenderer->m_pNanoContext->device, vertexMem.buffer, vertexMem.bufferMemory, 0);

    return vertexMem;
}

NanoVkImageMemory CreateImageBuffer(NanoRenderer* nanoRenderer, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memProperties){
    NanoVkImageMemory imageMem = {};

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage; // will be the object of a copy. will also be used for sampling in-shader
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only used by the transfer enabled queue (ie Graphics queue)
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    if (vkCreateImage(nanoRenderer->m_pNanoContext->device, &imageInfo, nullptr, &imageMem.textureImage) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create image!\n");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(nanoRenderer->m_pNanoContext->device, imageMem.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(nanoRenderer, memRequirements.memoryTypeBits, memProperties);

    if (vkAllocateMemory(nanoRenderer->m_pNanoContext->device, &allocInfo, nullptr, &imageMem.textureImageMemory) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to allocate image memory!\n");
    }

    vkBindImageMemory(nanoRenderer->m_pNanoContext->device, imageMem.textureImage, imageMem.textureImageMemory, 0);

    return imageMem;
}

//TODO: Append the commands to the mainloop's recorded command buffers as an optimized buffer to buffer copy since this command only runs when the device Queue idles
void CopyBuffer(NanoRenderer* nanoRenderer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(nanoRenderer);
    {
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }
    EndAndSubmitSingleTimeCommands(nanoRenderer, commandBuffer);
}

//TODO: Append the commands to the mainloop's recorded command buffers as an optimized buffer to image copy since this command only runs when the device Queue idles
void CopyBufferToImage(NanoRenderer* nanoRenderer, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height){
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(nanoRenderer);
    {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0; //tightly packed = 0. no padding
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(
            commandBuffer,
            srcBuffer,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,// layout the image is currently using. we assume it has already transitioned to the optimal layout for buffer copy
            1,
            &region);
    }
    EndAndSubmitSingleTimeCommands(nanoRenderer, commandBuffer);
}

NanoVkBufferMemory CreateVertexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize) {
    NanoVkBufferMemory vertexMem;
    vertexMem = CreateBuffer(nanoRenderer, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT , memProperties, dataSize);

    NanoVkBufferMemory stagingBufferMem;
    stagingBufferMem = CreateBuffer(nanoRenderer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dataSize);

    void* data;
    vkMapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, dataSize, 0, &data);
    memcpy(data, pData, (size_t)dataSize);
    vkUnmapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    CopyBuffer(nanoRenderer, stagingBufferMem.buffer, vertexMem.buffer, dataSize);

    vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    return vertexMem;
}

NanoVkBufferMemory CreateIndexBuffer(NanoRenderer* nanoRenderer, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize) {
    NanoVkBufferMemory indexMem;
    indexMem = CreateBuffer(nanoRenderer, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT , memProperties, dataSize);

    NanoVkBufferMemory stagingBufferMem;
    stagingBufferMem = CreateBuffer(nanoRenderer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dataSize);

    void* data;
    vkMapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, dataSize, 0, &data);
    memcpy(data, pData, (size_t)dataSize);
    vkUnmapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    CopyBuffer(nanoRenderer, stagingBufferMem.buffer, indexMem.buffer, dataSize);

    vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    return indexMem;
}

void CleanUpBuffer(NanoRenderer* nanoRenderer, NanoVkBufferMemory* bufferMem){
    vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, bufferMem->buffer, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, bufferMem->bufferMemory, nullptr);
    bufferMem->buffer = VK_NULL_HANDLE;
    bufferMem->bufferMemory = VK_NULL_HANDLE;
    bufferMem->bufferMemoryMapped = nullptr;
}

/* void SendMeshObjectToGPUMemory(NanoRenderer* nanoRenderer, MeshObject* meshObject){ */
/*     meshObject->vertexMemory = CreateVertexBuffer(nanoRenderer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, meshObject->meshMemory.vertexMemStart, meshObject->meshMemory.vertexMemSize); */
/*     meshObject->indexMemory = CreateIndexBuffer(nanoRenderer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, meshObject->meshMemory.indexMemStart, meshObject->meshMemory.indexMemSize); */
/* } */

void SendAllocatedMeshMemoryToGPUMemory(NanoRenderer* nanoRenderer, MeshMemory* meshMemory){
    if(!meshMemory->meshHostMemory.isInitialized){
        LOG_MSG(stderr, "Failed to send allocated memory to gpu. Host Memory is not initialized/n");
        DEBUG_BREAK;
        return;
    }

    meshMemory->meshVKMemory.vertexMemory = CreateVertexBuffer(nanoRenderer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, meshMemory->meshHostMemory.vertexMemory, meshMemory->meshHostMemory.numVertices * sizeof(Vertex));
    meshMemory->meshVKMemory.indexMemory = CreateIndexBuffer(nanoRenderer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, meshMemory->meshHostMemory.indexMemory, meshMemory->meshHostMemory.numIndices * sizeof(uint32_t));
    meshMemory->isInitialized = true;
}

/* void CleanUpMeshObject(NanoRenderer* nanoRenderer, MeshObject* meshObject){ */
/*     CleanUpBuffer(nanoRenderer, &meshObject->vertexMemory); */
/*     CleanUpBuffer(nanoRenderer, &meshObject->indexMemory); */
/* } */

void CleanUpMeshVkMemory(NanoRenderer* nanoRenderer, MeshVKMemory* meshMemory){
    // have to wait until device is done executing it's buffered commands in case the buffers are in use
    vkDeviceWaitIdle(nanoRenderer->m_pNanoContext->device);
    CleanUpBuffer(nanoRenderer, &meshMemory->vertexMemory);
    CleanUpBuffer(nanoRenderer, &meshMemory->indexMemory);
}

void CleanUpMeshMemory(NanoRenderer* nanoRenderer, MeshMemory* meshMemory){
    CleanUpMeshHostMemory(&meshMemory->meshHostMemory);
    CleanUpMeshVkMemory(nanoRenderer, &meshMemory->meshVKMemory);
    meshMemory->isInitialized = false;
}
