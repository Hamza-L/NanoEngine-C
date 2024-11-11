#ifndef NANOGRAPHICS_H_
#define NANOGRAPHICS_H_

#include "NanoConfig.h"
#include "NanoWindow.h"
#include "NanoGraphicsPipeline.h"
#include <stdint.h>

typedef struct NanoGraphics NanoGraphics;
typedef struct QueueFamilyIndices QueueFamilyIndices;
typedef struct SwapchainDetails SwapchainDetails;
typedef struct SwapchainSyncObjects SwapchainSyncObjects;
typedef struct SwapchainContext SwapchainContext;
typedef struct NanoVKContext NanoVKContext;

struct QueueFamilyIndices{
    int32_t graphicsFamily;
    int32_t presentFamily;
};

struct SwapchainDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceFormatKHR* formats; //array
    uint32_t formats_size;
    VkSurfaceFormatKHR selectedFormat;

    VkPresentModeKHR* presentModes; //array
    uint32_t presentModes_size;
    VkPresentModeKHR selectedPresentMode;

    VkExtent2D currentExtent;

    uint32_t imageCount;
};

struct SwapchainSyncObjects {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

struct SwapchainContext{
    VkSwapchainKHR swapchain;

    struct SwapchainDetails info;
    VkImage* images;
    VkImageView* imageViews;
    VkFramebuffer* framebuffers;

    uint32_t currentFrame;
    VkCommandBuffer commandBuffer[MAX_FRAMES_IN_FLIGHT];
    SwapchainSyncObjects syncObjects[MAX_FRAMES_IN_FLIGHT];
};

struct NanoVKContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    QueueFamilyIndices queueIndices ;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;

    VkRenderPass renderpass;

    NanoGraphicsPipeline graphicsPipelines[MAX_GRAPHICS_PIPELINE]; // array of graphicsPipeline
    uint32_t graphicPipelinesCount;
    uint32_t currentGraphicsPipeline;

    VkCommandPool commandPool;

    SwapchainContext swapchainContext;
};

struct NanoGraphics {
    NanoVKContext* m_pNanoContext;
};

ERR InitGraphics(NanoGraphics* nanoGraphics, NanoWindow* window);
ERR DrawFrame(NanoGraphics* nanoGraphics);
ERR CleanUpGraphics(NanoGraphics* nanoGraphics);

#endif // NANOGRAPHICS_H_
