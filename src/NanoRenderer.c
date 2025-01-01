#include "NanoRenderer.h"
#include "NanoError.h"
#include "NanoScene.h"
#include "NanoInput.h"
#include "NanoUtility.h"
#include "NanoVkUtility.h"
#include "Str.h"

#include "cglm/mat4.h"
#include "vulkan/vulkan_core.h"
#include <_abort.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

String testToDisplay;

static MeshMemory* s_meshMemoryPtr;
static ImageMemory* s_imageMemoryPtr;
static NanoRenderer* s_nanoRenderer;

static RenderableScene* s_sceneToRender;

bool IsQueueFamilyIndicesValid(QueueFamilyIndices queueFamily) { // helper function to validate queue indices
    return queueFamily.graphicsFamily != -1 && queueFamily.presentFamily != -1;
}


void AddGraphicsPipelineToNanoContext(NanoRenderer* nanoRenderer, const NanoGraphicsPipeline graphicsPipeline){
    if(!graphicsPipeline.m_isInitialized){
        return;
    }

    uint32_t indx = nanoRenderer->m_pNanoContext->currentGraphicsPipeline;
    if(nanoRenderer->m_pNanoContext->graphicPipelinesCount != 0){
        indx++;
    }

    nanoRenderer->m_pNanoContext->graphicsPipelines[indx] = graphicsPipeline;
    nanoRenderer->m_pNanoContext->currentGraphicsPipeline = indx;
    nanoRenderer->m_pNanoContext->graphicPipelinesCount++;
}

VkDebugUtilsMessengerEXT gDebugMessenger;

// We have to look up the address of the debug callback create function ourselves using vkGetInstanceProcAddr
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// We have to look up the address of the debug callback destroy function ourselves using vkGetInstanceProcAddr
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    ERRLevel messageLevel;

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        messageLevel = NANO_DEBUG;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        messageLevel = NANO_INFO;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        messageLevel = NANO_WARNING;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        messageLevel = NANO_FATAL;
        break;
    default:
        messageLevel = NANO_INFO;
        break;
    }

    LOG_MSG(stderr, "%s", pCallbackData->pMessage);
    LOG_MSG(stderr, "Objects involved: \n");

    for (int i = 0; i < pCallbackData->objectCount; i++) {
        LOG_MSG(stderr, "\t%s\n", pCallbackData->pObjects[i].pObjectName);
    }

    LOG_MSG(stderr, "\n");

    if (messageLevel == NANO_FATAL){
        DEBUG_BREAK;
        abort();
    }

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
    createInfo->pUserData = nullptr;
    createInfo->pNext = nullptr;
}

void cleanupSwapChainContext(VkDevice device, SwapchainContext* swapchainContext) {
    for (int i = 0 ; i < swapchainContext->info.imageCount; i++) {
        // clean swapchain framebuffers
        vkDestroyFramebuffer(device, swapchainContext->framebuffers[i], nullptr);
        // clean swapchain imageviews
        vkDestroyImageView(device, swapchainContext->imageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(device, swapchainContext->swapchain, nullptr);
}

void CleanUpRenderer(NanoRenderer* nanoRenderer){

    // wait in case there are any pending tasks
    vkDeviceWaitIdle(nanoRenderer->m_pNanoContext->device);
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[i].renderFinishedSemaphore, nullptr);
        vkDestroyFence(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[i].inFlightFence, nullptr);
    }

    CleanUpImageVkMemory(nanoRenderer, &nanoRenderer->m_pNanoContext->swapchainContext.depthImage);

    CleanUpAllMeshVkMemory(nanoRenderer, &s_meshMemoryPtr->meshVKMemory);

    // clean commandPool and incidently the commandbuffers acquired from them
    // clean graphic pipelines

    vkDestroyCommandPool(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->commandPool, nullptr);

    cleanupSwapChainContext(nanoRenderer->m_pNanoContext->device, &nanoRenderer->m_pNanoContext->swapchainContext);


    // clean renderpass
    vkDestroyRenderPass(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->defaultRenderpass, nullptr);

    vkDestroySurfaceKHR(nanoRenderer->m_pNanoContext->instance, nanoRenderer->m_pNanoContext->surface, nullptr);

    vkDestroyDevice(nanoRenderer->m_pNanoContext->device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(nanoRenderer->m_pNanoContext->instance, gDebugMessenger, nullptr);
    }

    vkDestroyInstance(nanoRenderer->m_pNanoContext->instance, nullptr);

}

