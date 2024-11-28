#ifndef NANORENDERERPIPELINE_H_
#define NANORENDERERPIPELINE_H_

#include "NanoShader.h"
#include "NanoError.h"
#include "vulkan/vulkan_core.h"

typedef struct NanoGraphicsPipeline NanoGraphicsPipeline;

struct NanoGraphicsPipeline{
        VkDevice _device;
        VkRenderPass _renderpass;
        VkExtent2D m_extent;
        NanoShader m_vertShader;
        NanoShader m_fragShader;
        NanoShader* m_otherShaders; //optional
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;
        bool m_isInitialized;
        bool m_isCompiled;

        // optionals
        bool m_isWireFrame;
};

void InitGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const VkExtent2D extent);
void AddVertShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const char* vertShaderFile);
void AddFragShaderToGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, const char* fragShaderFile);
void UpdateGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, uint32_t currentFrame);
ERR CompileGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline, bool forceReCompile);
void CleanUpGraphicsPipeline(NanoRenderer* nanoRenderer, NanoGraphicsPipeline* graphicsPipeline);

#endif // NANORENDERERPIPELINE_H_
