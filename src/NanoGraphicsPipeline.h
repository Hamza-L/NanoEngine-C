#ifndef NANOGRAPHICSPIPELINE_H_
#define NANOGRAPHICSPIPELINE_H_

#include "NanoShader.h"
#include "vulkan/vulkan_core.h"

typedef struct NanoGraphicsPipeline NanoGraphicsPipeline;

struct NanoGraphicsPipeline{
        VkDevice _device;
        VkRenderPass _renderpass;
        VkExtent2D m_extent;
        NanoShader* m_vertShader;
        NanoShader* m_fragShader;
        NanoShader* m_otherShaders; //optional
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;
};

void InitGraphicsPipeline(NanoGraphicsPipeline* graphicsPipeline, VkDevice device, const VkExtent2D extent);
void AddVertShaderToNGPipeline(NanoGraphicsPipeline* graphicsPipeline, const char* vertShaderFile);
void AddFragShaderToNGPipeline(NanoGraphicsPipeline* graphicsPipeline, const char* fragShaderFile);
ERR CompileNGPipeline(NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile);
void CleanUpGraphicsPipeline(NanoGraphicsPipeline* graphicsPipeline);

#endif // NANOGRAPHICSPIPELINE_H_
