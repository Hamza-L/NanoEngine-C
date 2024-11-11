#ifndef NANOGRAPHICS_H_
#define NANOGRAPHICS_H_

#include "NanoWindow.h"

ERR InitGraphics(NanoWindow* window);
ERR DrawFrame();
ERR CleanUp();

#endif // NANOGRAPHICS_H_
