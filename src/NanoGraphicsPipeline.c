#include "NanoGraphicsPipeline.h"
#include "NanoConfig.h"
#include "NanoImage.h"
#include "NanoShader.h"
#include "NanoBuffers.h"
#include "NanoRenderer.h"
#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "vulkan/vulkan_core.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cglm/cglm.h>

double startTime = 0;
bool timeStarted = false;

void CreateTextureSampler(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR; //we would use VK_FILTER_NEAREST if we want a more pixelated look
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = nanoRenderer->m_pNanoContext->deviceProperties.limits.maxSamplerAnisotropy;
    /* samplerInfo.anisotropyEnable = VK_FALSE; */ //if we do not want anisotropy
    /* samplerInfo.maxAnisotropy = 1.0f; */
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if(vkCreateSampler(nanoRenderer->m_pNanoContext->device, &samplerInfo, nullptr, &graphicsPipeline->m_sampler) != VK_SUCCESS) {
        fprintf(stderr, "failed to create texture sampler!\n");
        abort();
    }
}

void CreateUniformBuffersWithMappedMem(NanoRenderer* nanoRenderer, NanoVkBufferMemory UniformMemoryToInitialize[], uint32_t numBuffers){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    for (int i = 0; i < numBuffers; i++) {
        UniformMemoryToInitialize[i] = CreateBuffer(nanoRenderer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize);
        vkMapMemory(nanoRenderer->m_pNanoContext->device, UniformMemoryToInitialize[i].bufferMemory, 0, bufferSize, 0, &UniformMemoryToInitialize[i].bufferMemoryMapped);
    }
}

void UpdateDynamicUniformBufferMemory(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoVkBufferMemory UniformMemoryToUpdate){
    // update the whole list of model
    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = UniformMemoryToUpdate.bufferMemory;
    memoryRange.size = graphicsPipeline->uniformBufferDynamicAllignment* MAX_OBJECT_PER_SCENE;
    vkFlushMappedMemoryRanges(nanoRenderer->m_pNanoContext->device, 1, &memoryRange);
}

