#ifndef NANOSHADER_H_
#define NANOSHADER_H_

#include "NanoConfig.h"
#include "Str.h"

#include "vulkan/vulkan_core.h"

typedef struct NanoShader NanoShader;
typedef struct NanoRenderer NanoRenderer;

typedef struct {
        String m_fileFullPath;
} NanoShaderConfig;

struct NanoShader{
        String m_fileFullPath;
        char* m_rawShaderCode;
        uint32_t m_rawShaderCodeSize;
        bool m_isCompiled;

        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkShaderModule m_shaderModule;
};

struct NanoRenderer;

void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile);
int CompileShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCompile, bool forceCompile);
void CleanUpShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCleanUp);

#endif // NANOSHADER_H_
