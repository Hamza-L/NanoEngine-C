#ifndef NANOSCENE_H_
#define NANOSCENE_H_

#include <stdint.h>
#include "NanoGraphicsPipeline.h"

struct NanoRenderer;
struct NanoEngine;
struct NanoImage;
struct FrameData;

typedef struct RenderableObject RenderableObject;
typedef struct RenderableScene RenderableScene;
typedef struct RenderableNode RenderableNode;
typedef struct FrameData FrameData;

typedef enum {
TRIANGLE,
SQUARE,
CUBE,
SPHERE,
NUM_PRIMITIVE
} Primitive;

struct RenderableObject {
    MeshMemoryObject meshObject;
    int32_t ID; //on a per scene basis. 0 for the first object of every scene

    mat4 model;

    // Texture
    NanoImage* albedoTexture;
    NanoImage* normalTexture;
    NanoImage* additionalTexture1;
    NanoImage* additionalTexture2;
};

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
    RenderableObject renderableObject;

    mat4 localModel;

    RenderableNode* childNodes[16];
    uint32_t numChild;

    //update function
    void (*Update)(void* objectToUpdate, void* frameData);
};


void InitRenderableScene(struct NanoEngine* nanoEngine, RenderableScene* renderableScene);
void AddObjectToScene(RenderableObject* object, RenderableScene* renderableScene);
void AddRootNodeToScene(struct RenderableNode* rootNode, RenderableScene* renderableScene);
void PropagateNodeTransform(struct RenderableNode* rootNode);
void UpdateScene(RenderableScene* renderableScene, FrameData* data);
void CompileRenderableScene(RenderableScene* renderableScene);
void CleanUpScene(RenderableScene* renderableScene);

RenderableNode CreateRenderableNode(Vertex* vertices, uint32_t numVertices, uint32_t* indices, uint32_t numIndices);
RenderableNode CreateRenderableNodeFromPrimitive(Primitive primType, void* primParam, float color[4]);
RenderableNode CreateRenderableNodeFromFile(const char* fileName, float color[4]);

RenderableNode* AddChildRenderableNode(RenderableNode* renderableParent, RenderableNode* renderableChild);

#endif // NANOSCENE_H_
