#include "NanoEngine.h"
#include "NanoConfig.h"
#include "NanoRenderer.h"
#include "NanoShader.h"
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
    InitRenderer(&nanoEngine->m_Renderer, &nanoEngine->m_Window);
    return err;
}

ERR MainLoop(NanoRenderer* nanoRenderer, NanoWindow* nanoWindow){
    ERR err = OK;
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

