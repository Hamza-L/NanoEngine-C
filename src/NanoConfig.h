#ifndef NANOCONFIG_H_
#define NANOCONFIG_H_

#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_GRAPHICS_PIPELINE 16
#define MAX_FILEPATH_LENGTH 512
#define DEFAULT_STRING_ALLOC_LENGTH 512
#define MAX_ARRAY_OF_EXTENSIONS 256
#define MAX_SHORT_STRING_LENGTH 64
#define MAX_SUPPORTED_GPU_COUNT 8

// MEMORY SETTINGS
#define MAX_MEMORY_MESH_OBJECT 256

// max vertex per mesh object is 1024*16.
#define MAX_VERTEX_PER_OBJECT 16384

// Max supported size is 3840 x 2160 (4K textures)
#define MAX_NUM_ALLOCATED_IMAGES 1028

// Max supported size is 3840 x 2160 (4K textures)
#define MAX_IMAGE_MEMSIZE 8294400

// Max supported memory size of the image memory (for all images in the host app).
// is equivalent to 128 4K images;
#define MAX_TOTAL_ALLOCATED_IMAGES_MEMSIZE 1061683200

#define MAX_TEXTURE_PER_SCENE 256

//this is a physical device limitation on macos
#define MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE 16

typedef struct String String;

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;
extern const char* ENGINE_NAME;
extern const char* APP_NAME;
extern const bool enableValidationLayers;
extern bool FORCE_RECOMPILE;

extern const char* desiredValidationLayers[MAX_ARRAY_OF_EXTENSIONS];
extern const char* desiredDeviceExtensions[MAX_ARRAY_OF_EXTENSIONS];
extern const char* desiredInstanceExtensions[MAX_ARRAY_OF_EXTENSIONS];

char* GetArg0();
void GetExecutablePath(String* exePath);
void SetVar(char* arg0);

void SetForceShaderRecompile(bool isForceCompile);

#endif // NANOCONFIG_H_
