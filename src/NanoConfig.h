#ifndef NANOCONFIG_H_
#define NANOCONFIG_H_

#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_GRAPHICS_PIPELINE 16
#define MAX_FILEPATH_LENGTH 512
#define DEFAULT_STRING_ALLOC_LENGTH 512
#define MAX_ARRAY_OF_EXTENSIONS 256

struct NamedStringConstant{
    char extensionName[128];
};

typedef struct NamedStringConstant DeviceExtension;
typedef struct NamedStringConstant InstanceExtension;
typedef struct NamedStringConstant ValidationLayer;

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;
extern const char* ENGINE_NAME;
extern const char* APP_NAME;
extern const bool enableValidationLayers;

extern const char* desiredValidationLayers[MAX_ARRAY_OF_EXTENSIONS];
extern const char* desiredDeviceExtensions[MAX_ARRAY_OF_EXTENSIONS];
extern const char* desiredInstanceExtensions[MAX_ARRAY_OF_EXTENSIONS];

#endif // NANOCONFIG_H_