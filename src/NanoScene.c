#include "NanoScene.h"
#include "NanoGraphicsPipeline.h"
#include "NanoImage.h"
#include "NanoEngine.h"
#include <string.h>

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
    int graphicsPipelineIndx = s_NanoEngine->m_Renderer.m_pNanoContext->currentGraphicsPipeline;

    // Setting up the image data of all the scene
    for(int i = 0; i < renderableScene->numTextures; i++){
        if(renderableScene->textures[i]){
            SubmitImageToGPUMemory(&s_NanoEngine->m_Renderer, renderableScene->textures[i]);
            AddImageToGraphicsPipeline(&s_NanoEngine->m_Renderer, &s_NanoEngine->m_Renderer.m_pNanoContext->graphicsPipelines[graphicsPipelineIndx], renderableScene->textures[i]);
        }
    }

    // Setting up the mesh data of all the scene
    SendAllocatedMeshMemoryToGPUMemory(&s_NanoEngine->m_Renderer, &s_NanoEngine->m_meshMemory);
}
