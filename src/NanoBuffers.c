#include "NanoBuffers.h"
#include "NanoGraphics.h"
#include "NanoShader.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint32_t findMemoryType(NanoGraphics* nanoGraphics, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(nanoGraphics->m_pNanoContext->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "failed to find suitable memory type!\n");
    abort();
}

void GetVertexBindingDescription(VkVertexInputBindingDescription* pVertexInputBindingDescription) {
    pVertexInputBindingDescription->binding = 0;
    pVertexInputBindingDescription->stride = sizeof(Vertex);
    pVertexInputBindingDescription->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void GetAttributeDescriptions(VkVertexInputAttributeDescription vertexInputBindingDescription[DATA_PER_VERTEX]) {
    vertexInputBindingDescription[0].binding = 0;
    vertexInputBindingDescription[0].location = 0;
    vertexInputBindingDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputBindingDescription[0].offset = offsetof(Vertex, pos);

    vertexInputBindingDescription[1].binding = 0;
    vertexInputBindingDescription[1].location = 1;
    vertexInputBindingDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputBindingDescription[1].offset = offsetof(Vertex, color);
}

// usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT for vertices
NanoVkBufferMemory CreateBuffer(NanoGraphics* nanoGraphics, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, uint32_t dataSize){
    NanoVkBufferMemory vertexMem;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //this is only going to be accessed by the graphics queue

    if (vkCreateBuffer(nanoGraphics->m_pNanoContext->device, &bufferInfo, NULL, &vertexMem.buffer) != VK_SUCCESS) {
        fprintf(stderr, "failed to create vertex buffer!\n");
        abort();
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(nanoGraphics->m_pNanoContext->device, vertexMem.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(nanoGraphics, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(nanoGraphics->m_pNanoContext->device, &allocInfo, nullptr, &vertexMem.bufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate vertex buffer memory!\n");
        abort();
    }

    vkBindBufferMemory(nanoGraphics->m_pNanoContext->device, vertexMem.buffer, vertexMem.bufferMemory, 0);

    return vertexMem;
}

void CopyBuffer(NanoGraphics* nanoGraphics, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = nanoGraphics->m_pNanoContext->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(nanoGraphics->m_pNanoContext->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    {
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(nanoGraphics->m_pNanoContext->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(nanoGraphics->m_pNanoContext->graphicsQueue);

    vkFreeCommandBuffers(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->commandPool, 1, &commandBuffer);
}

NanoVkBufferMemory CreateVertexBuffer(NanoGraphics* nanoGraphics, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize) {
    NanoVkBufferMemory vertexMem;
    vertexMem = CreateBuffer(nanoGraphics, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT , memProperties, dataSize);

    NanoVkBufferMemory stagingBufferMem;
    stagingBufferMem = CreateBuffer(nanoGraphics, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dataSize);

    void* data;
    vkMapMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, dataSize, 0, &data);
    memcpy(data, pData, (size_t)dataSize);
    vkUnmapMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    CopyBuffer(nanoGraphics, stagingBufferMem.buffer, vertexMem.buffer, dataSize);

    vkDestroyBuffer(nanoGraphics->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    return vertexMem;
}

NanoVkBufferMemory CreateIndexBuffer(NanoGraphics* nanoGraphics, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memProperties, void* pData, uint32_t dataSize) {
    NanoVkBufferMemory vertexMem;
    vertexMem = CreateBuffer(nanoGraphics, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT , memProperties, dataSize);

    NanoVkBufferMemory stagingBufferMem;
    stagingBufferMem = CreateBuffer(nanoGraphics, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dataSize);

    void* data;
    vkMapMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, dataSize, 0, &data);
    memcpy(data, pData, (size_t)dataSize);
    vkUnmapMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    CopyBuffer(nanoGraphics, stagingBufferMem.buffer, vertexMem.buffer, dataSize);

    vkDestroyBuffer(nanoGraphics->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoGraphics->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    return vertexMem;
}

void CleanUpBuffer(NanoGraphics* nanoGraphics, NanoVkBufferMemory* bufferMem){
    vkDestroyBuffer(nanoGraphics->m_pNanoContext->device, bufferMem->buffer, nullptr);
    vkFreeMemory(nanoGraphics->m_pNanoContext->device, bufferMem->bufferMemory, nullptr);
}
