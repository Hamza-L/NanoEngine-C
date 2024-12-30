#include "NanoImage.h"
#include "MemManager.h"
#include "NanoRenderer.h"
#include "NanoBuffers.h"
#include "NanoUtility.h"
#include "NanoVkUtility.h"
#include "Str.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library  library;
FT_Face     face;      /* handle to face object */

typedef struct {
    uint32_t width;
    uint32_t height;
} TextDimensions;

void WrapText(HeapString text, uint32_t width, uint32_t fontSize){
    int textWidth = 0;

    FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
    int           pen_x, n;
    int num_chars = text.m_size;
    int error = FT_Set_Pixel_Sizes(
        face,
        0, // 0 if same as pixel height
        fontSize);


    int indxStartOfWord = 0;

    pen_x = 0;
    for ( n = 0; n < text.m_size; n++ ) //the size might change with the addition of new lines
    {
        FT_UInt  glyph_index;

        /* retrieve glyph index from character code */
        glyph_index = FT_Get_Char_Index( face, text.m_pData[n] );

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_BITMAP );
        if ( error )
            continue;  /* ignore errors */

        if(text.m_pData[n] == ' ' || text.m_pData[n] == '\n' || text.m_pData[n] == '/'){
            indxStartOfWord = n + 1;
            if(text.m_pData[n] == '\n'){
                pen_x = 0;
            }
        }

        pen_x += slot->advance.x >> 6; // divide by 64 */

        if (pen_x > width){
            pen_x = 0;

            if(text.m_pData[n] == ' ') {
                indxStartOfWord = n;
            }

            char textToAppend[text.m_size - indxStartOfWord + 1] = {}; // extra space for null termination
            memcpy(textToAppend, &text.m_pData[indxStartOfWord], text.m_size - indxStartOfWord);
            // while(textToAppend[offset] == ' ') {
            //     offset++;
            // }
            text.m_size = indxStartOfWord + 1;
            AppendToHeapString(&text, &textToAppend[0]);
            text.m_pData[indxStartOfWord] = '\n';

            n = indxStartOfWord;
        }
    }
}

TextDimensions GetTextDimensions(const char* text, int pixelFontSize){
    int textHeight = 0;
    int textWidth = 0;

    FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
    int           pen_x, pen_y, n;
    int num_chars = strlen(text);
    int error = FT_Set_Pixel_Sizes(
        face,
        0, // 0 if same as pixel height
        pixelFontSize);

    pen_x = 0;
    for ( n = 0; n < num_chars; n++ )
    {
        FT_UInt  glyph_index;

        /* retrieve glyph index from character code */
        glyph_index = FT_Get_Char_Index( face, text[n] );

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_BITMAP );
        if ( error )
            continue;  /* ignore errors */

        int yOffset = slot->bitmap.rows;
        if (yOffset > textHeight) {
            textHeight = yOffset;
        }

        if(text[n] == '\n'){
            pen_x = 0;
        }

        pen_x += slot->advance.x >> 6; // divide by 64 */

        if (pen_x > textWidth) {
            textWidth = pen_x;
        }
    }
    TextDimensions dimensions = {.width = textWidth, .height = textHeight};
    return dimensions;
}

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
            LOG_MSG(stderr, "unsupported layout transition!\n");
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

