#ifndef NANOENGINE_H_
#define NANOENGINE_H_

#include "NanoRenderer.h"
#include "NanoWindow.h"
#include "MemManager.h"

typedef struct NanoEngine NanoEngine;

struct NanoEngine{
    NanoRenderer m_Renderer;
    NanoWindow m_Window;
    MeshMemory m_meshMemory;
    ImageMemory m_ImageMemory;
};

ERR InitEngine(NanoEngine* nanoEngine);
ERR CleanUpEngine(NanoEngine* nanoEngine);
ERR RunEngine(NanoEngine* nanoEngine);

/* struct NanoEngine { */
/*   NanoGraphics m_NanoGraphics; */
/*   NanoWindow m_NanoWindow; */
/* }; */

#endif // NANOENGINE_H_
