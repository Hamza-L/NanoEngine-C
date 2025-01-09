#ifndef NANOENGINE_H_
#define NANOENGINE_H_

#include "NanoRenderer.h"
#include "NanoWindow.h"

typedef struct NanoEngine NanoEngine;

struct NanoEngine{
    NanoRenderer m_Renderer;
    NanoWindow m_Window;
    MeshMemory m_meshMemory;
    ImageMemory m_ImageMemory;

    bool isInitialized;

    void (*framebufferResize_callback)(void* self, void* frameData);
    void (*key_callback)(void* self, void* frameData);
    void (*mouseButton_callback)(void* self, void* frameData);
    void (*cursorPos_callback)(void* self, void* frameData);
    void (*cursorScroll_callback)(void* self, void* frameData);
    void (*fileDrop_callback)(void* self, void* frameData);
};

ERR InitEngine(NanoEngine* nanoEngine);
ERR CleanUpEngine(NanoEngine* nanoEngine);
ERR RunEngine(NanoEngine* nanoEngine);

#endif // NANOENGINE_H_