void AddTextToImage(NanoImage* nanoImage, const char* text, int fontSize, int verticalSpacing, float color[4]){

    if(nanoImage->imageMemory.imageData == nullptr)
        LOG_MSG(stderr, "Image should be allocated first before adding text to it\n");

    // create an image from scratch
    int error = FT_Init_FreeType( &library );
    if ( error ) {
        LOG_MSG(stderr, "ERROR OCCURED LOADING FONTS\n");
        abort();
    }

    error = FT_New_Face( library,
                         PrependCWD("Fonts/CascadiaCode.ttf").m_data, // can't use relative paths here
                         0,
                         &face );

    if ( error == FT_Err_Unknown_File_Format ) {
        LOG_MSG(stderr, "UNKNOWW FILE FORMAT FOR GIVEN FONTS\n");
        abort();
    }
    else if ( error ) {
        LOG_MSG(stderr, "UNKOWN ERROR OCCURED LOADING FONTS\n");
        abort();
    }

    error = FT_Set_Pixel_Sizes(
        face,
        0, // 0 if same as pixel height
        fontSize);

    if ( error ) {
        LOG_MSG(stderr, "ERROR SETTING FONT SIZE\n");
        abort();
    }

    TextDimensions dimensions = GetTextDimensions(text, fontSize);
    HeapString heapText = AllocHeapString(text);
    WrapText(heapText, nanoImage->width, fontSize);

    FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
    int           pen_x, pen_y, n;
    const int num_chars = (int)strlen(heapText.m_pData); //TODO: Size if wrong here. we'll use strlen for now
    int numChannels = 4;
    int width = nanoImage->width;
    int height = nanoImage->height;

    unsigned char* imageData = (unsigned char*)nanoImage->imageMemory.imageData;

    pen_x = 0;
    int yOffset = (verticalSpacing + dimensions.height);

    for ( n = 0; n < num_chars; n++ )
    {
        FT_UInt  glyph_index;

        /* retrieve glyph index from character code */
        glyph_index = FT_Get_Char_Index( face, heapText.m_pData[n] );

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
        if ( error )
            continue;  /* ignore errors */

        /* convert to an anti-aliased bitmap */
        error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
        if ( error )
            continue;

        if(heapText.m_pData[n] == '\n'){
            pen_x = 0;
            yOffset += (verticalSpacing + dimensions.height);
            continue;
        }

        int xOffset = pen_x + slot->bitmap_left;
        for (int y = yOffset - slot->bitmap_top; y < yOffset + slot->bitmap.rows - slot->bitmap_top; y++) {
            for (int x = xOffset; x < xOffset + slot->bitmap.width; x++) {
                uint32_t bmapW = slot->bitmap.width;
                unsigned char alphaVal = slot->bitmap.buffer[((y - (yOffset - slot->bitmap_top)) * bmapW) + (x - xOffset)];
                float alphaValNormalized = alphaVal/255.0f;
                if ( alphaVal == 0 ){
                    continue;
                }
                imageData[(y * width * numChannels) + (x * numChannels) + 0] =  (1-alphaValNormalized) * imageData[(y * width * numChannels) + (x * numChannels) + 0] + (alphaValNormalized) * (color[0] * 255); // r
                imageData[(y * width * numChannels) + (x * numChannels) + 1] =  (1-alphaValNormalized) * imageData[(y * width * numChannels) + (x * numChannels) + 1] + (alphaValNormalized) * (color[1] * 255); // g
                imageData[(y * width * numChannels) + (x * numChannels) + 2] =  (1-alphaValNormalized) * imageData[(y * width * numChannels) + (x * numChannels) + 2] + (alphaValNormalized) * (color[2] * 255); // b
                imageData[(y * width * numChannels) + (x * numChannels) + 3] =  (1-alphaValNormalized) * imageData[(y * width * numChannels) + (x * numChannels) + 3] + (alphaValNormalized) * (color[3] * 255); // a
            }
        }

        /* increment pen position */
        pen_x += slot->advance.x >> 6; // divide by 64 */
        /* pen_y += slot->advance.y >> 6; /\* not useful for now *\/ */
    }

}

void InitHostPersistentImage(ImageHostMemory* imageHostMemory, uint32_t width, uint32_t height, IMAGE_FORMAT numChannels, NanoImage* nanoImage){
    // create an image from scratch
    const VkDeviceSize imageSize = width * height * numChannels;
    nanoImage->imageMemory = GetAllocateImageMemoryObject(imageHostMemory, imageSize);
    nanoImage->imageDescriptorID = - 1;

    int checkerBoardSquareSize = 64;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int isColored = ((x % (checkerBoardSquareSize*2)) > checkerBoardSquareSize) ^ ((y % (checkerBoardSquareSize*2)) > checkerBoardSquareSize);
            nanoImage->imageMemory.imageData[(y * width * numChannels) + (x * numChannels)] = isColored * 255; // r
            nanoImage->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 1] = isColored * 128; // g
            nanoImage->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 2] = 0; // b
            nanoImage->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 3] = (char)255; // a
        }
    }

    nanoImage->width = width;
    nanoImage->height = height;
    nanoImage->numChannels = IMAGE_FORMAT_RGBA;
    nanoImage->imageDataSize = imageSize;
}

void InitImage(uint32_t width, uint32_t height, IMAGE_FORMAT numChannels, NanoImage* image){
    // create an image from scratch
    const VkDeviceSize imageSize = width * height * numChannels;
    image->imageMemory.imageData = (char*)malloc(sizeof(char)*imageSize);
    image->imageMemory.imageMemSize = imageSize;

    int checkerBoardSquareSize = 64;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int isColored = ((x % (checkerBoardSquareSize*2)) > checkerBoardSquareSize) ^ ((y % (checkerBoardSquareSize*2)) > checkerBoardSquareSize);
            image->imageMemory.imageData[(y * width * numChannels) + (x * numChannels)] = isColored * 255; // r
            image->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 1] = isColored * 128; // g
            image->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 2] = 0; // b
            image->imageMemory.imageData[(y * width * numChannels) + (x * numChannels) + 3] = (char)255; // a
        }
    }

    image->width = width;
    image->height = height;
    image->numChannels = IMAGE_FORMAT_RGBA;
    image->imageDataSize = imageSize;
}

// file format default to RGBA for now
NanoImage CreateHostPersistentImage(ImageHostMemory* imageHostMemory, int width, int height, int numChannels, float colour[4]){
    NanoImage image;
    int imageSize = width * height * numChannels;

    image.imageMemory = GetAllocateImageMemoryObject(imageHostMemory, imageSize);
    char* imageData = image.imageMemory.imageData;

    for (int y=0; y < height; y++)
    {
        for (int x=0; x < width; x++)
        {
            imageData[y*width*numChannels+x*numChannels + 0] = colour[0] * 255;
            imageData[y*width*numChannels+x*numChannels + 1] = colour[1] * 255;
            imageData[y*width*numChannels+x*numChannels + 2] = colour[2] * 255;
            imageData[y*width*numChannels+x*numChannels + 3] = colour[3] * 255;
        }
    }

    image.width = width;
    image.height = height;
    image.numChannels = IMAGE_FORMAT_RGBA;
    image.imageDataSize = imageSize;
    image.imageDescriptorID = - 1;

    if (!imageData) {
        LOG_MSG(stderr, "failed to allocate an image\n");
    }

    return image;
}

