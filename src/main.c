#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "NanoEngine.h"
#include "NanoInput.h"
#include "NanoRenderer.h"
#include "NanoScene.h"
#include "Str.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"

#include <string.h>

void UpdateNode(void* objectToUpdate, void* frameData){
    RenderableNode* object = (RenderableNode*)objectToUpdate;
    FrameData* fData = (FrameData*)frameData;

    /* glm_mat4_identity(object->localModel); */
    float axis_x[3] = {1.0f, 0.0f, 0.0f};
    float axis_y[3] = {0.0f, 1.0f, 0.0f};
    float axis_z[3] = {0.0f, 0.0f, 1.0f};
    glm_spin(object->localModel, fData->deltaTime * 0.5f, axis_x);
    glm_spin(object->localModel, fData->deltaTime * -0.8f, axis_y);
    /* glm_spin(object->localModel, fData->deltaTime * -1.0f, axis_z); */
};

RenderableNode MakeCubeNode(NanoEngine* nanoEngine){
    RenderableNode cubeRoot = CreateEmptyRenderableNode();
    SquareParam param1 = {.width = 1.0f,
                          .height = 1.0f,
                          .position = {-0.5f,0.5f,0.5f},
                          .color = {0.8f, 0.7f, 0.3f, 1.0f}};
    RenderableNode plane = CreateRenderableNodeFromPrimitive(SQUARE, &param1);
    RenderableNode cube[6] = {plane};
    // assembling the cube
    for(int i = 0; i < 4; i++){
        cube[i] = plane;
        float axis[3] = {0.0f, 1.0f, 0.0f};
        glm_rotate(cube[i].localModel, M_PI * 0.5f * (i), axis);
        AddChildRenderableNode(&cubeRoot, &cube[i]);
    }

    float axis[3] = {1.0f, 0.0f, 0.0f};
    cube[4] = plane;
    glm_rotate(cube[4].localModel, M_PI * 0.5f, axis);
    AddChildRenderableNode(&cubeRoot, &cube[4]);

    cube[5] = plane;
    glm_rotate(cube[5].localModel, M_PI * -0.5f, axis);
    AddChildRenderableNode(&cubeRoot, &cube[5]);

    // texturize the cube
    NanoImage textures[6] = {};
    const char* texts[6] = {"face 1","face 2","face 3","face 4","face 5","face 6"};
    float color[4] = {0.25f, 0.2f, 0.3f, 1.0f};
    float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    int verticalTextSpacing = 10;
    for(int i = 0; i < 6; i++){
        textures[i] = CreateHostPersistentImage(&nanoEngine->m_ImageMemory.imageHostMemory, 512, 512, IMAGE_FORMAT_RGBA, color);
        AddTextToImage(&textures[i], texts[i], 70, verticalTextSpacing, textColor);
        cube[i].renderableObject.albedoTexture = &textures[i];
    }

    cubeRoot.Update = UpdateNode;
    return cubeRoot;
}

int main(int argc, char *argv[]) {
    String cwd = CreateString(argv[0]);
    SubString(&cwd, 0, strlen(argv[0]) - strlen("NanoEngine"));
    SetArg0(cwd.m_data);
    if(argc > 1){
        if(strcmp(argv[1], "-FSC") == 0){
            LOG_MSG(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);

    // create scene
    RenderableScene scene;
    InitRenderableScene(&nanoEngine, &scene);

    RenderableNode cubeRoot = MakeCubeNode(&nanoEngine);

    AddRootNodeToScene(&cubeRoot, &scene);

    CompileRenderableScene(&scene);
    RenderScene(&scene);

    RunEngine(&nanoEngine);

    CleanUpScene(&scene);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
