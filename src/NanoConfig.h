#ifndef NANOCONFIG_H_
#define NANOCONFIG_H_

#include <vulkan/vulkan_core.h>
#include <stdbool.h>

const uint16_t WINDOW_WIDTH = 800;
const uint16_t WINDOW_HEIGHT = 600;
#define MAX_FRAMES_IN_FLIGHT 2
#define ENGINE_NAME "NanoEngine"
#define APP_NAME "NanoApplication"
#define MAX_FILEPATH_LENGTH 512


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const char *desiredValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    NULL // to allow for while loops without crash
};

const char *desiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset",
#endif
    NULL // to allow for while loops without crash
};

const char *desiredInstanceExtensions[] = {
#ifdef __APPLE__
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    "VK_KHR_get_physical_device_properties2",
#endif
    NULL // to allow for while loops without crash
};

#endif // NANOCONFIG_H_
