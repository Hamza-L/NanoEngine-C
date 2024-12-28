#ifndef NANORENDERERPIPELINE_H_
#define NANORENDERERPIPELINE_H_

#include "NanoImage.h"
#include "NanoShader.h"
#include "NanoError.h"
#include "vulkan/vulkan_core.h"

typedef struct NanoGraphicsPipeline NanoGraphicsPipeline;

struct NanoGraphicsPipeline{
        VkRenderPass _renderpass;

        VkExtent2D m_extent;

        NanoShader m_vertShader;
        NanoShader m_fragShader;
        NanoShader* m_otherShaders; //optional

        VkSampler m_sampler;

        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorPool m_descriptorPool;
        VkDescriptorSet DescSets[MAX_FRAMES_IN_FLIGHT];

        bool m_isInitialized;
        bool m_isCompiled;

        // Textures
        NanoImage* textures[16];
        NanoImage defaultTexture;
        NanoImage emptyTexture;
        uint32_t numTextures;

        // Uniform Buffers
        UniformBufferObject uniformBuffer;
        NanoVkBufferMemory UniformBufferMemory[MAX_FRAMES_IN_FLIGHT];

        // dynamic uniform buffers
        uint32_t uniformBufferDynamicAllignment;
        NanoVkBufferMemory uniformBufferDynamicMemory[MAX_FRAMES_IN_FLIGHT];

        // optionals
        bool m_isWireFrame;
};

void InitGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const VkExtent2D extent);
void AddVertShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoShaderConfig config);
void AddFragShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoShaderConfig config);
void AddImageToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, NanoImage* nanoImage);
void UpdateGraphicsPipelineAtFrame(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, uint32_t currentFrame);
void UpdateGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);
void UpdateDescriptorSets(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);
void SetupDescriptors(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);
ERR CompileGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile);
void CleanUpGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);

#endif // NANORENDERERPIPELINE_H_
