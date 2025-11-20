#include <nds.h>

#include <stdio.h>
#include <CharSet.h>

#define RED 0
#define GREEN 6
#define BLUE 12 

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

    consoleDemoInit();
    videoSetMode(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    // Idk where Libnds stores the standard character set (There is something stored at 0x600000), so to no override it, we store the bg in tile offset 1
    int bg_char = bgInit(1, BgType_Text8bpp, BgSize_B8_256x256, 0, 1);

    dmaCopy(CharSetTiles, bgGetGfxPtr(bg_char), CharSetTilesLen);
    dmaCopy(CharSetPal, BG_PALETTE, CharSetPalLen);
    dmaCopy(CharSetMap, bgGetMapPtr(bg_char), CharSetMapLen);
    *(BG_PALETTE + 1) = (0x00 < RED) | (0x1F << GREEN) | (0x1F << BLUE);    
    bgUpdate();

    while (1)
    {
        swiWaitForVBlank();
        iprintf("Hello!");
    }
    
}