static bool checkValidationLayerSupport(const char* validationLayers[]) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    int indx = 0;
    while (*validationLayers[indx]) {
        bool layerFound = false;
        for (int i = 0; i < layerCount ; i++) {
            if (strcmp(validationLayers[indx], availableLayers[i].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
        indx++;
    }

    return true;
}

SwapchainDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainDetails details = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats = (VkSurfaceFormatKHR*)calloc(formatCount, sizeof(VkSurfaceFormatKHR));
        details.formats_size = formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes = (VkPresentModeKHR*)calloc(presentModeCount, sizeof(VkPresentModeKHR));
        details.presentModes_size = presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);
    }

    return details;
}

void getRequiredInstanceExtensions(char* requiredInstanceExtensions[], uint32_t* numOfInstanceExtensions){
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(numOfInstanceExtensions);
    /* printf("num of instance extensions: %d", *numOfInstanceExtensions); */

    // additional instance extension we may want to add
    // 64 additional extensions that we can potentially add
    int extIdx = 0;
    while (*desiredInstanceExtensions[extIdx]){
        requiredInstanceExtensions[extIdx] = (char*)desiredInstanceExtensions[extIdx];
        extIdx++;
    }

    for (uint32_t i = 0; i < *numOfInstanceExtensions; i++) {
        requiredInstanceExtensions[extIdx] = (char*)glfwExtensions[i];
        extIdx++;
    }

    if (enableValidationLayers) {
        requiredInstanceExtensions[extIdx] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extIdx++;
    }

    //null terminated for size measurement
    requiredInstanceExtensions[extIdx] = NULL_STR;

    *numOfInstanceExtensions = extIdx;
}

//WARNING: Need to clean the extensions allocated array
static void getSupportedInstanceExtensions(VkExtensionProperties* extensions, uint32_t* extensionCount) {
    *extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, extensionCount, nullptr);

    extensions = (VkExtensionProperties*)calloc(*extensionCount, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(nullptr, extensionCount, extensions);
}

static void createInstance(const char *applicationName, const char *engineName, VkInstance* instance) {

    if (enableValidationLayers && !checkValidationLayerSupport(desiredValidationLayers)) {
        LOG_MSG(stderr, "Number of Desired Layers %zu\n", SizeOf(desiredValidationLayers));
        LOG_MSG(stderr, "validation layers requested, but not available!");
        abort();
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = engineName;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#if __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    const char* instanceExtensions[MAX_ARRAY_OF_EXTENSIONS] = {};
    uint32_t numInstanceExtensions = 0;
    getRequiredInstanceExtensions((char **)instanceExtensions, &numInstanceExtensions);
    createInfo.enabledExtensionCount = numInstanceExtensions;
    createInfo.ppEnabledExtensionNames = (const char* const*)instanceExtensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = SizeOf(desiredValidationLayers);
        createInfo.ppEnabledLayerNames = (const char *const *)desiredValidationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, instance) != VK_SUCCESS) {
        LOG_MSG(stderr, "Failed to create an instance\n");
        abort();
    }

}

static ERR setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger) {

    if (!enableValidationLayers) {
        return OK;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populateDebugMessengerCreateInfo(&createInfo);

    //TODO: use VK_CHECK_THROW
    VK_CHECK_THROW(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, debugMessenger), "Failed to create Debug Messenger\n");
    return OK; // this is never reached if we use try/catch.
}

ERR findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices* indices) {
    ERR err = NOT_FOUND;
    // Logic to find queue family indices to populate struct with

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices->graphicsFamily = i;
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                indices->presentFamily = i;
        }

        if (IsQueueFamilyIndicesValid(*indices)) {
            return OK;
        }
    }
    return err;
}

