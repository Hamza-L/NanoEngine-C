#ifndef NANOGRAPHICS_H_
#define NANOGRAPHICS_H_

#include "NanoWindow.h"

struct NanoGraphics{
    public:
        ERR Init(NanoWindow& window);
        ERR DrawFrame();
        ERR CleanUp();
};

#endif // NANOGRAPHICS_H_
