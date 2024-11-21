#include "NanoGraphics.h"
#include "NanoConfig.h"
#include "NanoError.h"
#include "NanoUtility.h"
#include "NanoWindow.h"
#include "NanoShader.h"
#include "NanoGraphicsPipeline.h"
#include "NanoBuffers.h"

#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

Mesh object;

void CreateVertexData(NanoGraphics *nanoGraphics, Mesh* meshObject){
    int numVertices = 4;
    int numIndices = 6;
    meshObject->pVertexData = (Vertex*)malloc(sizeof(Vertex) * numVertices);
    meshObject->vertexDataSize = sizeof(Vertex) * numVertices;
    meshObject->pIndexData = (uint32_t*)malloc(sizeof(uint32_t) * numIndices);
    meshObject->indexDataSize = sizeof(uint32_t) * numIndices;

    Vertex vertices[4] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                          {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                          {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                          {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
    memcpy(meshObject->pVertexData, vertices, meshObject->vertexDataSize);
    memcpy(meshObject->pIndexData, indices, meshObject->indexDataSize);

    meshObject->vertexMemory = CreateVertexBuffer(nanoGraphics, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, meshObject->pVertexData, meshObject->vertexDataSize);
    meshObject->indexMemory = CreateIndexBuffer(nanoGraphics, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, meshObject->pIndexData, meshObject->indexDataSize);
}

void CleanUpVertexData(NanoGraphics *nanoGraphics){
    CleanUpBuffer(nanoGraphics, &object.vertexMemory);
    CleanUpBuffer(nanoGraphics, &object.indexMemory);
}

bool IsQueueFamilyIndicesValid(QueueFamilyIndices queueFamily) { // helper function to validate queue indices
    return queueFamily.graphicsFamily != -1 && queueFamily.presentFamily != -1;
}


void AddGraphicsPipelineToNanoContext(NanoGraphics* nanoGraphics, const NanoGraphicsPipeline* graphicsPipeline){
    if(!graphicsPipeline){
        return;
    }

    uint32_t indx = nanoGraphics->m_pNanoContext->currentGraphicsPipeline;
    if(nanoGraphics->m_pNanoContext->graphicPipelinesCount != 0){
        indx++;
    }

    nanoGraphics->m_pNanoContext->graphicsPipelines[indx] = *graphicsPipeline;
    nanoGraphics->m_pNanoContext->currentGraphicsPipeline = indx;
    nanoGraphics->m_pNanoContext->graphicPipelinesCount++;
}

VkDebugUtilsMessengerEXT gDebugMessenger;

// We have to look up the address of the debug callback create function ourselves using vkGetInstanceProcAddr
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// We have to look up the address of the debug callback destroy function ourselves using vkGetInstanceProcAddr
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    char message[1024];
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

    fprintf(stderr, "%s", pCallbackData->pMessage);
    fprintf(stderr, "Objects involved: \n");

    for (int i = 0; i < pCallbackData->objectCount; i++) {
        fprintf(stderr, "\t%s\n", pCallbackData->pObjects[i].pObjectName);
    }

    fprintf(stderr, "\n");

    if (messageLevel == NANO_FATAL){
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
    createInfo->pUserData = NULL;
    createInfo->pNext = NULL;
}

void cleanupSwapChainContext(const VkDevice device, SwapchainContext* swapchainContext) {
    for (int i = 0 ; i < swapchainContext->info.imageCount; i++) {
        vkDestroyFramebuffer(device, swapchainContext->framebuffers[i], NULL);
        vkDestroyImageView(device, swapchainContext->imageViews[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchainContext->swapchain, NULL);
}

ERR CleanUpGraphics(NanoGraphics* nanoGraphics){
    ERR err = OK;

    // wait in case there are any pending tasks
    vkDeviceWaitIdle(nanoGraphics->m_pNanoContext->device);
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[i].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[i].renderFinishedSemaphore, NULL);
        vkDestroyFence(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[i].inFlightFence, NULL);
    }

    CleanUpVertexData(nanoGraphics);

    // clean commandPool and incidently the commandbuffers acquired from them
    vkDestroyCommandPool(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->commandPool, NULL);

    // clean swapchain framebuffers
    for (int i = 0 ; i < nanoGraphics->m_pNanoContext->swapchainContext.info.imageCount; i++) {
        vkDestroyFramebuffer(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.framebuffers[i], NULL);
    }
    // clean swapchain imageviews
    for (int i = 0 ; i < nanoGraphics->m_pNanoContext->swapchainContext.info.imageCount; i++) {
        vkDestroyImageView(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.imageViews[i], NULL);
    }

    // clean graphic pipelines
    for (int i = 0 ; i < nanoGraphics->m_pNanoContext->graphicPipelinesCount; i++) {
        CleanUpGraphicsPipeline(nanoGraphics, &nanoGraphics->m_pNanoContext->graphicsPipelines[i]);
    }

    // clean renderpass
    vkDestroyRenderPass(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->renderpass, NULL);

    vkDestroySwapchainKHR(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.swapchain, NULL);

    vkDestroySurfaceKHR(nanoGraphics->m_pNanoContext->instance, nanoGraphics->m_pNanoContext->surface, NULL);

    vkDestroyDevice(nanoGraphics->m_pNanoContext->device, NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(nanoGraphics->m_pNanoContext->instance, gDebugMessenger, NULL);
    }

    vkDestroyInstance(nanoGraphics->m_pNanoContext->instance, NULL);

    return err;
}

static bool checkValidationLayerSupport(const char* validationLayers[]) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

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
    SwapchainDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

    if (formatCount != 0) {
        details.formats = (VkSurfaceFormatKHR*)calloc(formatCount, sizeof(VkSurfaceFormatKHR));
        details.formats_size = formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

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
    vkEnumerateInstanceExtensionProperties(NULL, extensionCount, NULL);

    extensions = (VkExtensionProperties*)calloc(*extensionCount, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(nullptr, extensionCount, extensions);
}

static ERR createInstance(const char *applicationName, const char *engineName, VkInstance* instance) {
    ERR err = OK;

    if (enableValidationLayers && !checkValidationLayerSupport(desiredValidationLayers)) {
        fprintf(stderr, "Number of Desired Layers %zu\n", SizeOf(desiredValidationLayers));
        fprintf(stderr, "validation layers requested, but not available!");
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

    //std::vector<VkExtensionProperties> test = {};
    //getSupportedInstanceExtensions(test);

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
        return UNDEFINED;
    }
    return err;
}

static ERR setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    ERR err = OK;

    if (!enableValidationLayers) {
        return OK;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populateDebugMessengerCreateInfo(&createInfo);

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
            err = OK;
            return err;
        }
    }

    return err;
}

ERR checkDeviceExtensionSupport(VkPhysicalDevice device) {
    ERR err = OK;
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

int rateDeviceSuitability(const VkPhysicalDevice device, const VkSurfaceKHR surface, QueueFamilyIndices* queueIndices) {
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

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        fprintf(stderr, "No Geometry shader support found\n");
    }

    if (!deviceFeatures.tessellationShader) {
        fprintf(stderr, "No Tesselation shader support found\n");
    }

    // Application can't function without the required device extensions
    bool extensionsSupported = OK == checkDeviceExtensionSupport(device);
    if (!extensionsSupported) {
        fprintf(stderr, "missing extensions. score = 0\n");
        return 0;
    }

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainDetails swapchainSupport = querySwapChainSupport(device, surface);
        swapchainAdequate = swapchainSupport.formats_size && swapchainSupport.presentModes_size;
        if (!swapchainAdequate) {
            fprintf(stderr, "swapchain not adequate. score = 0\n");
            return 0; // Application can't function without an adequate swapchain extensions
        }
    }

    if (NOT_FOUND == findQueueFamilies(device, surface, queueIndices)) {
        fprintf(stderr, "queue family not found. score = 0\n");
        return 0;
    }

    return score;
}

static ERR pickPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface, QueueFamilyIndices* queueIndices,
                              VkPhysicalDevice* physicalDevice) {
    ERR err = OK;
    *physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        fprintf(stderr, "failed to find GPUs with Vulkan support!\n");
        abort();
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // Use an ordered map to automatically sort candidates by increasing score
    VkPhysicalDevice candidates[deviceCount];

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
        err = NOT_FOUND;
        fprintf(stderr, "failed to find a suitable GPU!\n");
        abort();
    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(*physicalDevice, &deviceProperties);

        char deviceType[256] = {0};
        switch (deviceProperties.deviceType) {
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
        fprintf(stderr, "Physical device selected: %s [%s]\n", deviceProperties.deviceName, deviceType);
    }

    return OK; // this is never reached if we use try/catch.
}

ERR createLogicalDeviceAndGetQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                        QueueFamilyIndices indices,
                        VkQueue* graphicsQueue, VkQueue *presentQueue,
                        VkDevice* device) {
    ERR err = OK;

    if (!IsQueueFamilyIndicesValid(indices) && NOT_FOUND == findQueueFamilies(physicalDevice, surface, &indices)) {
        fprintf(stderr, "failed to find a graphics familiy queue!");
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

    VkPhysicalDeviceFeatures deviceFeatures = {}; // defaults all the features to false for now
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
        fprintf(stderr, "failed to create logical device!");
        abort();
    }

    if (IsQueueFamilyIndicesValid(indices)) {
        vkGetDeviceQueue(*device, indices.graphicsFamily, 0, graphicsQueue); // The graphics queue is already created if we have successfully created
                                                                             // a logical device. This is only to retrieve the handle
        vkGetDeviceQueue(*device, indices.presentFamily, 0, presentQueue); // The graphics queue is already created if we have successfully created a
                                                                           // logical device. This is only to retrieve the handle
    }

    return err;
}

ERR createSurface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR* surface) {
    ERR err = OK;

    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        err = INVALID;
        fprintf(stderr, "failed to create window surface!");
        abort();
    }
    return err;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, uint32_t formatCount) {

    for(int i = 0; i < formatCount ; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            fprintf(stderr, "swapchain format used: VK_FORMAT_B8G8R8A8_SRGB\n");
            fprintf(stderr, "swapchain colorspace used: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n");
            return availableFormats[i];
        }
    }
    return availableFormats[0]; // pick the first available format if we do not find the format desired
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR* availablePresentModes, uint32_t presentModeCount) {
    for(int i = 0; i < presentModeCount; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            fprintf(stderr, "swapchain present mode used: VK_PRESENT_MODE_MAILBOX_KHR\n");
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        fprintf(stderr, "swapchain extent mode used: {%d,%d}\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
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

ERR createSCImageViews(const VkDevice device, SwapchainContext* swapchainContext) {
    ERR err = OK;
    swapchainContext->imageViews = (VkImageView*)calloc(swapchainContext->info.imageCount, sizeof(VkImageView));
    for (size_t i = 0; i < swapchainContext->info.imageCount ; i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainContext->images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainContext->info.selectedFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainContext->imageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create image views!");
            abort();
        }
    }

    return err;
}

ERR createFramebuffer(const VkDevice device, const VkRenderPass renderpass, SwapchainContext* swapchainContext){
    ERR err = OK;
    swapchainContext->framebuffers = (VkFramebuffer*)calloc(swapchainContext->info.imageCount, sizeof(VkFramebuffer));

    for (size_t i = 0; i < swapchainContext->info.imageCount; i++) {
        VkImageView attachments[] = {swapchainContext->imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainContext->info.currentExtent.width;
        framebufferInfo.height = swapchainContext->info.currentExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainContext->framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create framebuffer!");
            abort();
        }
    }
    return err;
}

ERR createSwapchain(const VkPhysicalDevice physicalDevice, const VkDevice device, GLFWwindow *window, const VkSurfaceKHR surface, SwapchainContext* swapchainContext) {
    ERR err = OK;
    swapchainContext->info = querySwapChainSupport(physicalDevice, surface);

    swapchainContext->info.selectedFormat = chooseSwapSurfaceFormat(swapchainContext->info.formats, swapchainContext->info.formats_size);
    swapchainContext->info.selectedPresentMode = chooseSwapPresentMode(swapchainContext->info.presentModes, swapchainContext->info.presentModes_size);
    swapchainContext->info.currentExtent = chooseSwapExtent(window, swapchainContext->info.capabilities);

    swapchainContext->info.imageCount = swapchainContext->info.capabilities.minImageCount + 1;
    if (swapchainContext->info.capabilities.maxImageCount > 0 && swapchainContext->info.imageCount > swapchainContext->info.capabilities.maxImageCount) {
        swapchainContext->info.imageCount = swapchainContext->info.capabilities.maxImageCount;
    }

    fprintf(stderr, "Number of images used by the swapchain: %d\n", swapchainContext->info.imageCount);

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
    err = findQueueFamilies(physicalDevice, surface, &indices);
    uint32_t queueFamilyIndices[2] = {};
    if (IsQueueFamilyIndicesValid(indices)) {
        queueFamilyIndices[0] = indices.graphicsFamily;
        queueFamilyIndices[1] = indices.presentFamily;
    } else {
        fprintf(stderr, "Queue Family indices is invalid. called in : createSwapchain(...)");
    }

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        fprintf(stderr, "Graphics queue family is not the same as Present queue family. Swapchain images have to be concurent");
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapchainContext->info.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // TODO: If transparency is to be enabled, change this
    if (createInfo.compositeAlpha != VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        fprintf(stderr, "swapchain images not set to OPAQUE_BIT_KHR. Transparent window might be enabled");
    }
    createInfo.presentMode = swapchainContext->info.selectedPresentMode;
    createInfo.clipped = VK_TRUE; // This deals with obstructed pixels when, for example, another window is ontop.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchainContext->swapchain) != VK_SUCCESS) {
        fprintf(stderr, "failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapchainContext->swapchain, &swapchainContext->info.imageCount, nullptr);
    swapchainContext->images = (VkImage*)calloc(swapchainContext->info.imageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchainContext->swapchain, &swapchainContext->info.imageCount, swapchainContext->images);

    return err;
}


ERR recreateSwapchain(const VkPhysicalDevice physicalDevice, const VkDevice device, GLFWwindow *window, const VkSurfaceKHR surface, SwapchainContext* swapChainContext, const VkRenderPass renderpass){
    ERR err = OK;
    vkDeviceWaitIdle(device);

    err = createSwapchain(physicalDevice,
                          device,
                          window,
                          surface,
                          swapChainContext);

    err = createSCImageViews(device,
                             swapChainContext);

    err = createFramebuffer(device,
                            renderpass,
                            swapChainContext);

    return err;
}


ERR createGraphicsPipeline(NanoGraphics* nanoGraphics, NanoGraphicsPipeline* graphicsPipeline) {
    ERR err = OK;
    VkDevice device = nanoGraphics->m_pNanoContext->device;
    VkExtent2D extent = nanoGraphics->m_pNanoContext->swapchainContext.info.currentExtent;
    VkRenderPass renderpass = nanoGraphics->m_pNanoContext->renderpass;

    InitGraphicsPipeline(graphicsPipeline, device, extent);
    AddVertShaderToNGPipeline(nanoGraphics, graphicsPipeline, "./src/shader/shader.vert");
    AddFragShaderToNGPipeline(nanoGraphics, graphicsPipeline, "./src/shader/shader.frag");
    graphicsPipeline->_renderpass = renderpass;
    CompileNGPipeline(graphicsPipeline, true);


    return err;
}

// renderpass with at least one color attachment
ERR createRenderPass(VkDevice device, const SwapchainDetails swapchainDetails, VkRenderPass* renderpass){
    ERR err = OK;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainDetails.selectedFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // The following other types of attachments can be referenced by a subpass:
    // pInputAttachments: Attachments that are read from a shader
    // pResolveAttachments: Attachments used for multisampling color attachments
    // pDepthStencilAttachment: Attachment for depth and stencil data
    // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // always add the reference to the attachment and never the attachment itself
    // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!

    // subpass synchronization with swapchain images state
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; //The dstSubpass is our one and only subpass (for now). it must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, renderpass) != VK_SUCCESS) {
        fprintf(stderr, "failed to create render pass!");
        abort();
    }

    return err;
}

ERR createCommandPool(VkDevice device, const QueueFamilyIndices queueFamilyIndices, VkCommandPool* commandPool){
    ERR err = OK;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ASSERT(queueFamilyIndices.graphicsFamily != -1, "Graphics family indices is not defined - createCommandPool(...)");
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        fprintf(stderr, "failed to create command pool!");
        abort();
    }

    return err;
}

ERR createCommandBuffer(VkDevice device, const VkCommandPool commandPool, VkCommandBuffer* commandBuffer, uint32_t size){
    ERR err = OK;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = size;

    for(int i = 0; i < size ; i++){
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer) != VK_SUCCESS) {
            fprintf(stderr, "failed to allocate command buffers!");
            abort();
        }
    }

    return err;
}

