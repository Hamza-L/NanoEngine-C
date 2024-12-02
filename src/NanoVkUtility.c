#include "NanoVkUtility.h"
#include <stdlib.h>
#include <stdio.h>

VkCommandBuffer BeginSingleTimeCommands(NanoRenderer* nanoRenderer) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = nanoRenderer->m_pNanoContext->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(nanoRenderer->m_pNanoContext->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void EndAndSubmitSingleTimeCommands(NanoRenderer* nanoRenderer, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(nanoRenderer->m_pNanoContext->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(nanoRenderer->m_pNanoContext->graphicsQueue);

    vkFreeCommandBuffers(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->commandPool, 1, &commandBuffer);
}

uint32_t findMemoryType(NanoRenderer* nanoRenderer, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(nanoRenderer->m_pNanoContext->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "failed to find suitable memory type!\n");
    abort();
}
