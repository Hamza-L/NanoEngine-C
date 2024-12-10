#include <stdio.h>
#include <stdlib.h>

#include "NanoEngine.h"
#include "NanoUtility.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>

#include "NanoConfig.h"

#include <ft2build.h>
#include FT_FREETYPE_H

/* FT_Library  library; */
/* FT_Face     face;      /\* handle to face object *\/ */

int main(int argc, char *argv[]) {
    /* int error = FT_Init_FreeType( &library ); */
    /* if ( error ) { */
    /*     fprintf(stderr, "ERROR OCCURED LOADING FONTS\n"); */
    /*     abort(); */
    /* } */
    /* error = FT_New_Face( library, */
    /*                      "/Users/h_lahmimsi/Library/Fonts/Cascadia.ttf", */
    /*                      0, */
    /*                      &face ); */

    /* if ( error == FT_Err_Unknown_File_Format ) { */
    /*     fprintf(stderr, "UNKNOWW FILE FORMAT FOR GIVEN FONTS\n"); */
    /*     abort(); */
    /* } */
    /* else if ( error ) { */
    /*     fprintf(stderr, "UNKOWN ERROR OCCURED LOADING FONTS\n"); */
    /*     abort(); */
    /* } */

    /* // error = FT_Set_Char_Size( */
    /* //     face,    /\* handle to face object         *\/ */
    /* //     0,       /\* char_width in 1/64 of points  *\/ */
    /* //     16*64,   /\* char_height in 1/64 of points *\/ */
    /* //     300,     /\* horizontal device resolution  *\/ */
    /* //     300 );   /\* vertical device resolution    *\/ */

    /* error = FT_Set_Pixel_Sizes( */
    /*     face, */
    /*     0, // 0 if same as pixel height */
    /*     16); */

    /* if ( error ) { */
    /*     fprintf(stderr, "ERROR SETTING FONT SIZE\n"); */
    /*     abort(); */
    /* } */

    /* FT_GlyphSlot  slot = face->glyph;  /\* a small shortcut *\/ */
    /* int           pen_x, pen_y, n; */
    /* const int num_chars = 5; */
    /* const uint32_t text[] = {'H','e','l','l','o'}; */

    /* pen_x = 300; */
    /* pen_y = 200; */

    /* for ( n = 0; n < num_chars; n++ ) */
    /* { */
    /*     FT_UInt  glyph_index; */


    /*     /\* retrieve glyph index from character code *\/ */
    /*     glyph_index = FT_Get_Char_Index( face, text[n] ); */

    /*     /\* load glyph image into the slot (erase previous one) *\/ */
    /*     error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT ); */
    /*     if ( error ) */
    /*         continue;  /\* ignore errors *\/ */

    /*     /\* convert to an anti-aliased bitmap *\/ */
    /*     error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL ); */
    /*     if ( error ) */
    /*         continue; */

    /*     /\* /\\* now, draw to our target surface *\\/ *\/ */
    /*     /\* my_draw_bitmap( &slot->bitmap, *\/ */
    /*     /\*                 pen_x + slot->bitmap_left, *\/ */
    /*     /\*                 pen_y - slot->bitmap_top ); *\/ */

    /*     /\* increment pen position *\/ */
    /*     pen_x += slot->advance.x >> 6; // divide by 64 */
    /*     pen_y += slot->advance.y >> 6; /\* not useful for now *\/ */
    /* } */

    SetVar(argv[0]);
    if(argc > 1){
        fprintf(stderr, "argument passed in\n");
        if(strcmp(argv[1], "-FSC") == 0){
            fprintf(stderr, "force shader compilation enabled\n");
            SetForceShaderRecompile(true);
        }
    }
    uint32_t sizeOfBuffer = 0;
    ReadBinaryFile(argv[0], &sizeOfBuffer);

    fprintf(stderr, "ARG0: %s\n", GetArg0());

    NanoEngine nanoEngine = {};
    InitEngine(&nanoEngine);
    RunEngine(&nanoEngine);
    CleanUpEngine(&nanoEngine);

    return EXIT_SUCCESS;
}
