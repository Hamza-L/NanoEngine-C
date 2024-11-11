#ifndef NANOENGINE_H_
#define NANOENGINE_H_

#include "NanoGraphics.h"
#include "NanoWindow.h"


enum ERR Init();
ERR Run();
ERR CleanUp();

ERR MainLoop();

/* struct NanoEngine { */
/*   NanoGraphics m_NanoGraphics; */
/*   NanoWindow m_NanoWindow; */
/* }; */

#endif // NANOENGINE_H_
