#include "NanoGraphics.h"
#include "NanoConfig.h"
#include "NanoError.h"
#include "NanoUtility.h"
#include "NanoWindow.h"
#include "NanoShader.h"
#include "NanoGraphicsPipeline.h"

#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define _CRT_SECURE_NO_WARNINGS

_Bool IsQueueFamilyIndicesValid(QueueFamilyIndices queueFamily) { // helper function to validate queue indices
    return queueFamily.graphicsFamily != -1 && queueFamily.presentFamily != -1;
}


void AddGraphicsPipeline(NanoGraphics* nanoGraphics, const NanoGraphicsPipeline* graphicsPipeline){
    int indx = nanoGraphics->m_pNanoContext->currentGraphicsPipeline;
    nanoGraphics->m_pNanoContext->graphicsPipelines[indx + 1] = *graphicsPipeline;
}

VkDebugUtilsMessengerEXT debugMessenger;

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
        CleanUpGraphicsPipeline(&nanoGraphics->m_pNanoContext->graphicsPipelines[i]);
    }

    // clean renderpass
    vkDestroyRenderPass(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->renderpass, NULL);

    vkDestroySwapchainKHR(nanoGraphics->m_pNanoContext->device, nanoGraphics->m_pNanoContext->swapchainContext.swapchain, NULL);

    vkDestroySurfaceKHR(nanoGraphics->m_pNanoContext->instance, nanoGraphics->m_pNanoContext->surface, NULL);

    vkDestroyDevice(nanoGraphics->m_pNanoContext->device, NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(nanoGraphics->m_pNanoContext->instance, debugMessenger, NULL);
    }

    vkDestroyInstance(nanoGraphics->m_pNanoContext->instance, NULL);

    return err;
}