ERR checkDeviceExtensionSupport(VkPhysicalDevice device) {

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties availableExtensions[extensionCount] = {};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

    const char* requiredExtensions[MAX_ARRAY_OF_EXTENSIONS] = {};
    int extIdx = 0;
    while (*desiredDeviceExtensions[extIdx]) {
        requiredExtensions[extIdx] = desiredDeviceExtensions[extIdx];
        extIdx++;
    }

    // make sure we go through all the list of desiredExtensions and "check off" all of them
    int numFoundExtensions = 0;
    for (int i = 0; i < extensionCount ; i++) {
        for (int j = 0 ; j < extIdx ; j++){
            if(strcmp(requiredExtensions[j], availableExtensions[i].extensionName) == 0){
                numFoundExtensions++;
                if(numFoundExtensions == extIdx){
                    return OK;
                }
            }
        }
    }

    return NOT_FOUND;
}

int rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices* queueIndices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ){
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    //optional feature. Does not exist on macos
    /* if (!deviceFeatures.geometryShader) { */
    /*     LOG_MSG(stderr, "No Geometry shader support found\n"); */
    /* } */

    if (!deviceFeatures.tessellationShader) {
        LOG_MSG(stderr, "No Tesselation shader support found\n");
    }

    if (!deviceFeatures.samplerAnisotropy) {
        LOG_MSG(stderr, "No Anisotropy support found\n");
    }

    // Application can't function without the required device extensions
    bool extensionsSupported = OK == checkDeviceExtensionSupport(device);
    if (!extensionsSupported) {
        LOG_MSG(stderr, "missing extensions. score = 0\n");
        return 0;
    }

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainDetails swapchainSupport = querySwapChainSupport(device, surface);
        swapchainAdequate = swapchainSupport.formats_size && swapchainSupport.presentModes_size;
        if (!swapchainAdequate) {
            LOG_MSG(stderr, "swapchain not adequate. score = 0\n");
            return 0; // Application can't function without an adequate swapchain extensions
        }
    }

    if (NOT_FOUND == findQueueFamilies(device, surface, queueIndices)) {
        LOG_MSG(stderr, "queue family not found. score = 0\n");
        return 0;
    }

    return score;
}

