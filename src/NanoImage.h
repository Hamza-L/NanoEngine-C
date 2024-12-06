#ifndef NANOIMAGE_H_
#define NANOIMAGE_H_

#include <stdint.h>
#include "NanoBuffers.h"

typedef enum {
IMAGE_FORMAT_GRAY = 1,
IMAGE_FORMAT_RGB = 3,
IMAGE_FORMAT_RGBA = 4,
} IMAGE_FORMAT;

typedef struct {
    uint32_t numChannels; // 1, 3, 4 for GRAY, RGB, and RGBA respectively
    uint32_t width;
    uint32_t height;
    uint32_t imageDataSize;

    NanoVkImageMemory nanoVkBuffer;
    VkImageView imageView;

    bool isInitialized;
    unsigned char padding[3];
} NanoImage;

void InitText(NanoRenderer* nanoRenderer, NanoImage* nanoImage, const char* text);
void InitImage(NanoRenderer* nanoRenderer, NanoImage* nanoImage, uint32_t width, uint32_t height, IMAGE_FORMAT numChannels);
void InitImageFromFile(NanoRenderer* nanoRenderer, NanoImage* nanoImage, const char* fileName);
void CleanUpImage(NanoRenderer* nanoRenderer, NanoImage* nanoImage);

//utility function
VkImageView CreateImageView(NanoRenderer* nanoRenderer, VkImage image, VkFormat format);

#endif // NANOIMAGE_H_
