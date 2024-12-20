#include "NanoGraphicsPipeline.h"
#include "NanoShader.h"
#include "NanoBuffers.h"
#include "NanoRenderer.h"
#include "cglm/cam.h"

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

static void UpdateDescriptorSets(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkWriteDescriptorSet descriptorWrites[2] = {};
        uint32_t numDescriptorUpdates = 1;

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

        // combined image sampler descriptor set update
        if(graphicsPipeline->numTextures > 0){
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = graphicsPipeline->textures->imageView; //For now we use the same image for all in flight frames. We also use only the first image
            imageInfo.sampler = graphicsPipeline->m_sampler;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = graphicsPipeline->DescSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // Optional

            numDescriptorUpdates++;
        }
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
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1; //could pass an array of uniform buffer objects
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional. used for texture sampling

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[2] = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(nanoRenderer->m_pNanoContext->device, &layoutInfo, nullptr, &graphicsPipeline->m_descriptorSetLayout) != VK_SUCCESS) {
      fprintf(stderr, "failed to create descriptor set layout!");
      ASSERT(false, "failed to create descriptor set layout!");
    }
}

static void CreateDescriptorPool(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    VkDescriptorPoolSize poolSize[2] = {};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = (uint32_t)(MAX_FRAMES_IN_FLIGHT + 1);
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = (uint32_t)(MAX_FRAMES_IN_FLIGHT + 1);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)2;
    poolInfo.pPoolSizes = poolSize;
    poolInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(nanoRenderer->m_pNanoContext->device, &poolInfo, nullptr, &graphicsPipeline->m_descriptorPool) != VK_SUCCESS) {
      fprintf(stderr, "failed to create descriptor pool!\n");
    }

}

void AddImageToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoImage* nanoImage){
    graphicsPipeline->textures = nanoImage;
    graphicsPipeline->numTextures++;

    UpdateDescriptorSets(nanoRenderer, graphicsPipeline);
}


void InitGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const VkExtent2D extent){
    graphicsPipeline->m_extent = extent;
    graphicsPipeline->m_isInitialized = true;

    glm_mat4_identity(graphicsPipeline->uniformBuffer.model);
    glm_mat4_identity(graphicsPipeline->uniformBuffer.proj);
    glm_mat4_identity(graphicsPipeline->uniformBuffer.view);

    // does not use a staging buffer since it's updated at every frame
    CreateUniformBuffersWithMappedMem(nanoRenderer, graphicsPipeline->UniformBufferMemory, MAX_FRAMES_IN_FLIGHT);

    CreateTextureSampler(nanoRenderer, graphicsPipeline);

    CreateDescriptorSetLayout(nanoRenderer, graphicsPipeline);
    CreateDescriptorPool(nanoRenderer, graphicsPipeline);
    CreateDescriptorSets(nanoRenderer, graphicsPipeline);

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
    vec3 eye = {0.0f, 0.0f, 2.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(eye, center, up, ubo->view);

    // Projection matrix
    glm_perspective(glm_rad(45.0f), graphicsPipeline->m_extent.width / (float)graphicsPipeline->m_extent.height, 0.1f, 10.0f, ubo->proj);
    ubo->proj[1][1] *= -1;
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

}

void UpdateGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline){
    UniformBufferObject ubo = {};
    glm_mat4_identity(ubo.model);
    /* vec3 axis = {0.0f, 1.0f, 0.0f}; */
    /* glm_mat4_identity(ubo.model); */
    /* glm_rotate(ubo.model, 45.0f, axis); */

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        memcpy(graphicsPipeline->UniformBufferMemory[i].bufferMemoryMapped, &ubo, sizeof(ubo));
    }

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

    VkPipelineShaderStageCreateInfo vertexShaderStage = {};
    vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStage.module = graphicsPipeline->m_vertShader.m_shaderModule;
    vertexShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    vertexShaderStage.pSpecializationInfo = NULL; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

    VkPipelineShaderStageCreateInfo fragmentShaderStage = {};
    fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStage.module = graphicsPipeline->m_fragShader.m_shaderModule;
    fragmentShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    fragmentShaderStage.pSpecializationInfo = NULL; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

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
    viewport.y = 0.0f;
    viewport.width = graphicsPipeline->m_extent.width;
    viewport.height = graphicsPipeline->m_extent.height;
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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &graphicsPipeline->m_descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional

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
    vkDestroySampler(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_sampler, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(nanoRenderer->m_pNanoContext->device, graphicsPipeline->UniformBufferMemory[i].buffer, nullptr);
        vkFreeMemory(nanoRenderer->m_pNanoContext->device, graphicsPipeline->UniformBufferMemory[i].bufferMemory, nullptr);
    }

    vkDestroyDescriptorPool(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_descriptorSetLayout, nullptr);
    vkDestroyPipeline(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_pipeline, NULL);
    vkDestroyPipelineLayout(nanoRenderer->m_pNanoContext->device, graphicsPipeline->m_pipelineLayout, NULL);
    CleanUpShader(nanoRenderer, &graphicsPipeline->m_vertShader);
    CleanUpShader(nanoRenderer, &graphicsPipeline->m_fragShader);
}
