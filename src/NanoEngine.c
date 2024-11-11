#include "NanoEngine.h"

ERR CleanUpEngine(NanoEngine* nanoEngine){
    ERR err = ERR::OK;
    s_NanoWindow.CleanUp();
    s_NanoGraphics.CleanUp();
    return err;
}

ERR InitEngine(NanoEngine* nanoEngine){
    ERR err = ERR::OK;
    err = s_NanoWindow.Init();
    err = s_NanoGraphics.Init(m_NanoWindow);
    return err;
}

ERR NanoEngineRun(){
    ERR err = ERR::OK;
    while(!m_NanoWindow.ShouldWindowClose()){

        m_NanoWindow.PollEvents();

        MainLoop();

    }
    return err;
}

ERR MainLoop(){
    ERR err = ERR::OK;

    m_NanoGraphics.DrawFrame();

    return err;
}