void CreateDynamicUniformBufferWithMappedMem(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoVkBufferMemory uboDynamicMemoryToInitialize[], uint32_t numBuffers){
// Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = nanoRenderer->m_pNanoContext->deviceProperties.limits.minUniformBufferOffsetAlignment;
    graphicsPipeline->uniformBufferDynamicAllignment = sizeof(mat4);
    if (minUboAlignment > 0) {
        graphicsPipeline->uniformBufferDynamicAllignment = (graphicsPipeline->uniformBufferDynamicAllignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    size_t bufferSize = MAX_OBJECT_PER_SCENE * graphicsPipeline->uniformBufferDynamicAllignment;
    /* uboDynamic->model = (mat4*)aligned_alloc(uboDynamic->dynamicAlignment, bufferSize); */

    for (int i = 0; i < numBuffers; i++) {
        uboDynamicMemoryToInitialize[i] = CreateBuffer(nanoRenderer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufferSize);
        vkMapMemory(nanoRenderer->m_pNanoContext->device, uboDynamicMemoryToInitialize[i].bufferMemory, 0, bufferSize, 0, &uboDynamicMemoryToInitialize[i].bufferMemoryMapped);
    }

    // zero initialize the matrices
    for(int i = 0; i < numBuffers; i++){
        mat4* modelMemory = (mat4*)uboDynamicMemoryToInitialize[i].bufferMemoryMapped;
        for(size_t i = 0; i < MAX_OBJECT_PER_SCENE ; i++)
        {
            glm_mat4_identity(modelMemory[i]);
        }
    }

    for(int i = 0 ; i < numBuffers; i++) {
        VkMappedMemoryRange memoryRange = {};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.memory = uboDynamicMemoryToInitialize[i].bufferMemory;
        memoryRange.size = graphicsPipeline->uniformBufferDynamicAllignment * MAX_OBJECT_PER_SCENE;
        vkFlushMappedMemoryRanges(nanoRenderer->m_pNanoContext->device, 1, &memoryRange);
    }
}

void UpdateDescriptorSets(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkWriteDescriptorSet descriptorWrites[3] = {};
        uint32_t numDescriptorUpdates = 2;

        // Uniform buffer descriptor set update
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = graphicsPipeline->UniformBufferMemory[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = graphicsPipeline->DescSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr; // Optional
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        // Dynamic Uniform buffer descriptor set update
        VkDescriptorBufferInfo dynamicBufferInfo = {};
        dynamicBufferInfo.buffer = graphicsPipeline->uniformBufferDynamicMemory[i].buffer;
        dynamicBufferInfo.offset = 0;
        dynamicBufferInfo.range = sizeof(UniformBufferObjectDynamic);

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = graphicsPipeline->DescSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &dynamicBufferInfo;
        descriptorWrites[1].pImageInfo = nullptr; // Optional
        descriptorWrites[1].pTexelBufferView = nullptr; // Optional

        // combined image sampler descriptor set update
        VkDescriptorImageInfo imageInfos[MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE] = {};
        int textureCount = 0;
        for(int j = 0; j < MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE; j++){
            if(graphicsPipeline->textures[j] ){
                graphicsPipeline->textures[j]->imageDescriptorID = textureCount;
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = graphicsPipeline->textures[j]->isInitialized ? graphicsPipeline->textures[j]->imageView : graphicsPipeline->defaultTexture.imageView; //For now we use the same image for all in flight frames. We also use only the first image
                imageInfos[j].sampler = graphicsPipeline->m_sampler;
                textureCount++;
            } else {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = graphicsPipeline->emptyTexture.imageView; //For now we use the same image for all in flight frames. We also use only the first image
                imageInfos[j].sampler = graphicsPipeline->m_sampler;
            }
        }
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = graphicsPipeline->DescSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE;
        descriptorWrites[2].pImageInfo = imageInfos;
        descriptorWrites[2].pTexelBufferView = nullptr; // Optional

        numDescriptorUpdates += 1; //add 1 if we have at least one image update

        vkUpdateDescriptorSets(nanoRenderer->m_pNanoContext->device, numDescriptorUpdates, descriptorWrites, 0, nullptr);
    }
}

static void CreateDescriptorSets(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
      layouts[i] = graphicsPipeline->m_descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = graphicsPipeline->m_descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(nanoRenderer->m_pNanoContext->device, &allocInfo, graphicsPipeline->DescSets) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate descriptor sets!\n");
    }
}

static void CreateDescriptorSetLayout(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1; //could pass an array of uniform buffer objects
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional. used for texture sampling

    VkDescriptorSetLayoutBinding uboDynLayoutBinding = {};
    uboDynLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboDynLayoutBinding.binding = 1;
    uboDynLayoutBinding.descriptorCount = 1;
    uboDynLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboDynLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.descriptorCount = MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[3] = {uboLayoutBinding, uboDynLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(nanoRenderer->m_pNanoContext->device, &layoutInfo, nullptr, &graphicsPipeline->m_descriptorSetLayout) != VK_SUCCESS) {
      fprintf(stderr, "failed to create descriptor set layout!");
      ASSERT(false, "failed to create descriptor set layout!");
    }
}

static void CreateDescriptorPool(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    VkDescriptorPoolSize poolSize[3] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = (uint32_t)(MAX_FRAMES_IN_FLIGHT);
    poolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize[1].descriptorCount = (uint32_t)(MAX_FRAMES_IN_FLIGHT); //TODO: need to make sure we don't need MAX_FRAMES_IN_FLIGHT amount
    poolSize[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[2].descriptorCount = (uint32_t)(MAX_FRAMES_IN_FLIGHT * MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_PER_SCENE);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)3;
    poolInfo.pPoolSizes = poolSize;
    poolInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(nanoRenderer->m_pNanoContext->device, &poolInfo, nullptr, &graphicsPipeline->m_descriptorPool) != VK_SUCCESS) {
      fprintf(stderr, "failed to create descriptor pool!\n");
    }

}

void AddImageToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoImage* nanoImage){
    graphicsPipeline->textures[graphicsPipeline->numTextures] = nanoImage;
    graphicsPipeline->numTextures++;
}

static void AddDefaultTexture(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    InitImage(256, 256, IMAGE_FORMAT_RGBA, &graphicsPipeline->defaultTexture);
    SubmitImageToGPUMemory(nanoRenderer, &graphicsPipeline->defaultTexture);
}

static void AddEmptyTexture(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    InitImage(1, 1, IMAGE_FORMAT_RGBA, &graphicsPipeline->emptyTexture);
    SubmitImageToGPUMemory(nanoRenderer, &graphicsPipeline->emptyTexture);
}

void InitGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const VkExtent2D extent){
    graphicsPipeline->m_extent = extent;
    graphicsPipeline->m_isInitialized = true;
    graphicsPipeline->numTextures = 0;

    glm_mat4_identity(graphicsPipeline->uniformBuffer.proj);
    glm_mat4_identity(graphicsPipeline->uniformBuffer.view);

    // does not use a staging buffer since it's updated at every frame
    CreateUniformBuffersWithMappedMem(nanoRenderer, graphicsPipeline->UniformBufferMemory, MAX_FRAMES_IN_FLIGHT);
    CreateDynamicUniformBufferWithMappedMem(nanoRenderer, graphicsPipeline, graphicsPipeline->uniformBufferDynamicMemory, MAX_FRAMES_IN_FLIGHT);

    CreateTextureSampler(nanoRenderer, graphicsPipeline);
    AddDefaultTexture(nanoRenderer, graphicsPipeline);
    AddEmptyTexture(nanoRenderer, graphicsPipeline);
}

void AddVertShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoShaderConfig config){
    InitShader(nanoRenderer, &graphicsPipeline->m_vertShader, config);
}

void AddFragShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoShaderConfig config){
    InitShader(nanoRenderer, &graphicsPipeline->m_fragShader, config);
}

void UpdateGraphicsPipelineAtFrame(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, uint32_t currentFrame){
    UniformBufferObject* ubo = &graphicsPipeline->uniformBuffer;

    // View matrix
    vec3 eye = {0.0f, 0.0f, 5.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(eye, center, up, ubo->view);

    // Projection matrix
    glm_perspective(glm_rad(50.0f), graphicsPipeline->m_extent.width / (float)graphicsPipeline->m_extent.height, 0.1f, 10.0f, ubo->proj);
    /* ubo->proj[1][1] *= -1; */
    /* vec3 axis = {0.0f, 1.0f, 0.0f}; */
    /* glm_mat4_identity(ubo.model); */
    /* glm_rotate(ubo.model, 45.0f, axis); */

    if(!timeStarted){
        startTime = (double)clock()/CLOCKS_PER_SEC;
        timeStarted = true;
    }
    /* double currentTime = (double)clock()/CLOCKS_PER_SEC - startTime; */

    /* fprintf(stderr, "currentTime: %f\n", currentTime); */
    memcpy(graphicsPipeline->UniformBufferMemory[currentFrame].bufferMemoryMapped, ubo, sizeof(UniformBufferObject));

    UpdateDynamicUniformBufferMemory(nanoRenderer, graphicsPipeline, graphicsPipeline->uniformBufferDynamicMemory[currentFrame]);
}

void SetupDescriptors(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    CreateDescriptorSetLayout(nanoRenderer, graphicsPipeline);
    CreateDescriptorPool(nanoRenderer, graphicsPipeline);
    CreateDescriptorSets(nanoRenderer, graphicsPipeline);
}

ERR CompileGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile){
    ERR err = OK;

    if(!graphicsPipeline->m_vertShader.m_isCompiled || forceReCompile){
        CompileShader(nanoRenderer, &graphicsPipeline->m_vertShader, true);
        /* ASSERT(vertShader->m_isCompiled, "graphics pipeline's vertex shader was not compiled\n"); */
        /* return NOT_INITIALIZED; */
    }

    if(graphicsPipeline->m_fragShader.m_isCompiled || forceReCompile){
        CompileShader(nanoRenderer, &graphicsPipeline->m_fragShader, true);
        /* ASSERT(fragShader->m_isCompiled, "graphics pipeline's fragment shader was not compiled\n"); */
        /* return NOT_INITIALIZED; */
    }

    // Shaders ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    typedef struct {
        uint32_t fragData;
        uint32_t vertData;
    } specializationData;

    specializationData data = {.vertData = 1, .fragData = graphicsPipeline->numTextures};

    // Map entry for the lighting model to be used by the fragment shader //optional
    VkSpecializationMapEntry specializationMapEntries[2];
    specializationMapEntries[0].constantID = 0;
    specializationMapEntries[0].size = sizeof(uint32_t);
    specializationMapEntries[0].offset = 0;

    // Map entry for the lighting model to be used by the fragment shader //optional
    specializationMapEntries[1].constantID = 1;
    specializationMapEntries[1].size = sizeof(uint32_t);
    specializationMapEntries[1].offset = offsetof(specializationData, fragData);

    VkSpecializationInfo specializationInfo = {};
    specializationInfo.dataSize = sizeof(specializationData);
    specializationInfo.pData = &data;
    specializationInfo.mapEntryCount = 2;
    specializationInfo.pMapEntries = specializationMapEntries; //if data is an array, we can use map entries to selectively map areas with offset/size

    VkPipelineShaderStageCreateInfo vertexShaderStage = {};
    vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStage.module = graphicsPipeline->m_vertShader.m_shaderModule;
    vertexShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    vertexShaderStage.pSpecializationInfo = &specializationInfo; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

    VkPipelineShaderStageCreateInfo fragmentShaderStage = {};
    fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStage.module = graphicsPipeline->m_fragShader.m_shaderModule;
    fragmentShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    fragmentShaderStage.pSpecializationInfo = &specializationInfo; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStage, fragmentShaderStage};

    VkVertexInputBindingDescription vertexInputBindingDescription;
    GetVertexBindingDescription(&vertexInputBindingDescription);

    VkVertexInputAttributeDescription vertexInputAttributeDescription[DATA_MEMBER_PER_VERTEX];
    GetAttributeDescriptions(vertexInputAttributeDescription);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;// TODO: To implement VertexBindingDescriptions
    vertexInputInfo.vertexAttributeDescriptionCount = DATA_MEMBER_PER_VERTEX;
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescription; // TODO: To Implement VertexAttributeDescriptions

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Input assembly ////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Viewport & Scissor ////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = graphicsPipeline->m_extent.height;
    viewport.width = graphicsPipeline->m_extent.width;
    viewport.height = -graphicsPipeline->m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = graphicsPipeline->m_extent;

    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport; //if we want the viewport to be immutable and not dynamic, we would create it and add it here
    viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor; //if we want the scissor to be immutable and not dynamic, we would create it and add it here

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Rasterizer ////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
    // This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature. Keep it false for now
    rasterizer.depthClampEnable = VK_FALSE;
    // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. This basically disables any output to the framebuffer.
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = graphicsPipeline->m_isWireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL; // using anything other than fill requires enabling a gpu feature (this can allow us to set line width and point size)
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE; // sometimes used for shadow mapping
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    rasterizer.lineWidth = 1.0f;


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Multi-sampling ////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // leave multi-sampling (for AA) disabled for now
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil = {}; // optional.

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Color blending ////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Pipeline Layout ///////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // //setup push constants
	VkPushConstantRange pushConstantInfo;
	pushConstantInfo.offset = 0; //this push constant range starts at the beginning
	pushConstantInfo.size = sizeof(MeshObjectPushConstant); //this push constant range takes up the size of a MeshPushConstants struct
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //this push constant range is accessible only in the vertex shader

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &graphicsPipeline->m_descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo; // Optional

    if (vkCreatePipelineLayout(nanoRenderer->m_pNanoContext->device, &pipelineLayoutInfo, NULL, &graphicsPipeline->m_pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "failed to create pipeline layout!");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = graphicsPipeline->m_pipelineLayout;
    pipelineInfo.renderPass = graphicsPipeline->_renderpass;
    pipelineInfo.subpass = 0; // subpass to use
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(nanoRenderer->m_pNanoContext->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline->m_pipeline) != VK_SUCCESS) {
        err = INVALID;
        fprintf(stderr, "failed to create graphics pipeline!");
        exit(0);
    }

    graphicsPipeline->m_isCompiled = true;

    return err;
}

void CleanUpGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){

    vkDeviceWaitIdle(nanoRenderer->m_pNanoContext->device);
    vkDestroySampler(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_sampler, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, graphicsPipeline->UniformBufferMemory[i].buffer, nullptr);
        vkFreeMemory(nanoRenderer->m_pNanoContext->device, graphicsPipeline->UniformBufferMemory[i].bufferMemory, nullptr);
        vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, graphicsPipeline->uniformBufferDynamicMemory[i].buffer, nullptr);
        vkFreeMemory(nanoRenderer->m_pNanoContext->device, graphicsPipeline->uniformBufferDynamicMemory[i].bufferMemory, nullptr);
    }

    //free(graphicsPipeline->uniformBufferDynamic.model);

    for(int i = 0; i < graphicsPipeline->numTextures; i++){
        CleanUpImageVkMemory(nanoRenderer, graphicsPipeline->textures[i]);
    }
    CleanUpImageVkMemory(nanoRenderer, &graphicsPipeline->defaultTexture);
    CleanUpImageVkMemory(nanoRenderer, &graphicsPipeline->emptyTexture);

    vkDestroyDescriptorPool(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_descriptorSetLayout, nullptr);
    vkDestroyPipeline(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_pipeline, NULL);
    vkDestroyPipelineLayout(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_pipelineLayout, NULL);
    CleanUpShader(nanoRenderer, &graphicsPipeline->m_vertShader);
    CleanUpShader(nanoRenderer, &graphicsPipeline->m_fragShader);
}
