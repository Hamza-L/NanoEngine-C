#ifndef NANOSCENE_H_
#define NANOSCENE_H_

#include <stdint.h>

struct NanoRenderer;
struct NanoEngine;
struct NanoImage;

struct RenderableObject;

typedef struct{
    struct RenderableObject* renderableObjects[256];
    uint32_t numRenderableObjects;

    struct NanoImage* textures[256];
    uint32_t numTextures;
} RenderableScene;


void InitRenderableScene(struct NanoEngine* nanoEngine, RenderableScene* renderableScene);
void AddObjectToScene(struct RenderableObject* object, RenderableScene* renderableScene);
void CompileRenderableScene(RenderableScene* renderableScene);

#endif // NANOSCENE_H_
