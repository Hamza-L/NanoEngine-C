#ifndef NANOSHADER_H_
#define NANOSHADER_H_

#include "NanoBuffers.h"
#include "NanoConfig.h"
#include "Str.h"

#include "vulkan/vulkan_core.h"

typedef struct NanoShader NanoShader;

typedef struct {
        String m_fileFullPath;
} NanoShaderConfig;

struct NanoShader{
        String m_fileFullPath;
        char* m_rawShaderCode;
        uint32_t m_rawShaderCodeSize;
        bool m_isCompiled;

        VkShaderModule m_shaderModule;

        NanoVkBufferMemory UniformBufferMemory[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSet UniformBufferDescSets[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorSetLayout descriptorSetLayout;

        VkDescriptorPool descriptorPool;
};


void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile);
void UpdateShader(NanoShader* shaderToInitialize, uint32_t currentFrame);
int CompileShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCompile, bool forceCompile);
void InitVertexShaderUniformBuffers(NanoRenderer* nanoRenderer, NanoShader* shaderToInit);
void CleanUpShader(NanoRenderer* nanoRenderer, NanoShader* shaderToCleanUp);

#endif // NANOSHADER_H_
