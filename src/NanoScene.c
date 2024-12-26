#include "NanoScene.h"
#include "NanoError.h"
#include "NanoGraphicsPipeline.h"
#include "NanoImage.h"
#include "NanoEngine.h"
#include "cglm/mat4.h"

#include <string.h>
#include <stdio.h>

NanoEngine* s_NanoEngine;

void InitRenderableScene(NanoEngine* nanoEngine, RenderableScene* renderableScene){
    s_NanoEngine = nanoEngine;

    renderableScene->numRenderableObjects = 0;
    memset(renderableScene->renderableObjects, 0, sizeof(struct RenderableObject*)*256);
    renderableScene->numTextures = 0;
    memset(renderableScene->textures, 0, sizeof(struct NanoImage*)*256);
}

void AddObjectToScene(struct RenderableObject* object, RenderableScene* renderableScene){
    renderableScene->renderableObjects[renderableScene->numRenderableObjects++] = object;

    if(object->albedoTexture)
        renderableScene->textures[renderableScene->numTextures++] = object->albedoTexture;

    if(object->normalTexture)
        renderableScene->textures[renderableScene->numTextures++] = object->normalTexture;

    if(object->additionalTexture1)
        renderableScene->textures[renderableScene->numTextures++] = object->additionalTexture1;

    if(object->additionalTexture2)
        renderableScene->textures[renderableScene->numTextures++] = object->additionalTexture2;

}

void CompileRenderableScene(RenderableScene* renderableScene){
    //setup graphics pipeline
    // Setting up the image data of all the scene

    VkExtent2D extent = s_NanoEngine->m_Renderer.m_pNanoContext->swapchainContext.info.currentExtent;
    VkRenderPass renderpass = s_NanoEngine->m_Renderer.m_pNanoContext->defaultRenderpass;

    InitGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, extent);

    NanoShaderConfig vertConfig = {.m_fileFullPath = "./src/shader/shader.vert", .hasSampler = false, .hasUniformBuffer = true};
    AddVertShaderToGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, vertConfig);

    NanoShaderConfig fragConfig = {.m_fileFullPath = "./src/shader/shader.frag", .hasSampler = true, .hasUniformBuffer = false};
    AddFragShaderToGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, fragConfig);

    renderableScene->graphicsPipeline._renderpass = renderpass;

    // this needs to be called first so the graphics pipeline is compiled with the right number of texture updates
    for(int i = 0; i < renderableScene->numTextures; i++){
        if(renderableScene->textures[i]){
            SubmitImageToGPUMemory(&s_NanoEngine->m_Renderer, renderableScene->textures[i]);
            AddImageToGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, renderableScene->textures[i]);
        }
    }

    SetupDescriptors(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline);

    CompileGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, true);

    if(!renderableScene->graphicsPipeline.m_isInitialized){
        fprintf(stderr, "Failed to Initialize graphics pipeline for current scene\n");
        DEBUG_BREAK;
    }


    UpdateDescriptorSets(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline);

    // Setting up the mesh data of all the scene
    SendAllocatedMeshMemoryToGPUMemory(&s_NanoEngine->m_Renderer, &s_NanoEngine->m_meshMemory);
}

void CleanUpScene(RenderableScene* renderableScene){
    CleanUpGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline);
}