ERR recordCommandBuffer(const NanoGraphicsPipeline* graphicsPipeline, VkFramebuffer* swapchainFrameBufferToWriteTo, VkCommandBuffer* commandBuffer) {
    ERR err = OK;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(*commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "failed to begin recording command buffer!");
        abort();
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = graphicsPipeline->_renderpass;
    renderPassInfo.framebuffer = *swapchainFrameBufferToWriteTo;

    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = graphicsPipeline->m_extent;

    VkClearValue clearColor = {{{0.02f, 0.02f, 0.02f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(*commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->m_pipeline);

        // need to manually set the viewport and scissor here because we defined them as dynamic.
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)graphicsPipeline->m_extent.width;
        viewport.height = (float)graphicsPipeline->m_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = graphicsPipeline->m_extent;
        vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {object.vertexMemory.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(*commandBuffer, object.indexMemory.buffer, 0, VK_INDEX_TYPE_UINT32);


        //vkCmdDraw(*commandBuffer, object.vertexDataSize, 1, 0, 0);
        vkCmdDrawIndexed(*commandBuffer, (uint32_t)object.indexDataSize/sizeof(uint32_t), 1, 0, 0, 0);

        vkCmdEndRenderPass(*commandBuffer);

    if (vkEndCommandBuffer(*commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "failed to record command buffer!");
        abort();
    }

    return err;
}

ERR createSwapchainSyncObjects(VkDevice device ,SwapchainSyncObjects* syncObjects, uint32_t size){
    ERR err = OK;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // we create it at a signaled state so we don't bloack at the very first frame.

    for(int i = 0; i < size ; i++){
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].renderFinishedSemaphore) != VK_SUCCESS) {
            fprintf(stderr, "failed to create semaphores!");
        }

        if (vkCreateFence(device, &fenceInfo, nullptr, &syncObjects[i].inFlightFence)) {
            fprintf(stderr, "failed to create frences!");
        }
    }

    return err;
}

