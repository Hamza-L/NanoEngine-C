#ifndef NANOSHADER_H_
#define NANOSHADER_H_

#include "NanoConfig.h"
#include "Str.h"

#include "vulkan/vulkan_core.h"

typedef struct NanoShader NanoShader;
typedef struct NanoGraphics NanoGraphics;

struct NanoShader{
        String m_fileFullPath;
        char* m_rawShaderCode;
        uint32_t m_rawShaderCodeSize;
        bool m_isCompiled;

        VkShaderModule m_shaderModule;
};

struct NanoGraphics;

void InitShader(NanoShader* shaderToInitialize, const char* shaderCodeFile);
int CompileShader(NanoGraphics* nanoGraphics, NanoShader* shaderToCompile, bool forceCompile);
void CleanUpShader(NanoGraphics* nanoGraphics, NanoShader* shaderToCleanUp);

#endif // NANOSHADER_H_
