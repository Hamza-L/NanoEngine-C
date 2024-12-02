#ifndef NANOVKUTILITY_H_
#define NANOVKUTILITY_H_

#include "NanoRenderer.h"

VkCommandBuffer BeginSingleTimeCommands(NanoRenderer* nanoRenderer);
void EndAndSubmitSingleTimeCommands(NanoRenderer* nanoRenderer, VkCommandBuffer commandBuffer);
uint32_t findMemoryType(NanoRenderer* nanoRenderer, uint32_t typeFilter, VkMemoryPropertyFlags properties);

#endif // NANOVKUTILITY_H_