static bool checkValidationLayerSupport(const char *const *validationLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    int indx = 0;
    while (validationLayers[indx]) {
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

SwapchainDetails querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
    SwapchainDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

    if (formatCount != 0) {
        details.formats = (VkSurfaceFormatKHR*)malloc(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

    if (presentModeCount != 0) {
        details.presentModes = (VkPresentModeKHR*)malloc(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);
    }

    return details;
}

void getRequiredInstanceExtensions(char (*requiredInstanceExtensions)[128], uint32_t* numOfInstanceExtensions){
    const char** glfwExtensions;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, NULL, NULL);

    glfwExtensions = glfwGetRequiredInstanceExtensions(numOfInstanceExtensions);
    /* printf("num of instance extensions: %d", *numOfInstanceExtensions); */

    // additional instance extension we may want to add
    // 64 additional extensions that we can potentially add
    int extIdx = 0;
    while (desiredInstanceExtensions[extIdx]){
        strcpy(requiredInstanceExtensions[extIdx], desiredInstanceExtensions[extIdx]);
        extIdx++;
    }

    for (uint32_t i = 0; i < *numOfInstanceExtensions; i++) {
        strcpy(requiredInstanceExtensions[extIdx], glfwExtensions[i]);
        extIdx++;
    }

    if (enableValidationLayers) {
        strcpy(requiredInstanceExtensions[extIdx], VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extIdx++;
    }

    *numOfInstanceExtensions = extIdx;

    // ----------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
}

static void getSupportedInstanceExtensions(VkExtensionProperties* extensions, uint32_t* extensionCount) {
    *extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, extensionCount, NULL);

    extensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

#ifdef _DEBUG
    std::cout << "available extensions:\n";
    for (const auto &extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
    std::cout.flush();
#endif

    return extensions;
}

static ERR createInstance(const char *applicationName, const char *engineName, VkInstance &instance) {
    ERR err = ERR::OK;

    if (Config::enableValidationLayers && !checkValidationLayerSupport(Config::desiredValidationLayers)) {
        LOG_MSG(ERRLevel::FATAL, "Number of Desired Layers %zu\n", SizeOf(Config::desiredValidationLayers));
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = engineName;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#if __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // std::vector<VkExtensionProperties> test = {};
    // _getSupportedInstanceExtensions(test);

    std::vector<const char *> instanceExtensions = getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (Config::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(SizeOf(Config::desiredValidationLayers));
        createInfo.ppEnabledLayerNames = Config::desiredValidationLayers;

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        return ERR::UNDEFINED;
    }
    return err;
}

static ERR setupDebugMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessenger) {
    ERR err = ERR::OK;

    if (!Config::enableValidationLayers) {
        return ERR::OK;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_THROW(CreateDebugUtilsMessengerEXT(_NanoContext.instance, &createInfo, nullptr, &debugMessenger), "Failed to create Debug Messenger\n");
    return ERR::OK; // this is never reached if we use try/catch.
}

ERR findQueueFamilies(VkPhysicalDevice device, QueueFamilyIndices &indices) {
    ERR err = ERR::NOT_FOUND;
    // Logic to find queue family indices to populate struct with

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (int i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _NanoContext.surface, &presentSupport);
            if (presentSupport)
                indices.presentFamily = i;
        }

        if (indices.IsValid()) {
            err = ERR::OK;
            return err;
        }
    }

    return err;
}

ERR checkDeviceExtensionSupport(VkPhysicalDevice device) {
    ERR err = ERR::OK;
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions{};
    int extIdx = 0;
    while (Config::desiredDeviceExtensions[extIdx]) {
        requiredExtensions.emplace(std::string(Config::desiredDeviceExtensions[extIdx]));
        extIdx++;
    }

    // make sure we go through all the list of desiredExtensions and "check off" all of them
    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    err = requiredExtensions.empty() ? ERR::OK : ERR::NOT_FOUND;
    return err;
}

int rateDeviceSuitability(const VkPhysicalDevice &device, const VkSurfaceKHR &surface, QueueFamilyIndices &queueIndices) {
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
    bool extensionsSupported = ERR::OK == checkDeviceExtensionSupport(device);
    if (!extensionsSupported) {
        fprintf(stderr, "missing extensions. score = 0\n");
        return 0;
    }

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainDetails swapchainSupport = querySwapChainSupport(device, surface);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
        if (!swapchainAdequate) {
            fprintf(stderr, "swapchain not adequate. score = 0\n");
            return 0; // Application can't function without an adequate swapchain extensions
        }
    }

    if (ERR::NOT_FOUND == findQueueFamilies(device, queueIndices)) {
        fprintf(stderr, "queue family not found. score = 0\n");
        return 0;
    }

    return score;
}

static ERR pickPhysicalDevice(const VkInstance &instance, const VkSurfaceKHR &surface, QueueFamilyIndices &queueIndices,
                              VkPhysicalDevice &physicalDevice) {
    ERR err = ERR::OK;
    physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_NanoContext.instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_NanoContext.instance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto &device : devices) {
        int score = rateDeviceSuitability(device, surface, queueIndices);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    int bestScore = 0;
    for (auto &candidate : candidates) {
        if (candidate.first > 0 && candidate.first > bestScore) {
            bestScore = candidate.first;
            physicalDevice = candidates.rbegin()->second;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        err = ERR::NOT_FOUND;
        throw std::runtime_error("failed to find a suitable GPU!");
    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

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
        LOG_MSG(ERRLevel::INFO, "Physical device selected: %s [%s]", deviceProperties.deviceName, deviceType);
    }

    return ERR::OK; // this is never reached if we use try/catch.
}

ERR createLogicalDevice(VkPhysicalDevice &physicalDevice, QueueFamilyIndices &indices, VkQueue &graphicsQueue, VkQueue &presentQueue,
                        VkDevice &device) {
    ERR err = ERR::OK;

    if (!indices.IsValid() && ERR::NOT_FOUND == findQueueFamilies(_NanoContext.physicalDevice, indices)) {
        throw std::runtime_error("failed to find a graphics familiy queue!");
    }

    // we know both the graphics and present indices are valid, so we don't need to worry about checking it again here
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        if (queueFamily == -1)
            continue;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    VkPhysicalDeviceFeatures deviceFeatures{}; // defaults all the features to false for now
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(SizeOf(Config::desiredDeviceExtensions));
    createInfo.ppEnabledExtensionNames = Config::desiredDeviceExtensions;
    if (Config::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(SizeOf(Config::desiredValidationLayers));
        createInfo.ppEnabledLayerNames = Config::desiredValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    if (indices.IsValid()) {
        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue); // The graphics queue is already created if we have successfully created
                                                                             // a logical device. This is only to retrieve the handle
        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue); // The graphics queue is already created if we have successfully created a
                                                                           // logical device. This is only to retrieve the handle
    }

    return err;
}

ERR createSurface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR &surface) {
    ERR err = ERR::OK;

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        err = ERR::INVALID;
        throw std::runtime_error("failed to create window surface!");
    }
    return err;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            LOG_MSG(ERRLevel::INFO, "swapchain format used: VK_FORMAT_B8G8R8A8_SRGB");
            LOG_MSG(ERRLevel::INFO, "swapchain colorspace used: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
            return availableFormat;
        }
    }
    return availableFormats[0]; // pick the first available format if we do not find the format desired
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            LOG_MSG(ERRLevel::INFO, "swapchain present mode used: VK_PRESENT_MODE_MAILBOX_KHR");
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        LOG_MSG(ERRLevel::INFO, "swapchain extent mode used: {%d,%d}", capabilities.currentExtent.width, capabilities.currentExtent.height);
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

