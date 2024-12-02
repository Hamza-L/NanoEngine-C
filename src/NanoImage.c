#include "NanoImage.h"
#include "NanoRenderer.h"
#include "NanoBuffers.h"
#include "NanoVkUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//TODO: Append the commands to the mainloop's recorded command buffers as an optimized pipeline transfer since this command only runs when the device Queue idles
void TransitionImageLayout(NanoRenderer* nanoRenderer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(nanoRenderer);
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // used to transfer queue ownership. Is not ignored by default so must be set manually!
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        /**
         * There is actually a special type of image layout that supports all operations, VK_IMAGE_LAYOUT_GENERAL.
         * The problem with it, of course, is that it doesn’t necessarily offer the best performance for any operation.
         * It is required for some special cases, like using an image as both input and output,
         * or for reading an image after it has left the preinitialized layout.
         * **/

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            fprintf(stderr, "unsupported layout transition!\n");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }
    EndAndSubmitSingleTimeCommands(nanoRenderer, commandBuffer);
}

void InitImage(NanoRenderer* nanoRenderer, NanoImage* nanoImage, uint32_t width, uint32_t height, IMAGE_FORMAT numChannels){
// create an image from scratch
}

VkImageView CreateImageView(NanoRenderer* nanoRenderer, VkImage image, VkFormat format){
    VkImageView imageView;
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(nanoRenderer->m_pNanoContext->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        fprintf(stderr, "failed to create texture image view!\n");
    }
    return imageView;
}

// file format default to RGBA for now
void InitImageFromFile(NanoRenderer* nanoRenderer, NanoImage* nanoImage, const char* fileName){
    int width, height, numChannels;
    stbi_uc* imageData = stbi_load(fileName, &width, &height, &numChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;

    nanoImage->width = width;
    nanoImage->height = height;
    nanoImage->numChannels = IMAGE_FORMAT_RGBA;
    nanoImage->imageDataSize = imageSize;

    if (!imageData) {
        fprintf(stderr, "failed to open and load image\n");
    }

    NanoVkBufferMemory stagingBufferMem; // we can use a VkBuffer for a VkImage copy
    stagingBufferMem = CreateBuffer(nanoRenderer,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    imageSize);

    void* data;
    vkMapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, (size_t)imageSize);
    vkUnmapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    stbi_image_free(imageData);

    nanoImage->nanoVkBuffer = CreateImageBuffer(nanoRenderer,
                                                width, height,
                                                VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_IMAGE_TILING_LINEAR,
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    TransitionImageLayout(nanoRenderer,
                          nanoImage->nanoVkBuffer.textureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(nanoRenderer,
                      stagingBufferMem.buffer,
                      nanoImage->nanoVkBuffer.textureImage,
                      width, height);

    TransitionImageLayout(nanoRenderer,
                          nanoImage->nanoVkBuffer.textureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    nanoImage->imageView = CreateImageView(nanoRenderer, nanoImage->nanoVkBuffer.textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    nanoImage->isInitialized = true;
}

void CleanUpImage(NanoRenderer* nanoRenderer, NanoImage* nanoImage){
    vkDestroyImageView(nanoRenderer->m_pNanoContext->device, nanoImage->imageView, nullptr);
    vkDestroyImage(nanoRenderer->m_pNanoContext->device, nanoImage->nanoVkBuffer.textureImage, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, nanoImage->nanoVkBuffer.textureImageMemory, nullptr);
}