static void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, QueueFamilyIndices* queueIndices,
                              VkPhysicalDevice* physicalDevice, VkPhysicalDeviceProperties* physicalDeviceProperties) {

    *physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOG_MSG(stderr, "failed to find GPUs with Vulkan support!\n");
        abort();
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // Check if the best candidate is suitable at all
    int bestScore = 0;
    for (int i = 0 ; i < deviceCount ; i++) {
        int score = rateDeviceSuitability(devices[i], surface, queueIndices);
        if (score > 0 && score > bestScore) {
            bestScore = score;
            *physicalDevice = devices[i];
        }
    }

    if (*physicalDevice == VK_NULL_HANDLE) {
        LOG_MSG(stderr, "failed to find a suitable GPU!\n");
        abort();
    } else {
        vkGetPhysicalDeviceProperties(*physicalDevice, physicalDeviceProperties);

        char deviceType[256] = {0};
        switch (physicalDeviceProperties->deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            strcpy(deviceType, "DESCRETE GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            strcpy(deviceType, "INTEGRATED GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            strcpy(deviceType, "VIRTUAL GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            strcpy(deviceType, "CPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            strcpy(deviceType, "OTHER");
            break;
        default:
            strcpy(deviceType, "UNKNOWN");
        }
        LOG_MSG(stderr, "Physical device selected: %s [%s]\n", physicalDeviceProperties->deviceName, deviceType);
    }
}

void createLogicalDeviceAndGetQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                        QueueFamilyIndices indices,
                        VkQueue* graphicsQueue, VkQueue *presentQueue,
                        VkDevice* device) {


    if (!IsQueueFamilyIndicesValid(indices) && NOT_FOUND == findQueueFamilies(physicalDevice, surface, &indices)) {
        LOG_MSG(stderr, "failed to find a graphics familiy queue!");
        abort();
    }

    // we know both the graphics and present indices are valid, so we don't need to worry about checking it again here
    uint32_t numQueueFamilies = 0;
    int32_t uniqueQueueFamilies[2] = {};
    if(indices.graphicsFamily == indices.presentFamily){
        uniqueQueueFamilies[0] = indices.graphicsFamily;
        numQueueFamilies = 1;
    } else {
        uniqueQueueFamilies[0] = indices.graphicsFamily;
        uniqueQueueFamilies[1] = indices.presentFamily;
        numQueueFamilies = 2;
    }

    VkDeviceQueueCreateInfo queueCreateInfos[numQueueFamilies] = {};

    float queuePriority = 1.0f;
    for (int i = 0; i < numQueueFamilies ; i++) {
        if (uniqueQueueFamilies[i] == -1)
            continue;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = numQueueFamilies;

    VkPhysicalDeviceFeatures deviceFeatures = {}; // defaults most of the features to false for now
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = (uint32_t)SizeOf(desiredDeviceExtensions);
    createInfo.ppEnabledExtensionNames = (const char**)desiredDeviceExtensions;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = (uint32_t)SizeOf(desiredValidationLayers);
        createInfo.ppEnabledLayerNames = (const char**)desiredValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create logical device!");
        abort();
    }

    if (IsQueueFamilyIndicesValid(indices)) {
        vkGetDeviceQueue(*device, indices.graphicsFamily, 0, graphicsQueue); // The graphics queue is already created if we have successfully created
                                                                             // a logical device. This is only to retrieve the handle
        vkGetDeviceQueue(*device, indices.presentFamily, 0, presentQueue); // The graphics queue is already created if we have successfully created a
                                                                           // logical device. This is only to retrieve the handle
    }


}

void createSurface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR* surface) {


    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create window surface!");
        abort();
    }

}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint32_t formatCount) {

    for(int i = 0; i < formatCount ; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            /* LOG_MSG(stderr, "swapchain format used: VK_FORMAT_B8G8R8A8_SRGB\n"); */
            /* LOG_MSG(stderr, "swapchain colorspace used: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n"); */
            return availableFormats[i];
        }
    }
    return availableFormats[0]; // pick the first available format if we do not find the format desired
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint32_t presentModeCount) {
    for(int i = 0; i < presentModeCount; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            /* LOG_MSG(stderr, "swapchain present mode used: VK_PRESENT_MODE_MAILBOX_KHR\n"); */
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        /* LOG_MSG(stderr, "swapchain extent mode used: {%d,%d}\n", capabilities.currentExtent.width, capabilities.currentExtent.height); */
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {(uint32_t)width, (uint32_t)height};

        actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void createSCImageViews(NanoRenderer* nanoRenderer, SwapchainContext* swapchainContext) {

    swapchainContext->imageViews = (VkImageView*)calloc(swapchainContext->info.imageCount, sizeof(VkImageView));
    for (size_t i = 0; i < swapchainContext->info.imageCount ; i++){
        swapchainContext->imageViews[i] = CreateImageView(nanoRenderer, swapchainContext->images[i], swapchainContext->info.selectedFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }


}

void createFramebuffer(VkDevice device, VkRenderPass renderpass, SwapchainContext* swapchainContext){

    swapchainContext->framebuffers = (VkFramebuffer*)calloc(swapchainContext->info.imageCount, sizeof(VkFramebuffer));

    for (size_t i = 0; i < swapchainContext->info.imageCount; i++) {
        VkImageView attachments[] = {swapchainContext->imageViews[i], swapchainContext->depthImage.imageView};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderpass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainContext->info.currentExtent.width;
        framebufferInfo.height = swapchainContext->info.currentExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainContext->framebuffers[i]) != VK_SUCCESS) {
            LOG_MSG(stderr, "failed to create framebuffer!");
            abort();
        }
    }

}

void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window, VkSurfaceKHR surface, SwapchainContext* swapchainContext) {

    swapchainContext->info = querySwapChainSupport(physicalDevice, surface);

    swapchainContext->info.selectedFormat = chooseSwapSurfaceFormat(swapchainContext->info.formats, swapchainContext->info.formats_size);
    swapchainContext->info.selectedPresentMode = chooseSwapPresentMode(swapchainContext->info.presentModes, swapchainContext->info.presentModes_size);
    swapchainContext->info.currentExtent = chooseSwapExtent(window, swapchainContext->info.capabilities);

    swapchainContext->info.imageCount = swapchainContext->info.capabilities.minImageCount + 1;
    if (swapchainContext->info.capabilities.maxImageCount > 0 && swapchainContext->info.imageCount > swapchainContext->info.capabilities.maxImageCount) {
        swapchainContext->info.imageCount = swapchainContext->info.capabilities.maxImageCount;
    }

    /* LOG_MSG(stderr, "Number of images used by the swapchain: %d\n", swapchainContext->info.imageCount); */

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = swapchainContext->info.imageCount;
    createInfo.imageFormat = swapchainContext->info.selectedFormat.format;
    createInfo.imageColorSpace = swapchainContext->info.selectedFormat.colorSpace;
    createInfo.imageExtent = swapchainContext->info.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = {};
    findQueueFamilies(physicalDevice, surface, &indices);
    uint32_t queueFamilyIndices[2] = {};
    if (IsQueueFamilyIndicesValid(indices)) {
        queueFamilyIndices[0] = indices.graphicsFamily;
        queueFamilyIndices[1] = indices.presentFamily;
    } else {
        LOG_MSG(stderr, "Queue Family indices is invalid. called in : createSwapchain(...)");
    }

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        LOG_MSG(stderr, "Graphics queue family is not the same as Present queue family. Swapchain images have to be concurent");
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapchainContext->info.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // TODO: If transparency is to be enabled, change this
    // if (createInfo.compositeAlpha != VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    //     LOG_MSG(stderr, "swapChain images not set to OPAQUE_BIT_KHR. Transparent window might be enabled");
    // }
    createInfo.presentMode = swapchainContext->info.selectedPresentMode;
    createInfo.clipped = VK_TRUE; // This deals with obstructed pixels when, for example, another window is on top.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchainContext->swapchain) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapchainContext->swapchain, &swapchainContext->info.imageCount, nullptr);
    swapchainContext->images = (VkImage*)calloc(swapchainContext->info.imageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchainContext->swapchain, &swapchainContext->info.imageCount, swapchainContext->images);


}

