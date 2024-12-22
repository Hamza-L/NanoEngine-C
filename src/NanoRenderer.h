#ifndef NANORENDERER_H_
#define NANORENDERER_H_

#include "NanoConfig.h"
#include "NanoGraphicsPipeline.h"
#include "NanoWindow.h"
#include <stdint.h>

typedef struct NanoRenderer NanoRenderer;
typedef struct QueueFamilyIndices QueueFamilyIndices;
typedef struct SwapchainDetails SwapchainDetails;
typedef struct SwapchainSyncObjects SwapchainSyncObjects;
typedef struct SwapchainContext SwapchainContext;
typedef struct NanoVKContext NanoVKContext;
typedef struct RenderableObject RenderableObject;

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

    bool isInitialized;
};

struct NanoRenderer {
    NanoVKContext* m_pNanoContext;
};

struct RenderableObject {
    MeshMemoryObject meshObject;
    int32_t ID;

    mat4 model;

    // Texture
    NanoImage* albedoTexture;
    NanoImage* normalTexture;
    NanoImage* additionalTexture1;
    NanoImage* additionalTexture2;
};

ERR InitRenderer(NanoRenderer* nanoRenderer, MeshMemory* meshMemory, ImageMemory* imageMemory, NanoWindow* window);
ERR PreDrawFrame(NanoRenderer* renderer, NanoWindow* window);
ERR DrawFrame(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow);
ERR CleanUpRenderer(NanoRenderer* nanoRenderer);

void InitRenderableObject(Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, RenderableObject* renderableObject);
void AddTextureToRenderableObject(NanoImage* image, RenderableObject* renderableObject);

#endif // NANORENDERER_H_
