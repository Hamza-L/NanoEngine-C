#include <stdio.h>

#include "NanoError.h"
#include "NanoEngine.h"

int main(int argc, char *argv[]) {

    struct NanoEngine engine;
    engine.Init();
    engine.Run();

    return EXIT_SUCCESS;
}
