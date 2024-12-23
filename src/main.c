#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "MemManager.h"
#include "NanoBuffers.h"
#include "NanoEngine.h"
#include "NanoGraphicsPipeline.h"
#include "NanoRenderer.h"
#include "NanoScene.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"
//cube.obj
/* v 1.000000 -1.000000 -1.000000 */
/* v 1.000000 -1.000000 1.000000 */
/* v -1.000000 -1.000000 1.000000 */
/* v -1.000000 -1.000000 -1.000000 */
/* v 1.000000 1.000000 -0.999999 */
/* v 0.999999 1.000000 1.000001 */
/* v -1.000000 1.000000 1.000000 */
/* v -1.000000 1.000000 -1.000000 */
/* f 2/1/1 3/2/1 4/3/1 */
/* f 8/1/2 7/4/2 6/5/2 */
/* f 5/6/3 6/7/3 2/8/3 */
/* f 6/8/4 7/5/4 3/4/4 */
/* f 3/9/5 7/10/5 8/11/5 */
/* f 1/12/6 4/13/6 8/11/6 */
/* f 1/4/1 2/1/1 4/3/1 */
/* f 5/14/2 8/1/2 6/5/2 */
/* f 1/12/3 5/6/3 2/8/3 */
/* f 2/12/4 6/8/4 3/4/4 */
/* f 4/13/5 3/9/5 8/11/5 */
/* f 5/6/6 1/12/6 8/11/6 */

RenderableObject CreateSquare(float offset[2], float size[2], float color[4]){

    RenderableObject squareObject;

    Vertex vertices[4] = {{{ offset[0]          , offset[1] - size[1], 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 1.0f}},
                      {{ offset[0] + size[0], offset[1] - size[1], 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 1.0f}},
                      {{ offset[0] + size[0], offset[1]          , 0.0f}, {color[0], color[1], color[2], color[3]}, {1.0f, 0.0f}},
                      {{ offset[0]          , offset[1]          , 0.0f}, {color[0], color[1], color[2], color[3]}, {0.0f, 0.0f}}};

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

    InitRenderableObject(vertices, 4, indices, 6, &squareObject);

    return squareObject;
}

int main(int argc, char *argv[]) {

    SetVar(argv[0]);
    if(argc > 1){
        fprintf(stderr, "argument passed in\n");
        if(strcmp(argv[1], "-FSC") == 0){
            fprintf(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }
    fprintf(stderr, "ARG0: %s\n", GetArg0());

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);

    NanoGraphicsPipeline* graphicsPipeline = &nanoEngine.m_Renderer.m_pNanoContext->graphicsPipelines[nanoEngine.m_Renderer.m_pNanoContext->currentGraphicsPipeline];

    // create scene
    RenderableScene scene;
    InitRenderableScene(&nanoEngine, &scene);

    // create object 1
    float offset[2] = {-0.5f, 0.5f};
    float size[2] = {0.5f, 0.5f};
    float color1[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    RenderableObject object1 = CreateSquare( offset, size, color1);

    NanoImage texture1 = CreateHostPersistentImageFromFile(&nanoEngine.m_ImageMemory.imageHostMemory, "./textures/Vulkan Texture.jpg");
    AddTextureToRenderableObject(&texture1, &object1);

    // create object 2
    offset[0] = 0.5f; offset[1] = 0.5f;
    float color2[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    RenderableObject object2 = CreateSquare( offset, size, color2);

    NanoImage texture2 = CreateHostPersistentImageFromFile(&nanoEngine.m_ImageMemory.imageHostMemory, "./textures/Giraffe.jpg");
    AddTextureToRenderableObject(&texture2, &object2);

    AddObjectToScene(&object1, &scene);
    AddObjectToScene(&object2, &scene);
    CompileRenderableScene(&scene);
    RenderScene(&scene);

    RunEngine(&nanoEngine);

    /* CleanUpMeshObject(&nanoEngine.m_Renderer, &object); */
    CleanUpScene(&scene);
    CleanUpMeshMemory(&nanoEngine.m_Renderer, &nanoEngine.m_meshMemory);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
