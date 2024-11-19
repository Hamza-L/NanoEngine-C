#ifndef NANOENGINE_H_
#define NANOENGINE_H_

#include "NanoGraphics.h"
#include "NanoWindow.h"

typedef struct NanoEngine NanoEngine;

struct NanoEngine{
    NanoGraphics m_Graphics;
    NanoWindow m_Window;
};

ERR InitEngine(NanoEngine* nanoEngine);
ERR CleanUpEngine(NanoEngine* nanoEngine);
ERR RunEngine(NanoEngine* nanoEngine);

/* struct NanoEngine { */
/*   NanoGraphics m_NanoGraphics; */
/*   NanoWindow m_NanoWindow; */
/* }; */

#endif // NANOENGINE_H_
