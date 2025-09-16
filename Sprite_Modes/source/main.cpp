#include <nds.h>

#include <stdio.h>

#include "eevee_tileset.h"
#include "bg_map.h"

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------


	consoleDemoInit();
		// Sets the Video mode flag to mode 0 for the main screen (4xText BG)
	videoSetMode(MODE_0_2D);
	// Assign Memory Bank A to store the data for the 128KB sprite data (max 512KB)
	// VRAM_A_MAIN_SPRITE = 2 = 0b10 -> sets the setting for Bank A to be "ARM 9 Engine A OBJ memory" 
	vramSetBankA(VRAM_A_MAIN_SPRITE);
	
	// Initializes the actual OAM for the sprite mapping. Has to happen AFTER VRAM as been initialized
	oamInit(&oamMain, SpriteMapping_1D_128, false);

	// Assign Memory Bank B to store data for BG Layer 0
	// The variable VRAM_MAIN_BG would map to BG layer 1 instead
	// When using 3D is should be used instead, as the 3D Geometry Engine is fixed to BG0 automatically 
	vramSetBankB(VRAM_B_MAIN_BG_0x06000000);


	// See page 101
	// Set BG Flag registers for expected data
	// Returns the id of the BG, which is a libnds concept used for BG functions 
	int bg = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);

	// Instructs the DMA Unit to copy data from ROM to VRAM Bank B
	dmaCopy(bg_mapTiles, bgGetGfxPtr(bg), bg_mapTilesLen);
	// Instructs the DMA Unit to copy the BG Palette to the 1KB Palette memory
	// NOTE: Would this not overide OAM Palette data when we write w.o. an offset?
	dmaCopy(bg_mapPal, BG_PALETTE, bg_mapPalLen);

	// Allocate the memory on VRAM and return pointer to the memory address.
	// NOTE: This one is a puzzle to me. Where does it allocate the memory?
	// does it know to copy it to Bank A? Guess I have to print the address to find out.  
	// I assume it internally keeps a state of reserved memory
	// But why does the color depth matter, when its indexed anyway?
	// Isnt Color stored in the 2D Graphics Engine Memory anyway?
	u16* voi = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
	
	//Copy the sprite data to the VRAM allocated earlier  
	dmaCopy(eevee_tilesetTiles, voi, 512);
	// Sprite Pallete appears to be 512B offset to the BG Palette memory. See page 156
	dmaCopy(eevee_tilesetPal, SPRITE_PALETTE, 32);

	iprintf("VOI addr: %p\n", voi);
	oamSet(
		&oamMain, // which display
		0, // the oam entry to set
		10, 10, // x & y location
		0, // priority
		0, // palette for 16 color sprite or alpha for bmp sprite
		SpriteSize_32x32, // size
		SpriteColorFormat_16Color, // color type
		voi, // the oam gfx
		-1, //affine index
		false, //double the size of rotated sprites
		false, //don't hide the sprite
		false, false, //vflip, hflip
		false //apply mosaic
	);

	
	while (true)
	{



		oamUpdate(&oamMain);
		swiWaitForVBlank();
	
	}
	

}
