#include "NanoGraphicsPipeline.h"
#include "NanoShader.h"
#include "NanoBuffers.h"
#include "vulkan/vulkan_core.h"
#include <stdlib.h>
#include <stdio.h>

//TODO ; Match the init design of the NanoGraphics class
void InitGraphicsPipeline(NanoGraphicsPipeline* graphicsPipeline, VkDevice device, const VkExtent2D extent){
    graphicsPipeline->_device = device;
    graphicsPipeline->m_extent = extent;
    graphicsPipeline->m_isInitialized = true;
}

void AddVertShaderToNGPipeline(NanoRenderer* nanoGraphics, NanoGraphicsPipeline* graphicsPipeline, const char* vertShaderFile){
    NanoShader* vertShader = (NanoShader*)calloc(1, sizeof(NanoShader));
    InitShader(vertShader, vertShaderFile);
    CompileShader(nanoGraphics, vertShader, FORCE_RECOMPILE);
    graphicsPipeline->m_vertShader = vertShader;
}

void AddFragShaderToNGPipeline(NanoRenderer* nanoGraphics, NanoGraphicsPipeline* graphicsPipeline, const char* fragShaderFile){
    NanoShader* fragShader = (NanoShader*)calloc(1, sizeof(NanoShader));
    InitShader(fragShader, fragShaderFile);
    CompileShader(nanoGraphics, fragShader, FORCE_RECOMPILE);
    graphicsPipeline->m_fragShader = fragShader;
}

ERR CompileNGPipeline(NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile){
    ERR err = OK;

    NanoShader* vertShader = graphicsPipeline->m_vertShader;
    if(!vertShader->m_isCompiled){
        ASSERT(vertShader->m_isCompiled, "graphics pipeline's vertex shader was not compiled\n");
        return NOT_INITIALIZED;
    }

    NanoShader* fragShader = graphicsPipeline->m_fragShader;
    if(!fragShader->m_isCompiled){
        ASSERT(fragShader->m_isCompiled, "graphics pipeline's fragment shader was not compiled\n");
        return NOT_INITIALIZED;
    }

    // Shaders ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkPipelineShaderStageCreateInfo vertexShaderStage = {};
    vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStage.module = vertShader->m_shaderModule;
    vertexShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    vertexShaderStage.pSpecializationInfo = NULL; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

    VkPipelineShaderStageCreateInfo fragmentShaderStage = {};
    fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStage.module = fragShader->m_shaderModule;
    fragmentShaderStage.pName = "main";
    // VkSpecializationInfo specializationInfo = {};
    fragmentShaderStage.pSpecializationInfo = NULL; //Can specify different values for constant used in this shader. allows better optimization at shader creation stage

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStage, fragmentShaderStage};

    VkVertexInputBindingDescription vertexInputBindingDescription;
    GetVertexBindingDescription(&vertexInputBindingDescription);

    VkVertexInputAttributeDescription vertexInputAttributeDescription[DATA_PER_VERTEX];
    GetAttributeDescriptions(vertexInputAttributeDescription);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;// TODO: To implement VertexBindingDescriptions
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
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
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // using anything other than fill requires enabling a gpu feature (this can allow us to set line width and point size)
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = NULL; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional

    if (vkCreatePipelineLayout(graphicsPipeline->_device, &pipelineLayoutInfo, NULL, &graphicsPipeline->m_pipelineLayout) != VK_SUCCESS) {
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

    if (vkCreateGraphicsPipelines(graphicsPipeline->_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline->m_pipeline) != VK_SUCCESS) {
        err = INVALID;
        fprintf(stderr, "failed to create graphics pipeline!");
        exit(0);
    }

    graphicsPipeline->m_isCompiled = true;

    return err;
}

void CleanUpGraphicsPipeline(NanoRenderer* nanoGraphics, NanoGraphicsPipeline* graphicsPipeline){
    vkDestroyPipeline(graphicsPipeline->_device, graphicsPipeline->m_pipeline, NULL);
    vkDestroyPipelineLayout(graphicsPipeline->_device, graphicsPipeline->m_pipelineLayout, NULL);
    CleanUpShader(nanoGraphics, graphicsPipeline->m_vertShader);
    CleanUpShader(nanoGraphics, graphicsPipeline->m_fragShader);
}
