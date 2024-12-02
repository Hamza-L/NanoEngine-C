#ifndef NANOSHADER_H_
#define NANOSHADER_H_

#include "NanoBuffers.h"
#include "NanoConfig.h"
#include "NanoImage.h"
#include "Str.h"

#include "vulkan/vulkan_core.h"

typedef struct NanoShader NanoShader;

typedef struct {
        char m_fileFullPath[512];
        bool hasSampler;
        bool hasUniformBuffer;
} NanoShaderConfig;

struct NanoShader{
        NanoShaderConfig config;
        char* m_rawShaderCode;
        uint32_t m_rawShaderCodeSize;
        bool m_isCompiled;

        VkShaderModule m_shaderModule;

};

void InitShader(NanoRenderer* nanoRenderer, NanoShader* shaderToInitialize, NanoShaderConfig config);
int CompileShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCompile, bool forceCompile);
void BindUniformBuffer(NanoRenderer* nanoRenderer, NanoShader* nanoShader);
void CleanUpShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCleanUp);

#endif // NANOSHADER_H_
