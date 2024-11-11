#ifndef NANOSHADER_H_
#define NANOSHADER_H_

#include "NanoError.h"
#include "NanoConfig.h"

#include "vulkan/vulkan_core.h"

typedef struct NanoShader NanoShader;

struct NanoShader{
        char m_fileFullPath[MAX_FILEPATH_LENGTH];
        char* m_rawShaderCode;
        uint32_t m_rawShaderCodeSize;
        bool m_isCompiled;

        VkShaderModule m_shaderModule;
};

void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile);
int CompileShader(NanoShader* shaderToInitialize, bool forceCompile);
void CleanUpShader(NanoShader* shaderToInitialize);

#endif // NANOSHADER_H_
