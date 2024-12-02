#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "NanoEngine.h"
#include "NanoShader.h"
#include "Str.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library  library;
FT_Face     face;      /* handle to face object */

int main(int argc, char *argv[]) {
    int error = FT_Init_FreeType( &library );
    if ( error )
    {
        fprintf(stderr, "ERROR OCCURED LOADING FONTS\n");
        abort();
    }
    error = FT_New_Face( library,
                         "/Users/h_lahmimsi/Library/Fonts/Cascadia.ttf",
                         0,
                         &face );

    if ( error == FT_Err_Unknown_File_Format )
    {
        fprintf(stderr, "UNKNOWW FILE FORMAT FOR GIVEN FONTS\n");
        abort();
    }
    else if ( error )
    {
        fprintf(stderr, "UNKOWN ERROR OCCURED LOADING FONTS\n");
        abort();
    }

    error = FT_Set_Char_Size(
        face,    /* handle to face object         */
        0,       /* char_width in 1/64 of points  */
        16*64,   /* char_height in 1/64 of points */
        300,     /* horizontal device resolution  */
        300 );   /* vertical device resolution    */

    if ( error )
    {
        fprintf(stderr, "ERROR SETTING FONT SIZE\n");
        abort();
    }

    SetVar(argv[0]);
    if(argc > 1){
        fprintf(stderr, "argument passed in\n");
        if(strcmp(argv[1], "-FSC") == 0){
            fprintf(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }

    fprintf(stderr, "ARG0: %s\n", GetArg0());

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);
    RunEngine(&nanoEngine);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
