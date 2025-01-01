#ifndef NANOVKUTILITY_H_
#define NANOVKUTILITY_H_

#include "NanoRenderer.h"

VkCommandBuffer BeginSingleTimeCommands(NanoRenderer* nanoRenderer);
void EndAndSubmitSingleTimeCommands(NanoRenderer* nanoRenderer, VkCommandBuffer commandBuffer);
void TransitionImageLayout(NanoRenderer* nanoRenderer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
uint32_t findMemoryType(NanoRenderer* nanoRenderer, uint32_t typeFilter, VkMemoryPropertyFlags properties);
bool hasStencilComponent(VkFormat format);

#endif // NANOVKUTILITY_H_
