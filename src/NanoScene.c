#include "NanoScene.h"
#include "NanoConfig.h"
#include "NanoGraphicsPipeline.h"
#include "NanoEngine.h"
#include "NanoRenderer.h"
#include "cglm/mat4.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

NanoEngine* s_NanoEngine;
static uint32_t s_numNodes;

void InitRenderableScene(NanoEngine* nanoEngine, RenderableScene* renderableScene){
    s_NanoEngine = nanoEngine;
    renderableScene->numRenderableObjects = 0;
    memset(renderableScene->renderableObjects, 0, sizeof(struct RenderableObject*)*256);
    renderableScene->numTextures = 0;
    memset(renderableScene->textures, 0, sizeof(struct NanoImage*)*256);
}

void AddObjectToScene(struct RenderableObject* object, RenderableScene* renderableScene){

    object->ID = renderableScene->numRenderableObjects;
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

void AddRootNodeToScene(struct RenderableNode* rootNode, RenderableScene* renderableScene){
    if(rootNode == nullptr){
        fprintf(stderr, "rootNode to add to scene is null\n");
        return;
    }

    renderableScene->rootNode = rootNode;
    AddObjectToScene(rootNode->renderableObject, renderableScene);

    RenderableNode* queue[MAX_OBJECT_PER_SCENE] = {nullptr};

    int queueSize = 1; // we know the root node is not null so size is 1
    int currHead = 0;
    RenderableNode* currNode = queue[currHead] = rootNode;

    while(currNode){
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            AddObjectToScene(currChildNode->renderableObject, renderableScene);
            queue[queueSize++] = currNode->childNodes[i];
        }
        currHead++;
        currNode = queue[currHead];
    }
}

void UpdateScene(RenderableScene* renderableScene, FrameData* data){
    RenderableNode* queue[MAX_OBJECT_PER_SCENE] = {nullptr};

    int queueSize = 1; // we know the root node is not null so size is 1
    int currHead = 0;
    mat4 currTransform = {0};
    RenderableNode* currNode = queue[currHead] = renderableScene->rootNode;
    if(currNode->Update) {
        currNode->Update(currNode, data);
    }

    glm_mat4_copy(currNode->localModel, currNode->renderableObject->model);
    uint32_t memOffset = renderableScene->graphicsPipeline.uniformBufferDynamicAllignment;
    mat4* modelMemDest = (mat4*)(renderableScene->graphicsPipeline.uniformBufferDynamicMemory[data->currentFrame].bufferMemoryMapped + (currNode->renderableObject->ID * memOffset));
    memcpy(modelMemDest, &currNode->renderableObject->model, sizeof(mat4));

    while(currNode){
        glm_mat4_copy(currNode->renderableObject->model, currTransform);
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            RenderableObject* obj = currChildNode->renderableObject;

            if(currChildNode->Update){
                currChildNode->Update(currChildNode, data);
            }

            glm_mat4_copy(currChildNode->localModel, obj->model);
            glm_mat4_mul(currTransform, obj->model, obj->model);

            uint32_t memOffset = renderableScene->graphicsPipeline.uniformBufferDynamicAllignment;
            mat4* modelMemDest = (mat4*)(renderableScene->graphicsPipeline.uniformBufferDynamicMemory[data->currentFrame].bufferMemoryMapped + (obj->ID * memOffset));
            memcpy(modelMemDest, &obj->model, sizeof(mat4));

        }
        currHead++;
        currNode = queue[currHead];
    }
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

RenderableNode CreateRenderableNode(struct RenderableObject* renderableObj){
    RenderableNode node = {};
    node.renderableObject = renderableObj;
    node.NODE_ID = s_numNodes;
    node.numChild = 0;
    glm_mat4_identity(node.localModel);

    return node;
}

RenderableNode* AddChildRenderableNode(RenderableNode* renderableParent, RenderableNode* renderableChild){
    renderableParent->childNodes[renderableParent->numChild++] = renderableChild;
    glm_mat4_mul(renderableParent->renderableObject->model, renderableChild->renderableObject->model, renderableChild->renderableObject->model);
    return renderableParent;
}

void PropagateNodeTransform(struct RenderableNode* rootNode){
    RenderableNode* queue[MAX_OBJECT_PER_SCENE] = {nullptr};

    if(rootNode == nullptr){
        fprintf(stderr, "rootNode to propagate is null\n");
        return;
    }

    int queueSize = 1; // we know the root node is not null so size is 1
    int currHead = 0;
    mat4 currTransform = {0};
    RenderableNode* currNode = queue[currHead] = rootNode;
    glm_mat4_copy(rootNode->localModel, rootNode->renderableObject->model);

    while(currNode){
        glm_mat4_copy(currNode->renderableObject->model, currTransform);
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            glm_mat4_copy(currChildNode->localModel, currChildNode->renderableObject->model);
            glm_mat4_mul(currTransform, currChildNode->renderableObject->model, currChildNode->renderableObject->model);
            queue[queueSize++] = currNode->childNodes[i];
        }
        currHead++;
        currNode = queue[currHead];
    }
}