ERR DrawFrame(NanoGraphics* nanoGraphics){
    ERR err = OK;

    int currentFrame = nanoGraphics->m_pNanoContext->swapchainContext.currentFrame;
    vkWaitForFences(nanoGraphics->m_pNanoContext->device, 1, &nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(nanoGraphics->m_pNanoContext->device, 1, &nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.swapchain,
                          UINT64_MAX, nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(nanoGraphics->m_pNanoContext->swapchainContext.commandBuffer[nanoGraphics->m_pNanoContext->swapchainContext.currentFrame], 0);

    int currentGraphicsPipelineIndex = nanoGraphics->m_pNanoContext->currentGraphicsPipeline;
    recordCommandBuffer(&nanoGraphics->m_pNanoContext->graphicsPipelines[currentGraphicsPipelineIndex],
                        &nanoGraphics->m_pNanoContext->swapchainContext.framebuffers[imageIndex], //swapchain framebuffer for the command buffer to operate on
                        &nanoGraphics->m_pNanoContext->swapchainContext.commandBuffer[nanoGraphics->m_pNanoContext->swapchainContext.currentFrame]); //command buffer to write to.

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[nanoGraphics->m_pNanoContext->swapchainContext.currentFrame].imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[nanoGraphics->m_pNanoContext->swapchainContext.currentFrame].renderFinishedSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &nanoGraphics->m_pNanoContext->swapchainContext.commandBuffer[nanoGraphics->m_pNanoContext->swapchainContext.currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    if (vkQueueSubmit(nanoGraphics->m_pNanoContext->graphicsQueue, 1, &submitInfo, nanoGraphics->m_pNanoContext->swapchainContext.syncObjects[currentFrame].inFlightFence) != VK_SUCCESS) {
        fprintf(stderr, "failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = {nanoGraphics->m_pNanoContext->swapchainContext.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(nanoGraphics->m_pNanoContext->presentQueue, &presentInfo);

    nanoGraphics->m_pNanoContext->swapchainContext.currentFrame = (nanoGraphics->m_pNanoContext->swapchainContext.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return err;
}

ERR InitGraphics(NanoGraphics* nanoGraphics, NanoWindow* window){
    ERR err = OK;
    nanoGraphics->m_pNanoContext = (NanoVKContext*)calloc(1, sizeof(NanoVKContext));
    // Here the err validation is not that useful
    // a iferr_return can be added at the end of each statement to check it's state and exit (or at least warn) if an error did occur
    createInstance(APP_NAME,
                         ENGINE_NAME,
                         &nanoGraphics->m_pNanoContext->instance); // APP_NAME and ENGINE_NAME is defined in NanoConfig

    setupDebugMessenger(nanoGraphics->m_pNanoContext->instance,
                              &gDebugMessenger); // this depends on whether we are running in debug or not

    createSurface(nanoGraphics->m_pNanoContext->instance,
                        window->_window,
                        &nanoGraphics->m_pNanoContext->surface);

    pickPhysicalDevice(nanoGraphics->m_pNanoContext->instance,
                             nanoGraphics->m_pNanoContext->surface,
                             &nanoGraphics->m_pNanoContext->queueIndices,
                             &nanoGraphics->m_pNanoContext->physicalDevice); // physical device is not created but picked based on scores dictated by the number of supported features

    createLogicalDeviceAndGetQueues(nanoGraphics->m_pNanoContext->physicalDevice,
                              nanoGraphics->m_pNanoContext->surface,
                              nanoGraphics->m_pNanoContext->queueIndices,
                              &nanoGraphics->m_pNanoContext->graphicsQueue,
                              &nanoGraphics->m_pNanoContext->presentQueue,
                              &nanoGraphics->m_pNanoContext->device); // Logical device *is* created and therefore has to be destroyed

    createSwapchain(nanoGraphics->m_pNanoContext->physicalDevice,
                          nanoGraphics->m_pNanoContext->device,
                          window->_window,
                          nanoGraphics->m_pNanoContext->surface,
                          &nanoGraphics->m_pNanoContext->swapchainContext);

    createSCImageViews(nanoGraphics->m_pNanoContext->device,
                             &nanoGraphics->m_pNanoContext->swapchainContext);

    createRenderPass(nanoGraphics->m_pNanoContext->device,
                           nanoGraphics->m_pNanoContext->swapchainContext.info,
                           &nanoGraphics->m_pNanoContext->renderpass);

    NanoGraphicsPipeline* graphicsPipeline = (NanoGraphicsPipeline*)calloc(1, sizeof(NanoGraphicsPipeline));
    createGraphicsPipeline(nanoGraphics,
                                 graphicsPipeline);

    AddGraphicsPipelineToNanoContext(nanoGraphics, graphicsPipeline);

    createFramebuffer(nanoGraphics->m_pNanoContext->device,
                            nanoGraphics->m_pNanoContext->renderpass,
                            &nanoGraphics->m_pNanoContext->swapchainContext);

    createCommandPool(nanoGraphics->m_pNanoContext->device,
                            nanoGraphics->m_pNanoContext->queueIndices,
                            &nanoGraphics->m_pNanoContext->commandPool);

    createCommandBuffer(nanoGraphics->m_pNanoContext->device,
                              nanoGraphics->m_pNanoContext->commandPool,
                              nanoGraphics->m_pNanoContext->swapchainContext.commandBuffer,
                              MAX_FRAMES_IN_FLIGHT);

    createSwapchainSyncObjects(nanoGraphics->m_pNanoContext->device,
                                     nanoGraphics->m_pNanoContext->swapchainContext.syncObjects,
                                     MAX_FRAMES_IN_FLIGHT);

    CreateVertexData(nanoGraphics, &object);

    return err;
}