VkFormat findSupportedFormat(NanoRenderer* nanoRenderer, VkFormat candidates[], int numFormatCandidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    VkFormat format = {};
    for (int i = 0; i < numFormatCandidates; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(nanoRenderer->m_pNanoContext->physicalDevice, candidates[i], &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return candidates[i];
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return candidates[i];
        }
    }
    LOG_MSG(stderr, "failed to find suitable Vkformat");
    return format;
}

VkFormat findDepthFormat(NanoRenderer* nanoRenderer) {
    VkFormat formatCandidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(
        nanoRenderer,
        formatCandidates,
        3,
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void createDepthResources(NanoRenderer* nanoRenderer, SwapchainContext* swapchainContext) {
    VkFormat depthFormat = findDepthFormat(nanoRenderer);
    NanoImage* depthImgPtr = &nanoRenderer->m_pNanoContext->swapchainContext.depthImage;
    depthImgPtr->width = swapchainContext->info.currentExtent.width;
    depthImgPtr->height = swapchainContext->info.currentExtent.height;
    depthImgPtr->nanoVkBuffer = CreateImageBuffer(nanoRenderer, depthImgPtr->width, depthImgPtr->width, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    depthImgPtr->imageView = CreateImageView(nanoRenderer, depthImgPtr->nanoVkBuffer.textureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    depthImgPtr->isInitialized = false;
    depthImgPtr->isSubmittedToGPUMemory = false;

    TransitionImageLayout(nanoRenderer, depthImgPtr->nanoVkBuffer.textureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void recreateSwapchain(NanoRenderer* nanoRenderer, GLFWwindow* window){

    VkDevice device = nanoRenderer->m_pNanoContext->device;
    VkPhysicalDevice physicalDevice = nanoRenderer->m_pNanoContext->physicalDevice;
    SwapchainContext* swapChainContext = &nanoRenderer->m_pNanoContext->swapchainContext;
    VkSurfaceKHR surface = nanoRenderer->m_pNanoContext->surface;
    VkRenderPass renderpass = nanoRenderer->m_pNanoContext->defaultRenderpass;

    // handle minimization. pause rendering if that is the case
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChainContext(device, swapChainContext);

    // we query the new extent when re-creating the swapchain
    createSwapchain(physicalDevice,
                          device,
                          window,
                          surface,
                          swapChainContext);

    createSCImageViews(nanoRenderer,
                             swapChainContext);

    createDepthResources(nanoRenderer, swapChainContext);

    createFramebuffer(device,
                            renderpass,
                            swapChainContext);

    // update the extent of all graphics pipeines to reflect the framebuffer change due to window resize
    s_sceneToRender->graphicsPipeline.m_extent = swapChainContext->info.currentExtent;


}

// renderpass with at least one color attachment
void createRenderPass(VkDevice device, const SwapchainDetails swapchainDetails, VkRenderPass* renderpass){


    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainDetails.selectedFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat(s_nanoRenderer);//TODO: could do better by avoiding call to global function
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//TODO: if we want to save the depth image, we need to change this
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // The following other types of attachments can be referenced by a subpass:
    // pInputAttachments: Attachments that are read from a shader
    // pResolveAttachments: Attachments used for multisampling color attachments
    // pDepthStencilAttachment: Attachment for depth and stencil data
    // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // always add the reference to the attachment and never the attachment itself
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!

    // subpass synchronization with swapchain images state
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; //The dstSubpass is our one and only subpass (for now). it must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, renderpass) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create render pass!");
        abort();
    }


}

void createCommandPool(VkDevice device, const QueueFamilyIndices queueFamilyIndices, VkCommandPool* commandPool){


    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ASSERT(queueFamilyIndices.graphicsFamily != -1, "Graphics family indices is not defined - createCommandPool(...)");
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to create command pool!");
        abort();
    }


}

void createCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffer, uint32_t size){


    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = size;

    for(int i = 0; i < size ; i++){
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer) != VK_SUCCESS) {
            LOG_MSG(stderr, "failed to allocate command buffers!");
            abort();
        }
    }


}

void recordCommandBuffer(const NanoGraphicsPipeline* graphicsPipeline, VkFramebuffer* swapchainFrameBufferToWriteTo, VkCommandBuffer* commandBuffer, uint32_t currentFrame) {


    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(*commandBuffer, &beginInfo) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to begin recording command buffer!");
        abort();
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = graphicsPipeline->_renderpass;
    renderPassInfo.framebuffer = *swapchainFrameBufferToWriteTo;

    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = graphicsPipeline->m_extent;

    VkClearValue clearColor[2] = {{.color = {0.02f, 0.02f, 0.02f, 1.0f}},
                                  {.depthStencil = {1.0f, 0.0f}}};

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

        vkCmdBeginRenderPass(*commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->m_pipeline);

        // need to manually set the viewport and scissor here because we defined them as dynamic.
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = (float)graphicsPipeline->m_extent.height;
        viewport.width = (float)graphicsPipeline->m_extent.width;
        viewport.height = -(float)graphicsPipeline->m_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = graphicsPipeline->m_extent;
        vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

        if(s_meshMemoryPtr->isInitialized &&
            s_meshMemoryPtr->meshVKMemory.vertexMemory.buffer != VK_NULL_HANDLE &&
            s_meshMemoryPtr->meshVKMemory.indexMemory.buffer != VK_NULL_HANDLE){

            VkBuffer vertexBuffers[] = {s_meshMemoryPtr->meshVKMemory.vertexMemory.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(*commandBuffer, s_meshMemoryPtr->meshVKMemory.indexMemory.buffer, 0, VK_INDEX_TYPE_UINT32);

            for(int i = 0; i < s_sceneToRender->numRenderableObjects ; i++){
                RenderableObject* obj = s_sceneToRender->renderableObjects[i];
                if(!obj->isVisible)
                    continue;

                MeshObjectPushConstant objectPushConstant;
                objectPushConstant.albedoTextureID = obj->albedoTexture.isSubmittedToGPUMemory ? obj->albedoTexture.imageDescriptorID : -1;
                objectPushConstant.normalTextureID = obj->normalTexture.isSubmittedToGPUMemory ? obj->normalTexture.imageDescriptorID : -1;
                objectPushConstant.additionalTextureID = obj->additionalTexture1.isSubmittedToGPUMemory ? obj->additionalTexture1.imageDescriptorID : -1;
                objectPushConstant.additionalTextureID2 = obj->additionalTexture2.isSubmittedToGPUMemory ? obj->additionalTexture2.imageDescriptorID : -1;

                // One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
                uint32_t dynamicOffset = i * graphicsPipeline->uniformBufferDynamicAllignment;
                // Bind the descriptor set for rendering a mesh using the dynamic offset
                vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->m_pipelineLayout, 0, 1, &graphicsPipeline->DescSets[currentFrame], 1, &dynamicOffset);

                //upload the matrix to the GPU via push constants
                vkCmdPushConstants(*commandBuffer, graphicsPipeline->m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshObjectPushConstant), &objectPushConstant);

                int vertOffset = (int)(obj->meshObject.vertexMemStart - s_meshMemoryPtr->meshHostMemory.vertexMemory);
                uint32_t indOffset = (obj->meshObject.indexMemStart - s_meshMemoryPtr->meshHostMemory.indexMemory);
                uint32_t indSize = (obj->meshObject.indexMemSize / sizeof(uint32_t));
                vkCmdDrawIndexed(*commandBuffer, indSize, 1, indOffset, vertOffset, 0);
            }
        }

        vkCmdEndRenderPass(*commandBuffer);

    if (vkEndCommandBuffer(*commandBuffer) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to record command buffer!");
        abort();
    }


}

