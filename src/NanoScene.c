#include "NanoScene.h"
#include "NanoConfig.h"
#include "NanoGraphicsPipeline.h"
#include "NanoEngine.h"
#include "NanoRenderer.h"
#include "NanoUtility.h"
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
    //if the object contains no geometry, we do not add it to the scene
    if(object->meshObject.vertexMemSize == 0 || object->meshObject.indexMemSize == 0){
        return;
    }

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
        LOG_MSG(stderr, "rootNode to add to scene is null\n");
        return;
    }

    renderableScene->rootNode = rootNode;
    if(rootNode->renderableObject.meshObject.vertexMemSize > 0 && rootNode->renderableObject.meshObject.indexMemSize > 0){
        AddObjectToScene(&rootNode->renderableObject, renderableScene);
    }

    RenderableNode* queue[MAX_OBJECT_PER_SCENE] = {nullptr};

    int queueSize = 1; // we know the root node is not null so size is 1
    int currHead = 0;
    RenderableNode* currNode = queue[currHead] = rootNode;

    while(currNode){
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            AddObjectToScene(&currChildNode->renderableObject, renderableScene);
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

    glm_mat4_copy(currNode->localModel, currNode->renderableObject.model);
    uint32_t memOffset = renderableScene->graphicsPipeline.uniformBufferDynamicAllignment;
    mat4* modelMemDest = (mat4*)(renderableScene->graphicsPipeline.uniformBufferDynamicMemory[data->currentFrame].bufferMemoryMapped + (currNode->renderableObject.ID * memOffset));
    memcpy(modelMemDest, &currNode->renderableObject.model, sizeof(mat4));

    while(currNode){
        glm_mat4_copy(currNode->renderableObject.model, currTransform);
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            RenderableObject* obj = &currChildNode->renderableObject;

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

    NanoShaderConfig vertConfig = {.m_sourcefileFullPath = PrependCWD("../src/shader-code/shader.vert"),
                                   .m_binaryfileFullPath = PrependCWD("Shaders/vert_shader.spv"),
                                   .hasSampler = false,
                                   .hasUniformBuffer = true};
    AddVertShaderToGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, vertConfig);

    NanoShaderConfig fragConfig = {.m_sourcefileFullPath = PrependCWD("../src/shader-code/shader.frag"),
                                   .m_binaryfileFullPath = PrependCWD("Shaders/frag_shader.spv"),
                                   .hasSampler = false,
                                   .hasUniformBuffer = true};
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

    CompileGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline, FORCE_RECOMPILE);

    if(!renderableScene->graphicsPipeline.m_isInitialized){
        LOG_MSG(stderr, "Failed to Initialize graphics pipeline for current scene\n");
        DEBUG_BREAK;
    }


    UpdateDescriptorSets(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline);

    // Setting up the mesh data of all the scene
    SendAllocatedMeshMemoryToGPUMemory(&s_NanoEngine->m_Renderer, &s_NanoEngine->m_meshMemory);
}

void CleanUpScene(RenderableScene* renderableScene){
    CleanUpGraphicsPipeline(&s_NanoEngine->m_Renderer, &renderableScene->graphicsPipeline);
}

RenderableNode* AddChildRenderableNode(RenderableNode* renderableParent, RenderableNode* renderableChild){
    renderableParent->childNodes[renderableParent->numChild++] = renderableChild;
    glm_mat4_mul(renderableParent->renderableObject.model, renderableChild->renderableObject.model, renderableChild->renderableObject.model);
    return renderableParent;
}

void PropagateNodeTransform(struct RenderableNode* rootNode){
    RenderableNode* queue[MAX_OBJECT_PER_SCENE] = {nullptr};

    if(rootNode == nullptr){
        LOG_MSG(stderr, "rootNode to propagate is null\n");
        return;
    }

    int queueSize = 1; // we know the root node is not null so size is 1
    int currHead = 0;
    mat4 currTransform = {0};
    RenderableNode* currNode = queue[currHead] = rootNode;
    glm_mat4_copy(rootNode->localModel, rootNode->renderableObject.model);

    while(currNode){
        glm_mat4_copy(currNode->renderableObject.model, currTransform);
        for(int i = 0; i < currNode->numChild; i++){
            RenderableNode* currChildNode = currNode->childNodes[i];
            glm_mat4_copy(currChildNode->localModel, currChildNode->renderableObject.model);
            glm_mat4_mul(currTransform, currChildNode->renderableObject.model, currChildNode->renderableObject.model);
            queue[queueSize++] = currNode->childNodes[i];
        }
        currHead++;
        currNode = queue[currHead];
    }
}

