#ifndef NANORENDERER_H_
#define NANORENDERER_H_

#include "NanoConfig.h"
#include "NanoScene.h"
#include "NanoWindow.h"
#include <stdint.h>

typedef struct NanoRenderer NanoRenderer;
typedef struct QueueFamilyIndices QueueFamilyIndices;
typedef struct SwapchainDetails SwapchainDetails;
typedef struct SwapchainSyncObjects SwapchainSyncObjects;
typedef struct SwapchainContext SwapchainContext;
typedef struct NanoVKContext NanoVKContext;
typedef struct RenderableObject RenderableObject;
typedef struct FrameData FrameData;

struct FrameData{
    double time;
    double deltaTime;
    int currentFrame; //max is MAX_FRAME_IN_FLIGHT
};

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

    bool framebufferResized;
};

struct NanoVKContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkPhysicalDeviceProperties deviceProperties;

    QueueFamilyIndices queueIndices ;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;

    VkRenderPass defaultRenderpass;

    NanoGraphicsPipeline graphicsPipelines[MAX_GRAPHICS_PIPELINE]; // array of graphicsPipeline
    uint32_t graphicPipelinesCount;
    uint32_t currentGraphicsPipeline;

    VkCommandPool commandPool;

    SwapchainContext swapchainContext;

    FrameData m_frameData;

    bool isInitialized;
    bool framebufferResized;
};

struct NanoRenderer {
    NanoVKContext* m_pNanoContext;
};

ERR InitRenderer(NanoRenderer* nanoRenderer, MeshMemory* meshMemory, ImageMemory* imageMemory, NanoWindow* window);
ERR PreDrawFrame(NanoRenderer* renderer, NanoWindow* window);
ERR DrawFrame(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow);
ERR CleanUpRenderer(NanoRenderer* nanoRenderer);

void RenderScene(struct RenderableScene* scene);

#endif // NANORENDERER_H_
