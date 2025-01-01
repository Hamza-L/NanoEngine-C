#ifndef NANOIMAGE_H_
#define NANOIMAGE_H_

#include <stdint.h>
#include "NanoBuffers.h"
#include "Str.h"

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

    bool isInitialized;
    bool isSubmittedToGPUMemory;
};

typedef struct{
    NanoVkImageMemory imageMemory;
} ImageVKMemory;

typedef struct{
    ImageVKMemory imageVKMemory;
    ImageHostMemory imageHostMemory;

    bool isInitialized;
} ImageMemory;

void AddTextToImage(NanoImage* nanoImage, const char* text, int fontSize, int verticalSpacing, float color[4]);
NanoImage CreateHostPersistentImage(ImageHostMemory* imageHostMemory, int width, int height, int numChannels, float colour[4]);
void InitHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, NanoImage* image, const char* fileName);
NanoImage CreateHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, const char* fileName);
void InitImage(uint32_t width, uint32_t height, IMAGE_FORMAT numChannels, NanoImage* image);
void InitImageFromFile(const char* fileName, NanoImage* image);
void CleanUpImageVkMemory(NanoRenderer* nanoRenderer, NanoImage* nanoImage);
void CleanUpAllImageMemory(NanoRenderer* nanoRenderer, ImageMemory* imageMemory);
void CleanUpImage(NanoRenderer* nanoRenderer, NanoImage* image);

void SubmitImageToGPUMemory(NanoRenderer* nanoRenderer, NanoImage* image);

//utility function
VkImageView CreateImageView(NanoRenderer* nanoRenderer, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

void WrapText(HeapString text, uint32_t width, uint32_t fontSize);

#endif // NANOIMAGE_H_