ERR createSCImageViews(const VkDevice &device, SwapchainContext& swapchainContext) {
    ERR err = ERR::OK;
    swapchainContext.imageViews.resize(swapchainContext.images.size());
    for (size_t i = 0; i < swapchainContext.images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainContext.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainContext.info.selectedFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainContext.imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }

    return err;
}

ERR createFramebuffer(const VkDevice& device, const VkRenderPass& renderpass, SwapchainContext& swapchainContext){
    ERR err = ERR::OK;
    swapchainContext.framebuffers.resize(swapchainContext.info.imageCount);

    for (size_t i = 0; i < swapchainContext.imageViews.size(); i++) {
        VkImageView attachments[] = {swapchainContext.imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainContext.info.currentExtent.width;
        framebufferInfo.height = swapchainContext.info.currentExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainContext.framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
    return err;
}

ERR createSwapchain(const VkPhysicalDevice &physicalDevice, const VkDevice &device, GLFWwindow *window, const VkSurfaceKHR &surface, SwapchainContext& swapchainContext) {
    ERR err = ERR::OK;
    swapchainContext.info = querySwapChainSupport(physicalDevice, surface);

    swapchainContext.info.selectedFormat = chooseSwapSurfaceFormat(swapchainContext.info.formats);
    swapchainContext.info.selectedPresentMode = chooseSwapPresentMode(swapchainContext.info.presentModes);
    swapchainContext.info.currentExtent = chooseSwapExtent(window, swapchainContext.info.capabilities);

    swapchainContext.info.imageCount = swapchainContext.info.capabilities.minImageCount + 1;
    if (swapchainContext.info.capabilities.maxImageCount > 0 && swapchainContext.info.imageCount > swapchainContext.info.capabilities.maxImageCount) {
        swapchainContext.info.imageCount = swapchainContext.info.capabilities.maxImageCount;
    }

    LOG_MSG(ERRLevel::INFO, "Number of images used by the swapchain: %d", swapchainContext.info.imageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = swapchainContext.info.imageCount;
    createInfo.imageFormat = swapchainContext.info.selectedFormat.format;
    createInfo.imageColorSpace = swapchainContext.info.selectedFormat.colorSpace;
    createInfo.imageExtent = swapchainContext.info.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = {};
    err = findQueueFamilies(physicalDevice, indices);
    uint32_t queueFamilyIndices[2] = {};
    if (indices.IsValid()) {
        queueFamilyIndices[0] = indices.graphicsFamily;
        queueFamilyIndices[1] = indices.presentFamily;
    } else {
        LOG_MSG(ERRLevel::WARNING, "Queue Family indices is invalid. called in : createSwapchain(...)")
    }

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        LOG_MSG(ERRLevel::WARNING, "Graphics queue family is not the same as Present queue family. Swapchain images have to be concurent")
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapchainContext.info.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // TODO: If transparency is to be enabled, change this
    if (createInfo.compositeAlpha != VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        LOG_MSG(ERRLevel::INFO, "swapchain images not set to OPAQUE_BIT_KHR. Transparent window might be enabled")
    }
    createInfo.presentMode = swapchainContext.info.selectedPresentMode;
    createInfo.clipped = VK_TRUE; // This deals with obstructed pixels when, for example, another window is ontop.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchainContext.swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapchainContext.swapchain, &swapchainContext.info.imageCount, nullptr);
    swapchainContext.images.resize(swapchainContext.info.imageCount);
    vkGetSwapchainImagesKHR(device, swapchainContext.swapchain, &swapchainContext.info.imageCount, swapchainContext.images.data());

    return err;
}


ERR recreateSwapchain(const VkPhysicalDevice &physicalDevice, const VkDevice &device, GLFWwindow *window, const VkSurfaceKHR &surface, SwapchainContext& swapChainContext, const VkRenderPass& renderpass){
    ERR err = ERR::OK;
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


ERR createGraphicsPipeline(VkDevice& device,const SwapchainDetails& swapchainDetails, const VkRenderPass& renderpass, NanoGraphicsPipeline& graphicsPipeline) {
    ERR err = ERR::OK;

    graphicsPipeline.Init(device, swapchainDetails.currentExtent);
    graphicsPipeline.AddVertShader("./src/shader/shader.vert");
    graphicsPipeline.AddFragShader("./src/shader/shader.frag");
    graphicsPipeline.AddRenderPass(renderpass);
    graphicsPipeline.Compile();

    return err;
}

// renderpass with at least one color attachment
ERR createRenderPass(VkDevice& device, const SwapchainDetails& swapchainDetails, VkRenderPass& renderpass){
    ERR err = ERR::OK;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainDetails.selectedFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // The following other types of attachments can be referenced by a subpass:
    // pInputAttachments: Attachments that are read from a shader
    // pResolveAttachments: Attachments used for multisampling color attachments
    // pDepthStencilAttachment: Attachment for depth and stencil data
    // pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // always add the reference to the attachment and never the attachment itself
    // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!

    // subpass synchronization with swapchain images state
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; //The dstSubpass is our one and only subpass (for now). it must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    return err;
}

ERR createCommandPool(VkDevice& device, const QueueFamilyIndices& queueFamilyIndices, VkCommandPool& commandPool){
    ERR err = ERR::OK;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ASSERT(queueFamilyIndices.graphicsFamily != -1, "Graphics family indices is not defined - createCommandPool(...)");
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    return err;
}

ERR createCommandBuffer(VkDevice& device, const VkCommandPool& commandPool, VkCommandBuffer* commandBuffer, uint32_t size){
    ERR err = ERR::OK;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = size;

    for(int i = 0; i < size ; i++){
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    return err;
}

ERR recordCommandBuffer(NanoGraphicsPipeline& graphicsPipeline, VkFramebuffer& swapchainFrameBufferToWriteTo, VkCommandBuffer& commandBuffer) {
    ERR err = ERR::OK;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = graphicsPipeline.GetRenderPass();
    renderPassInfo.framebuffer = swapchainFrameBufferToWriteTo;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = graphicsPipeline.GetExtent();

    VkClearValue clearColor = {{{0.02f, 0.02f, 0.02f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipeline());

        // need to manually set the viewport and scissor here because we defined them as dynamic.
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(graphicsPipeline.GetExtent().width);
        viewport.height = static_cast<float>(graphicsPipeline.GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = graphicsPipeline.GetExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    return err;
}

ERR createSwapchainSyncObjects(VkDevice& device ,SwapchainSyncObjects* syncObjects, uint32_t size){
    ERR err = ERR::OK;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // we create it at a signaled state so we don't bloack at the very first frame.

    for(int i = 0; i < size ; i++){
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects[i].renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }

        if (vkCreateFence(device, &fenceInfo, nullptr, &syncObjects[i].inFlightFence)) {
            throw std::runtime_error("failed to create frences!");
        }
    }

    return err;
}

ERR InitGraphics(NanoGraphics* nanoGraphics, NanoWindow* window){
    ERR err = ERR::OK;
    nanoGraphics->m_pNanoContext = (NanoVKContext*)malloc(sizeof(NanoVKContext));
    // Here the err validation is not that useful
    // a iferr_return can be added at the end of each statement to check it's state and exit (or at least warn) if an error did occur
    err = createInstance(Config::APP_NAME,
                         Config::ENGINE_NAME,
                         _NanoContext.instance); // APP_NAME and ENGINE_NAME is defined in NanoConfig

    err = setupDebugMessenger(_NanoContext.instance,
                              debugMessenger); // this depends on whether we are running in debug or not

    err = createSurface(_NanoContext.instance,
                        window.getGLFWwindow(),
                        _NanoContext.surface);

    err = pickPhysicalDevice(_NanoContext.instance,
                             _NanoContext.surface,
                             _NanoContext.queueIndices,
                             _NanoContext.physicalDevice); // physical device is not created but picked based on scores dictated by the number of supported features

    err = createLogicalDevice(_NanoContext.physicalDevice,
                              _NanoContext.queueIndices,
                              _NanoContext.presentQueue,
                              _NanoContext.graphicsQueue,
                              _NanoContext.device); // Logical device *is* created and therefore has to be destroyed

    err = createSwapchain(_NanoContext.physicalDevice,
                          _NanoContext.device,
                          window.getGLFWwindow(),
                          _NanoContext.surface,
                          _NanoContext.swapchainContext);

    err = createSCImageViews(_NanoContext.device,
                             _NanoContext.swapchainContext);

    err = createRenderPass(_NanoContext.device,
                           _NanoContext.swapchainContext.info,
                           _NanoContext.renderpass);

    NanoGraphicsPipeline graphicsPipeline{};
    err = createGraphicsPipeline(_NanoContext.device,
                                 _NanoContext.swapchainContext.info,
                                 _NanoContext.renderpass,
                                 graphicsPipeline);

    _NanoContext.AddGraphicsPipeline(graphicsPipeline);

    err = createFramebuffer(_NanoContext.device,
                            _NanoContext.renderpass,
                            _NanoContext.swapchainContext);

    err = createCommandPool(_NanoContext.device,
                            _NanoContext.queueIndices,
                            _NanoContext.commandPool);

    err = createCommandBuffer(_NanoContext.device,
                              _NanoContext.commandPool,
                              _NanoContext.swapchainContext.commandBuffer,
                              Config::MAX_FRAMES_IN_FLIGHT);

    err = createSwapchainSyncObjects(_NanoContext.device,
                                     _NanoContext.swapchainContext.syncObjects,
                                     Config::MAX_FRAMES_IN_FLIGHT);

    return err;
}

ERR DrawFrame(NanoGraphics* nanoGraphics){
    ERR err = ERR::OK;

    vkWaitForFences(_NanoContext.device, 1, &_NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(_NanoContext.device, 1, &_NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(_NanoContext.device, _NanoContext.swapchainContext.swapchain, UINT64_MAX, _NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(_NanoContext.swapchainContext.commandBuffer[_NanoContext.swapchainContext.currentFrame], 0);

    recordCommandBuffer(*_NanoContext.currentGraphicsPipeline,
                        _NanoContext.swapchainContext.framebuffers[imageIndex], //swapchain framebuffer for the command buffer to operate on
                        _NanoContext.swapchainContext.commandBuffer[_NanoContext.swapchainContext.currentFrame]); //command buffer to write to.

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {_NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {_NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].renderFinishedSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_NanoContext.swapchainContext.commandBuffer[_NanoContext.swapchainContext.currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    if (vkQueueSubmit(_NanoContext.graphicsQueue, 1, &submitInfo, _NanoContext.swapchainContext.syncObjects[_NanoContext.swapchainContext.currentFrame].inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = {_NanoContext.swapchainContext.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(_NanoContext.presentQueue, &presentInfo);

    _NanoContext.swapchainContext.currentFrame = (_NanoContext.swapchainContext.currentFrame + 1) % Config::MAX_FRAMES_IN_FLIGHT;

    return err;
}