#ifndef NANOSCENE_H_
#define NANOSCENE_H_

#include "MemManager.h"
#include "NanoGraphicsPipeline.h"

typedef struct{
    struct RenderableObject* renderableObjects[256];
    uint32_t numRenderableObjects;

    NanoImage* textures[256];
    uint32_t numTextures;
} RenderableScene;


void InitRenderableScene();

#endif // NANOSCENE_H_
