#include "NanoEngine.h"
#include "GLFW/glfw3.h"
#include "MemManager.h"
#include "NanoRenderer.h"
#include "NanoWindow.h"
#include "NanoInput.h"

#include <stdio.h>

NanoEngine* s_nanoEngineSingleton;

ERR CleanUpEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    CleanUpWindow(&nanoEngine->m_Window);
    CleanUpMeshMemory(&nanoEngine->m_Renderer, &nanoEngine->m_meshMemory);
    CleanUpImageMemory(&nanoEngine->m_Renderer, &nanoEngine->m_ImageMemory);
    CleanUpRenderer(&nanoEngine->m_Renderer);
    nanoEngine->isInitializaed = false;
    return err;
}

ERR InitEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    if(s_nanoEngineSingleton != nullptr && s_nanoEngineSingleton->isInitializaed){
        CleanUpEngine(s_nanoEngineSingleton);
    }
    InitWindow(&nanoEngine->m_Window, WINDOW_WIDTH, WINDOW_HEIGHT, true);
    glfwSetWindowUserPointer(nanoEngine->m_Window._window, nanoEngine);

    InitMeshHostMemory(&nanoEngine->m_meshMemory.meshHostMemory, MAX_MEMORY_MESH_OBJECT * MAX_VERTEX_PER_OBJECT);
    InitImageHostMemory(&nanoEngine->m_ImageMemory.imageHostMemory, MAX_TOTAL_ALLOCATED_IMAGES_MEMSIZE);
    InitRenderer(&nanoEngine->m_Renderer,
                 &nanoEngine->m_meshMemory,
                 &nanoEngine->m_ImageMemory,
                 &nanoEngine->m_Window);

    nanoEngine->isInitializaed = true;
    s_nanoEngineSingleton = nanoEngine;
    return err;
}

ERR MainLoop(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow){
    ERR err = OK;
    PreDrawFrame(nanoRenderer, nanoWindow);
    DrawFrame(nanoRenderer, nanoWindow);
    return err;
}

void ProcessEvents(NanoEngine* nanoEngine){
    NanoKey key = PopMostRecentInputKey();

    if(key.key_id != -1){
        printf("PROCESSEVENTS: key pressed: %d\n", key.key_id);
        printf("PROCESSEVENTS: key pressed: %d\n", key.key_state);
    }
}

ERR RunEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    nanoEngine->m_Renderer.m_pNanoContext->m_frameData.time = 0;
    nanoEngine->m_Renderer.m_pNanoContext->m_frameData.deltaTime = 0;
    glfwSetTime(0);
    while(!ShouldWindowClose(&nanoEngine->m_Window)){
        PollEvents(&nanoEngine->m_Window);
        ProcessEvents(nanoEngine);
        MainLoop(&nanoEngine->m_Renderer, &nanoEngine->m_Window);
    }
    return err;
}

