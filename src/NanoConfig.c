#include "NanoConfig.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* ENGINE_NAME = "NanoEngine";
const char* APP_NAME = "NanoApplication";

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const char* desiredValidationLayers[MAX_ARRAY_OF_EXTENSIONS] = {
    "VK_LAYER_KHRONOS_validation",
    NULL // to allow for while loops without crash
};

const char* desiredDeviceExtensions[MAX_ARRAY_OF_EXTENSIONS] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset",
#endif
    NULL // to allow for while loops without crash
};

const char* desiredInstanceExtensions[MAX_ARRAY_OF_EXTENSIONS] = {
#ifdef __APPLE__
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    "VK_KHR_get_physical_device_properties2",
#endif
    NULL // to allow for while loops without crash
};
