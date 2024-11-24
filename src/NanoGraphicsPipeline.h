#ifndef NANORENDERERPIPELINE_H_
#define NANORENDERERPIPELINE_H_

#include "NanoShader.h"
#include "NanoError.h"
#include "vulkan/vulkan_core.h"

typedef struct NanoRendererPipeline NanoGraphicsPipeline;

struct NanoRendererPipeline{
        VkDevice _device;
        VkRenderPass _renderpass;
        VkExtent2D m_extent;
        NanoShader* m_vertShader;
        NanoShader* m_fragShader;
        NanoShader* m_otherShaders; //optional
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;
        bool m_isInitialized;
        bool m_isCompiled;
};

void InitGraphicsPipeline(NanoGraphicsPipeline* graphicsPipeline, VkDevice device, const VkExtent2D extent);
void AddVertShaderToNGPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const char* vertShaderFile);
void AddFragShaderToNGPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const char* fragShaderFile);
ERR CompileNGPipeline(NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile);
void CleanUpGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);

#endif // NANORENDERERPIPELINE_H_