void MakeTriangle(Vertex* vertices, uint32_t* numVertices, uint32_t* indices, uint32_t* numIndices, TriangleParam* param){
    *numVertices = 3;
    *numIndices = 3;
    if (vertices == nullptr || indices == nullptr){
        return;
    }

    float color[4] = {};
    for(int i = 0; i < 4; i++)
        color[i] = param->color[i];
}

void MakeSquare(Vertex* vertices, uint32_t* numVertices, uint32_t* indices, uint32_t* numIndices, SquareParam* param){
    *numVertices = 4;
    *numIndices = 6;
    if (vertices == nullptr || indices == nullptr){
        return;
    }

    float color[4] = {};
    for(int i = 0; i < 4; i++)
        color[i] = param->color[i];

    Vertex verticesTemp[4] = {{{ param->position[0]               , param->position[1] - param->height, param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
                              {{ param->position[0] + param->width, param->position[1] - param->height, param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
                              {{ param->position[0] + param->width, param->position[1]                , param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
                              {{ param->position[0]               , param->position[1]                , param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}}};

    uint32_t indicesTemp[6] = { 0, 1, 2, 2, 3, 0 };

    memcpy(vertices, verticesTemp, *numVertices * sizeof(Vertex));
    memcpy(indices, indicesTemp, *numIndices * sizeof(uint32_t));
}

void MakeCube(Vertex* vertices, uint32_t* numVertices, uint32_t* indices, uint32_t* numIndices, CubeParam* param){
    *numVertices = 4;
    *numIndices = 6;
    if (vertices == nullptr || indices == nullptr){
        return;
    }

    float color[4] = {};
    for(int i = 0; i < 4; i++)
        color[i] = param->color[i];

    Vertex verticesTemp[4] = {{{ param->position[0]               , param->position[1] - param->height, param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
                              {{ param->position[0] + param->width, param->position[1] - param->height, param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
                              {{ param->position[0] + param->width, param->position[1]                , param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
                              {{ param->position[0]               , param->position[1]                , param->position[2]}, {0.0f,0.0f,1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}}};

    uint32_t indicesTemp[6] = { 0, 1, 2, 2, 3, 0 };

    memcpy(vertices, verticesTemp, *numVertices * sizeof(Vertex));
    memcpy(indices, indicesTemp, *numIndices * sizeof(uint32_t));
}

void MakeSphere(Vertex* vertices, uint32_t* numVertices, uint32_t* indices, uint32_t* numIndices, SphereParam* param){

    *numVertices = 12;
    *numIndices = 60;
    if (vertices == nullptr || indices == nullptr){
        return;
    }

    float color[4] = {};
    for(int i = 0; i < 4; i++)
        color[i] = param->color[i];

    float phi = (1 + sqrt(5.0f)) / 2.0f;
    Vertex v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11;
    Vertex verticesTemp[12] = {
        {{-1.0f,  phi, 0.0f}, {-1.0f,  phi, 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
        {{ 1.0f,  phi, 0.0f}, { 1.0f,  phi, 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
        {{-1.0f, -phi, 0.0f}, {-1.0f, -phi, 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
        {{ 1.0f, -phi, 0.0f}, { 1.0f, -phi, 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}},

        {{0.0f, -1.0f,  phi}, {0.0f, -1.0f,  phi}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
        {{0.0f,  1.0f,  phi}, {0.0f,  1.0f,  phi}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
        {{0.0f, -1.0f, -phi}, {0.0f, -1.0f, -phi}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
        {{0.0f,  1.0f, -phi}, {0.0f,  1.0f, -phi}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}},

        {{ phi, 0.0f, -1.0f}, { phi, 0.0f, -1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
        {{ phi, 0.0f,  1.0f}, { phi, 0.0f,  1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
        {{-phi, 0.0f, -1.0f}, {-phi, 0.0f, -1.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
        {{-phi, 0.0f,  1.0f}, {-phi, 0.0f,  1.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}},
    };

    uint32_t indicesTemp[60] =  {0, 11,  5,
                                 0,  5,  1,
                                 0,  1,  7,
                                 0,  7, 10,
                                 0, 10, 11,

                                 1,  5,  9,
                                 5, 11,  4,
                                 11, 10,  2,
                                 10,  7,  6,
                                 7,  1,  8,

                                 3,  9,  4,
                                 3,  4,  2,
                                 3,  2,  6,
                                 3,  6,  8,
                                 3,  8,  9,

                                 4,  9,  5,
                                 2,  4, 11,
                                 6,  2, 10,
                                 8,  6,  7,
                                 9,  8,  1};


    memcpy(vertices, verticesTemp, *numVertices * sizeof(Vertex));
    memcpy(indices, indicesTemp, *numIndices * sizeof(uint32_t));
        /* subdivide(10); */
}

void InitRenderableObject(Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices, RenderableObject* renderableObject){
    AllocateMeshMemoryObject(&s_NanoEngine->m_meshMemory.meshHostMemory, vertices, numVertices, indices, numIndices, &renderableObject->meshObject);
    renderableObject->ID = -1; //current meshObject index
    renderableObject->albedoTexture = nullptr;
    renderableObject->normalTexture = nullptr;
    renderableObject->additionalTexture1 = nullptr;
    renderableObject->additionalTexture2 = nullptr;
    renderableObject->isVisible = true;
    glm_mat4_identity(renderableObject->model);
}


RenderableObject CreateRenderableObject(Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices){
    RenderableObject object = {};
    InitRenderableObject(vertices, numVertices, indices, numIndices, &object);
    return object;
}

RenderableObject CreateRenderableObjectFromPrimitive(Primitive primType, void* primParam){
    RenderableObject object = {};
    uint32_t numVertex = 0;
    uint32_t numIndices = 0;
    Vertex* vertices = nullptr;
    uint32_t* indices = nullptr;
    switch (primType) {
        case TRIANGLE:
            MakeTriangle(nullptr, &numVertex, nullptr, &numIndices, (TriangleParam*)primParam);
            vertices = (Vertex*)malloc(numVertex * sizeof(Vertex));
            indices = (uint32_t*)malloc(numIndices * sizeof(uint32_t));
            MakeTriangle(vertices, &numVertex, indices, &numIndices, (TriangleParam*)primParam);
            break;
        case SQUARE:
            MakeSquare(nullptr, &numVertex, nullptr, &numIndices, (SquareParam*)primParam);
            vertices = (Vertex*)malloc(numVertex * sizeof(Vertex));
            indices = (uint32_t*)malloc(numIndices * sizeof(uint32_t));
            MakeSquare(vertices, &numVertex, indices, &numIndices, (SquareParam*)primParam);
            break;
        case CUBE:
            MakeCube(nullptr, &numVertex, nullptr, &numIndices, (CubeParam*)primParam);
            vertices = (Vertex*)malloc(numVertex * sizeof(Vertex));
            indices = (uint32_t*)malloc(numIndices * sizeof(uint32_t));
            MakeCube(vertices, &numVertex, indices, &numIndices, (CubeParam*)primParam);
            break;
        case SPHERE:
            MakeSphere(nullptr, &numVertex, nullptr, &numIndices, (SphereParam*)primParam);
            vertices = (Vertex*)malloc(numVertex * sizeof(Vertex));
            indices = (uint32_t*)malloc(numIndices * sizeof(uint32_t));
            MakeSphere(vertices, &numVertex, indices, &numIndices, (SphereParam*)primParam);
            break;
        default:
            LOG_MSG(stderr, "Invalid primitive type\n");
            break;
            }
    InitRenderableObject(vertices, numVertex, indices, numIndices, &object);
    return object;
}

RenderableObject CreateRenderableObjectFromFile(const char* fileName){
    RenderableObject object = {};
    return object;
}

RenderableNode CreateRenderableNode(Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices){
    RenderableNode node = {};
    node.renderableObject = CreateRenderableObject(vertices, numVertices, indices, numIndices);
    node.NODE_ID = s_numNodes;
    node.numChild = 0;
    glm_mat4_identity(node.localModel);
    return node;
}

RenderableNode CreateRenderableNodeFromPrimitive(Primitive primType, void* primParam){
    RenderableNode node = {};
    node.renderableObject = CreateRenderableObjectFromPrimitive(primType, primParam);
    node.NODE_ID = s_numNodes;
    node.numChild = 0;
    glm_mat4_identity(node.localModel);
    return node;
}

RenderableNode CreateRenderableNodeFromFile(const char* fileName){
    RenderableNode node = {};
    node.renderableObject = CreateRenderableObjectFromFile(fileName);
    node.NODE_ID = s_numNodes;
    node.numChild = 0;
    glm_mat4_identity(node.localModel);
    return node;
}

RenderableNode CreateEmptyRenderableNode(){
    RenderableNode node = {};
    node.NODE_ID = s_numNodes;
    node.numChild = 0;
    glm_mat4_identity(node.localModel);
    return node;
}
