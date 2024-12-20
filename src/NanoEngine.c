#include "NanoEngine.h"
#include "MemManager.h"
#include "NanoRenderer.h"
#include "NanoWindow.h"

ERR CleanUpEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    CleanUpWindow(&nanoEngine->m_Window);
    CleanUpRenderer(&nanoEngine->m_Renderer);
    return err;
}

ERR InitEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    InitWindow(&nanoEngine->m_Window, WINDOW_WIDTH, WINDOW_HEIGHT, true);
    InitMeshHostMemory(&nanoEngine->m_meshMemory.meshHostMemory, MAX_MEMORY_MESH_OBJECT * MAX_VERTEX_PER_OBJECT);
    InitRenderer(&nanoEngine->m_Renderer, &nanoEngine->m_meshMemory, &nanoEngine->m_Window);
    return err;
}

ERR MainLoop(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow){
    ERR err = OK;
    PreDrawFrame(nanoRenderer, nanoWindow);
    DrawFrame(nanoRenderer, nanoWindow);
    return err;
}

ERR RunEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    while(!ShouldWindowClose(&nanoEngine->m_Window)){

        PollEvents(&nanoEngine->m_Window);
        MainLoop(&nanoEngine->m_Renderer, &nanoEngine->m_Window);

    }
    return err;
}

