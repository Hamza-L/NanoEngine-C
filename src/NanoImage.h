#ifndef NANOIMAGE_H_
#define NANOIMAGE_H_

#include <stdint.h>
#include "NanoBuffers.h"

typedef struct NanoImage NanoImage;

typedef enum {
IMAGE_FORMAT_GRAY = 1,
IMAGE_FORMAT_RGB = 3,
IMAGE_FORMAT_RGBA = 4,
} IMAGE_FORMAT;

struct NanoImage {
    uint32_t numChannels; // 1, 3, 4 for GRAY, RGB, and RGBA respectively
    uint32_t width;
    uint32_t height;
    uint32_t imageDataSize;

    ImageMemoryObject imageMemory;
    int32_t imageDescriptorID;

    NanoVkImageMemory nanoVkBuffer;
    VkImageView imageView;

    _Bool isInitialized;
    unsigned char padding[3];
};

typedef struct{
    NanoVkImageMemory imageMemory;
} ImageVKMemory;

typedef struct{
    ImageVKMemory imageVKMemory;
    ImageHostMemory imageHostMemory;

    bool isInitialized;
} ImageMemory;

void InitText(NanoRenderer* nanoRenderer, NanoImage* nanoImage, const char* text);
void InitHostPersistentImage(ImageHostMemory* imageHostMemory, uint32_t width, uint32_t height, IMAGE_FORMAT numChannels, NanoImage* nanoImage);
void InitHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, NanoImage* image, const char* fileName);
NanoImage CreateHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, const char* fileName);
void InitImage(uint32_t width, uint32_t height, IMAGE_FORMAT numChannels, NanoImage* image);
void InitImageFromFile(const char* fileName, NanoImage* image);
void CleanUpImageVkMemory(NanoRenderer* nanoRenderer, NanoImage* nanoImage);
void CleanUpImageMemory(NanoRenderer* nanoRenderer, ImageMemory* imageMemory);

void SubmitImageToGPUMemory(NanoRenderer* nanoRenderer, NanoImage* image);

//utility function
VkImageView CreateImageView(NanoRenderer* nanoRenderer, VkImage image, VkFormat format);

#endif // NANOIMAGE_H_
