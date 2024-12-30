#include "NanoConfig.h"
#include "Str.h"
#include <string.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* ENGINE_NAME = "NanoEngine";
const char* APP_NAME = "NanoApplication";

static char s_arg0[512] = {};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

bool FORCE_RECOMPILE = false;

const char* desiredValidationLayers[MAX_ARRAY_OF_EXTENSIONS] = {
    "VK_LAYER_KHRONOS_validation",
    NULL_STR // to allow for while loops without crash
};

const char* desiredDeviceExtensions[MAX_ARRAY_OF_EXTENSIONS] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset",
    "VK_KHR_maintenance1",
#endif
    NULL_STR // to allow for while loops without crash
};

const char* desiredInstanceExtensions[MAX_ARRAY_OF_EXTENSIONS] = {
#ifdef __APPLE__
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    "VK_KHR_get_physical_device_properties2",
#endif
    NULL_STR
    // to allow for while loops without crash
};

void SetForceShaderRecompile(bool isForceCompile){
    FORCE_RECOMPILE = isForceCompile;
}

void SetArg0(char *arg0){
    memcpy(s_arg0, arg0, 512);
}

char* GetArg0(){
    return s_arg0;
}

void GetExecutablePath(String* exePath){
    InitString(exePath, s_arg0);
    int indxLastFSlash = FindLastRawString(s_arg0, "/");
    SubString(exePath, 0, indxLastFSlash + 1);
}
