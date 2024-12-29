#ifndef NANOSCENE_H_
#define NANOSCENE_H_

#include <stdint.h>
#include "NanoGraphicsPipeline.h"

struct NanoRenderer;
struct NanoEngine;
struct NanoImage;
struct RenderableObject;
struct FrameData;

typedef struct RenderableScene RenderableScene;
typedef struct RenderableNode RenderableNode;
typedef struct FrameData FrameData;

struct RenderableScene {
    struct RenderableObject* renderableObjects[256];
    uint32_t numRenderableObjects;

    struct NanoImage* textures[256];
    uint32_t numTextures;

    RenderableNode* rootNode;

    NanoGraphicsPipeline graphicsPipeline;
    int32_t ID;
};

struct RenderableNode{
    uint32_t NODE_ID;
    struct RenderableObject* renderableObject;

    mat4 localModel;

    RenderableNode* childNodes[16];
    uint32_t numChild;

    //update function
    void (*Update)(void* objectToUpdate, void* frameData);
};


void InitRenderableScene(struct NanoEngine* nanoEngine, RenderableScene* renderableScene);
void AddObjectToScene(struct RenderableObject* object, RenderableScene* renderableScene);
void AddRootNodeToScene(struct RenderableNode* rootNode, RenderableScene* renderableScene);
void PropagateNodeTransform(struct RenderableNode* rootNode);
void UpdateScene(RenderableScene* renderableScene, FrameData* data);
void CompileRenderableScene(RenderableScene* renderableScene);
void CleanUpScene(RenderableScene* renderableScene);

RenderableNode CreateRenderableNode(struct RenderableObject* renderableObj);
RenderableNode* AddChildRenderableNode(RenderableNode* renderableParent, RenderableNode* renderableChild);

#endif // NANOSCENE_H_