void createSwapchainSyncObjects(VkDevice device ,SwapchainSyncObjects* syncObjects, uint32_t size){


    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // we create it at a signaled state so we don't bloack at the very first frame.

    for(int i = 0; i < size ; i++){
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].renderFinishedSemaphore) != VK_SUCCESS) {
            LOG_MSG(stderr, "failed to create semaphores!");
        }

        if (vkCreateFence(device, &fenceInfo, nullptr, &syncObjects[i].inFlightFence)) {
            LOG_MSG(stderr, "failed to create frences!");
        }
    }


}

void PreDrawFrame(NanoRenderer* renderer, NanoWindow* window) {

    //CreateImageData(renderer, &texture);
    double newTime = glfwGetTime();
    renderer->m_pNanoContext->m_frameData.deltaTime = newTime - renderer->m_pNanoContext->m_frameData.time;
    renderer->m_pNanoContext->m_frameData.time = newTime;
    renderer->m_pNanoContext->m_frameData.currentFrame = renderer->m_pNanoContext->swapchainContext.currentFrame;

    UpdateScene(s_sceneToRender, &renderer->m_pNanoContext->m_frameData);

}

void DrawFrame(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow){


    uint32_t currentFrame = nanoRenderer->m_pNanoContext->swapchainContext.currentFrame;
    vkWaitForFences(nanoRenderer->m_pNanoContext->device, 1, &nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(nanoRenderer->m_pNanoContext->device, nanoRenderer->m_pNanoContext->swapchainContext.swapchain,
                                            UINT64_MAX, nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


    if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
        recreateSwapchain(nanoRenderer, nanoWindow->_window);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_MSG(stderr, "failed to acquire swap chain image!\n");
        abort();
    }

    // only reset fence if we know we will be performing work ( ie swapchain is optimal and we can write to it )
    vkResetFences(nanoRenderer->m_pNanoContext->device, 1, &nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence);

    vkResetCommandBuffer(nanoRenderer->m_pNanoContext->swapchainContext.commandBuffer[currentFrame], 0);

    if(s_sceneToRender){
        UpdateGraphicsPipelineAtFrame(nanoRenderer, &s_sceneToRender->graphicsPipeline, currentFrame);
        recordCommandBuffer(&s_sceneToRender->graphicsPipeline,
                            &nanoRenderer->m_pNanoContext->swapchainContext.framebuffers[imageIndex], //swapchain framebuffer for the command buffer to operate on
                            &nanoRenderer->m_pNanoContext->swapchainContext.commandBuffer[currentFrame], //command buffer to write to.
                            currentFrame);
    } else {
        LOG_MSG(stderr, "No scene to render");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].renderFinishedSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &nanoRenderer->m_pNanoContext->swapchainContext.commandBuffer[currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    if (vkQueueSubmit(nanoRenderer->m_pNanoContext->graphicsQueue, 1, &submitInfo, nanoRenderer->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence) != VK_SUCCESS) {
        LOG_MSG(stderr, "failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = {nanoRenderer->m_pNanoContext->swapchainContext.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(nanoRenderer->m_pNanoContext->presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || nanoWindow->framebufferResized) {
        nanoWindow->framebufferResized = false;
        /* LOG_MSG(stderr, "recreating swapchain\n"); */
        recreateSwapchain(nanoRenderer, nanoWindow->_window);
        /* return OK; */
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_MSG(stderr, "failed to present swap chain image!\n");
        abort();
    }

    nanoRenderer->m_pNanoContext->swapchainContext.currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    nanoRenderer->m_pNanoContext->isInitialized = true;

}

void InitRenderer(NanoRenderer* nanoRenderer, MeshMemory* meshMemory, ImageMemory* imageMemory, NanoWindow* window){


    if(!meshMemory->meshHostMemory.isInitialized){
       LOG_MSG(stderr, "Mesh Host Memory is not yet initialized. Renderer needs an initialized Mesh Host Memory\n");
       DEBUG_BREAK;
       abort();
    }

    s_meshMemoryPtr = meshMemory;
    s_imageMemoryPtr = imageMemory;
    s_nanoRenderer = nanoRenderer;
    s_sceneToRender = nullptr;

    nanoRenderer->m_pNanoContext = (NanoVKContext*)calloc(1, sizeof(NanoVKContext));
    // Here the err validation is not that useful
    // a iferr_return can be added at the end of each statement to check it's state and exit (or at least warn) if an error did occur
    createInstance(APP_NAME,
                   ENGINE_NAME,
                   &nanoRenderer->m_pNanoContext->instance); // APP_NAME and ENGINE_NAME is defined in NanoConfig

    setupDebugMessenger(nanoRenderer->m_pNanoContext->instance,
                        &gDebugMessenger); // this depends on whether we are running in debug or not

    createSurface(nanoRenderer->m_pNanoContext->instance,
                  window->_window,
                  &nanoRenderer->m_pNanoContext->surface);

    pickPhysicalDevice(nanoRenderer->m_pNanoContext->instance,
                       nanoRenderer->m_pNanoContext->surface,
                       &nanoRenderer->m_pNanoContext->queueIndices,
                       &nanoRenderer->m_pNanoContext->physicalDevice, // physical device is not created but picked based on scores dictated by the number of supported features
                       &nanoRenderer->m_pNanoContext->deviceProperties);

    createLogicalDeviceAndGetQueues(nanoRenderer->m_pNanoContext->physicalDevice,
                                    nanoRenderer->m_pNanoContext->surface,
                                    nanoRenderer->m_pNanoContext->queueIndices,
                                    &nanoRenderer->m_pNanoContext->graphicsQueue,
                                    &nanoRenderer->m_pNanoContext->presentQueue,
                                    &nanoRenderer->m_pNanoContext->device); // Logical device *is* created and therefore has to be destroyed

    createCommandPool(nanoRenderer->m_pNanoContext->device,
                      nanoRenderer->m_pNanoContext->queueIndices,
                      &nanoRenderer->m_pNanoContext->commandPool);

    createSwapchain(nanoRenderer->m_pNanoContext->physicalDevice,
                    nanoRenderer->m_pNanoContext->device,
                    window->_window,
                    nanoRenderer->m_pNanoContext->surface,
                    &nanoRenderer->m_pNanoContext->swapchainContext);

    createSCImageViews(nanoRenderer,
                       &nanoRenderer->m_pNanoContext->swapchainContext);

    createDepthResources(nanoRenderer,
                         &nanoRenderer->m_pNanoContext->swapchainContext);

    createRenderPass(nanoRenderer->m_pNanoContext->device,
                     nanoRenderer->m_pNanoContext->swapchainContext.info,
                     &nanoRenderer->m_pNanoContext->defaultRenderpass);

    createFramebuffer(nanoRenderer->m_pNanoContext->device,
                      nanoRenderer->m_pNanoContext->defaultRenderpass,
                      &nanoRenderer->m_pNanoContext->swapchainContext);

    createCommandBuffers(nanoRenderer->m_pNanoContext->device,
                        nanoRenderer->m_pNanoContext->commandPool,
                        nanoRenderer->m_pNanoContext->swapchainContext.commandBuffer,
                        MAX_FRAMES_IN_FLIGHT);

    createSwapchainSyncObjects(nanoRenderer->m_pNanoContext->device,
                               nanoRenderer->m_pNanoContext->swapchainContext.syncObjects,
                               MAX_FRAMES_IN_FLIGHT);


    //testToDisplay = {};
    testToDisplay = CreateString("");
    /* InitImage(&s_imageMemoryPtr->imageHostMemory, 256, 256, IMAGE_FORMAT_RGBA, &texture); */
    /* InitText(nanoRenderer, &texture, "                    "); */


}

void RenderScene(RenderableScene* scene){
    s_sceneToRender = scene;
}

