#include "NanoImage.h"
#include "NanoRenderer.h"
#include "NanoBuffers.h"
#include "NanoVkUtility.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library  library;
FT_Face     face;      /* handle to face object */

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

void InitText(NanoRenderer* nanoRenderer, NanoImage* nanoImage, const char* text){
    // create an image from scratch
    int error = FT_Init_FreeType( &library );
    if ( error ) {
        fprintf(stderr, "ERROR OCCURED LOADING FONTS\n");
        abort();
    }
    error = FT_New_Face( library,
                         "/Users/h_lahmimsi/Library/Fonts/Cascadia.ttf",
                         0,
                         &face );

    if ( error == FT_Err_Unknown_File_Format ) {
        fprintf(stderr, "UNKNOWW FILE FORMAT FOR GIVEN FONTS\n");
        abort();
    }
    else if ( error ) {
        fprintf(stderr, "UNKOWN ERROR OCCURED LOADING FONTS\n");
        abort();
    }

    uint32_t charWidth = 128;
    error = FT_Set_Pixel_Sizes(
        face,
        0, // 0 if same as pixel height
        charWidth);

    if ( error ) {
        fprintf(stderr, "ERROR SETTING FONT SIZE\n");
        abort();
    }

    FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
    int           pen_x, pen_y, n;
    const int num_chars = strlen(text);
    int numChannels = 4;
    int width = charWidth * num_chars;
    int height = charWidth + 45;
    const VkDeviceSize imageSize = width * height * numChannels;
    unsigned char* imageData = (unsigned char*)malloc(imageSize);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            imageData[(y * width * numChannels) + (x * numChannels)] = (y/(float)height) * 255; // r
            imageData[(y * width * numChannels) + (x * numChannels) + 1] = 0; // g
            imageData[(y * width * numChannels) + (x * numChannels) + 2] = 0; // b
            imageData[(y * width * numChannels) + (x * numChannels) + 3] = 255; // a
        }
    }



    pen_x = 0;

    for ( n = 0; n < num_chars; n++ )
    {
        FT_UInt  glyph_index;

        /* retrieve glyph index from character code */
        glyph_index = FT_Get_Char_Index( face, text[n] );

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
        if ( error )
            continue;  /* ignore errors */

        /* convert to an anti-aliased bitmap */
        error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
        if ( error )
            continue;

        int yOffset = height - slot->bitmap_top - 45;
        int xOffset = pen_x + slot->bitmap_left;
        for (int y = yOffset ; y < yOffset + slot->bitmap.rows; y++) {
            for (int x = xOffset; x < xOffset + slot->bitmap.width; x++) {
                uint32_t bmapW = slot->bitmap.width;
                unsigned char alphaVal = slot->bitmap.buffer[((y - yOffset) * bmapW) + (x - xOffset)];
                if ( alphaVal == 0 ){
                    continue;
                }
                imageData[(y * width * numChannels) + (x * numChannels)] = 255; // r
                imageData[(y * width * numChannels) + (x * numChannels) + 1] = 197; // g
                imageData[(y * width * numChannels) + (x * numChannels) + 2] = 211; // b
                imageData[(y * width * numChannels) + (x * numChannels) + 3] = slot->bitmap.buffer[((y - yOffset) * bmapW) + (x - xOffset)]; // a
            }
        }
        /* /\* now, draw to our target surface *\/ */
        /* my_draw_bitmap( &slot->bitmap, */
        /*                 pen_x + slot->bitmap_left, */
        /*                 pen_y - slot->bitmap_top ); */

        /* increment pen position */
        pen_x += slot->advance.x >> 6; // divide by 64 */
        /* pen_y += slot->advance.y >> 6; /\* not useful for now *\/ */
    }



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

    free(imageData);

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

void InitImage(NanoRenderer* nanoRenderer, NanoImage* nanoImage, uint32_t width, uint32_t height, IMAGE_FORMAT numChannels){
    // create an image from scratch
    const VkDeviceSize imageSize = width * height * numChannels;
    unsigned char* imageData = (unsigned char*)malloc(imageSize);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            imageData[(y * width * numChannels) + (x * numChannels)] = (y/(float)height) * 255; // r
            imageData[(y * width * numChannels) + (x * numChannels) + 1] = 0; // g
            imageData[(y * width * numChannels) + (x * numChannels) + 2] = 0; // b
            imageData[(y * width * numChannels) + (x * numChannels) + 3] = 255; // a
        }
    }

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

    free(imageData);

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
