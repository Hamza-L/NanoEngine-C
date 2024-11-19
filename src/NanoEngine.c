#include "NanoEngine.h"
#include "NanoConfig.h"
#include "NanoGraphics.h"
#include "NanoShader.h"
#include "NanoWindow.h"

ERR CleanUpEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    CleanUpWindow(&nanoEngine->m_Window);
    CleanUpGraphics(&nanoEngine->m_Graphics);
    return err;
}

ERR InitEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    InitWindow(&nanoEngine->m_Window, WINDOW_WIDTH, WINDOW_HEIGHT, true);
    InitGraphics(&nanoEngine->m_Graphics, &nanoEngine->m_Window);
    return err;
}

ERR MainLoop(NanoGraphics* nanoGraphics){
    ERR err = OK;
    DrawFrame(nanoGraphics);
    return err;
}

ERR RunEngine(NanoEngine* nanoEngine){
    ERR err = OK;
    while(!ShouldWindowClose(nanoEngine->m_Window)){

        PollEvents(nanoEngine->m_Window);
        MainLoop(&nanoEngine->m_Graphics);

    }
    return err;
}

