#ifndef NANOENGINE_H_
#define NANOENGINE_H_

#include "NanoGraphics.h"
#include "NanoWindow.h"

typedef struct NanoEngine NanoEngine;

struct NanoEngine{
    NanoGraphics* m_pGraphics;
    NanoWindow* m_pWindow;
};

ERR InitEngine(NanoEngine* nanoEngine);
ERR CleanUpEngine(NanoEngine* nanoEngine);
ERR Run();
ERR MainLoop();

/* struct NanoEngine { */
/*   NanoGraphics m_NanoGraphics; */
/*   NanoWindow m_NanoWindow; */
/* }; */

#endif // NANOENGINE_H_