// file format default to RGBA for now
NanoImage CreateHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, const char* fileName){
    NanoImage image;
    int width, height, numChannels;
    stbi_uc* imageData = stbi_load(fileName, &width, &height, &numChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;

    image.width = width;
    image.height = height;
    image.numChannels = IMAGE_FORMAT_RGBA;
    image.imageDataSize = imageSize;
    image.imageDescriptorID = - 1;

    if (!imageData) {
        LOG_MSG(stderr, "failed to open and load image\n");
    }

    CopyImageDataToAllocatedMemoryObject(imageHostMemory, (char*)imageData, imageSize, &image);

#if TEXTURE_DEBUG
    float color[4] = {0,0,0,1.0f};
    int textLength = strlen(fileName);
    AddTextToImage(&image, fileName, image.width / 16.0f, 10, color);
#endif

    stbi_image_free(imageData);
    return image;
}

// file format default to RGBA for now
void InitHostPersistentImageFromFile(ImageHostMemory* imageHostMemory, NanoImage* image, const char* fileName){
    int width, height, numChannels;
    stbi_uc* imageData = stbi_load(fileName, &width, &height, &numChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;

    image->width = width;
    image->height = height;
    image->numChannels = IMAGE_FORMAT_RGBA;
    image->imageDataSize = imageSize;
    image->imageDescriptorID = - 1;

    if (!imageData) {
        LOG_MSG(stderr, "failed to open and load image\n");
    }

    CopyImageDataToAllocatedMemoryObject(imageHostMemory, (char*)imageData, imageSize, image);

    stbi_image_free(imageData);
}

// file format default to RGBA for now
void InitImageFromFile(const char* fileName, NanoImage* image){
    int width, height, numChannels;
    stbi_uc* imageData = stbi_load(fileName, &width, &height, &numChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;

    image->imageMemory.imageData = (char*)malloc(sizeof(char)*imageSize);
    image->imageMemory.imageMemSize = imageSize;

    image->width = width;
    image->height = height;
    image->numChannels = IMAGE_FORMAT_RGBA;
    image->imageDataSize = imageSize;

    if (!imageData) {
        LOG_MSG(stderr, "failed to open and load image\n");
    }

    memcpy(image->imageMemory.imageData, imageData, imageSize);

    stbi_image_free(imageData);
}

void SubmitImageToGPUMemory(NanoRenderer* nanoRenderer, NanoImage* image){
    NanoVkBufferMemory stagingBufferMem; // we can use a VkBuffer for a VkImage copy
    stagingBufferMem = CreateBuffer(nanoRenderer,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    image->imageMemory.imageMemSize);

    void* data;
    vkMapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, 0, image->imageMemory.imageMemSize, 0, &data);
    memcpy(data, image->imageMemory.imageData, (size_t)image->imageMemory.imageMemSize);
    vkUnmapMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory);

    //free(image->imageMemory->imageData);

    if(!image->isInitialized){
        image->nanoVkBuffer = CreateImageBuffer(nanoRenderer,
                                                image->width, image->height,
                                                VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_IMAGE_TILING_LINEAR,
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    TransitionImageLayout(nanoRenderer,
                          image->nanoVkBuffer.textureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(nanoRenderer,
                      stagingBufferMem.buffer,
                      image->nanoVkBuffer.textureImage,
                      image->width, image->height);

    TransitionImageLayout(nanoRenderer,
                          image->nanoVkBuffer.textureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, stagingBufferMem.buffer, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, stagingBufferMem.bufferMemory, nullptr);

    if(!image->isInitialized)
        image->imageView = CreateImageView(nanoRenderer, image->nanoVkBuffer.textureImage, VK_FORMAT_R8G8B8A8_SRGB);

    image->isInitialized = true;
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
        LOG_MSG(stderr, "failed to create texture image view!\n");
    }
    return imageView;
}

void CleanUpImageVkMemory(NanoRenderer* nanoRenderer, NanoImage* nanoImage){
    vkDestroyImageView(nanoRenderer->m_pNanoContext->device, nanoImage->imageView, nullptr);
    vkDestroyImage(nanoRenderer->m_pNanoContext->device, nanoImage->nanoVkBuffer.textureImage, nullptr);
    vkFreeMemory(nanoRenderer->m_pNanoContext->device, nanoImage->nanoVkBuffer.textureImageMemory, nullptr);
}

void CleanUpImageMemory(NanoRenderer* nanoRenderer, ImageMemory* imageMemory){
    CleanUpImageHostMemory(&imageMemory->imageHostMemory);
}
