#include <nds.h>

#include <stdio.h>

#include "Train.h"

#include "SharedPal.h"
#include "BG0.h"
#include "BG1.h"
#include "BG2.h"
#include "BG3.h"


//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------


	consoleDemoInit();
		// Sets the Video mode flag to mode 0 for the main screen (4xText BG)
	videoSetMode(MODE_0_2D);
	// Assign Memory Bank A to store the data for the 128KB sprite data (max 512KB)
	// VRAM_A_MAIN_SPRITE = 2 = 0b10 -> sets the setting for Bank A to be "ARM 9 Engine A OBJ memory" 
	vramSetBankE(VRAM_E_MAIN_SPRITE);
	
	// Initializes the actual OAM for the sprite mapping. Has to happen AFTER VRAM as been initialized
	oamInit(&oamMain, SpriteMapping_1D_128, false);

	// Assign Memory Bank B to store data for BG Layer 0
	// The variable VRAM_MAIN_BG would map to BG layer 1 instead
	// When using 3D is should be used instead, as the 3D Geometry Engine is fixed to BG0 automatically 
	// ATTENTION I was wrong. We can store all our bacgkrounds in a single VRAM Bank
	// Backgrounds can share a single VBANK! You do not need different VBANKS for each Background
	// Here all the backgrounds share the same VRAM Bank
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);


	// See page 101
	// Set BG Flag registers for expected data
	// Returns the id of the BG, which is a libnds concept used for BG functions 
	int bg = bgInit(3, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
	

	// Instructs the DMA Unit to copy data from ROM to VRAM Bank B
	dmaCopy(BG0Tiles, bgGetGfxPtr(bg), BG0TilesLen);
	dmaCopy(BG0Map, bgGetMapPtr(bg), BG0MapLen);
	// Instructs the DMA Unit to copy the BG Palette to the 1KB Palette memory
	// NOTE: Would this not overide OAM Palette data when we write w.o. an offset?
	// -> No because the OAM Palette space is different to BG Palette
	dmaCopy(SharedPalPal, BG_PALETTE, SharedPalPalLen);
	

	// Initialize BG1 with the same settings as BG0
	int bg1 = bgInit(2, BgType_Text8bpp, BgSize_T_256x256, 1, 2);
	// Copy the BG1 Tileset to the VRAM Bank C
	dmaCopy(BG1Tiles, bgGetGfxPtr(bg1), BG1TilesLen);
	// Copy the BG1 Map to the VRAM Bank C
	dmaCopy(BG1Map, bgGetMapPtr(bg1), BG1MapLen);

	int bg2 = bgInit(1, BgType_Text8bpp, BgSize_T_256x256, 2, 3);
	dmaCopy(BG2Tiles, bgGetGfxPtr(bg2), BG2TilesLen);
	dmaCopy(BG2Map, bgGetMapPtr(bg2), BG2MapLen);
	

	int bg3 = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 3, 4);
	dmaCopy(BG3Tiles, bgGetGfxPtr(bg3), BG3TilesLen);
	dmaCopy(BG3Map, bgGetMapPtr(bg3), BG3MapLen);

	// Allocate the memory on VRAM and return pointer to the memory address.
	// NOTE: This one is a puzzle to me. Where does it allocate the memory?
	// does it know to copy it to Bank A? Guess I have to print the address to find out.  
	// I assume it internally keeps a state of reserved memory
	// But why does the color depth matter, when its indexed anyway?
	// Isnt Color stored in the 2D Graphics Engine Memory anyway?
	// IT does Matter!: with 16 Colors we can store more pixels per byte than with 256 Colors
	// And it allocates the memory in VRAM E, while OAM data is seprately mapped at the OAM address
	u16* Train0 = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_256Color);
	
	//Copy the sprite data to the VRAM allocated earlier  
	dmaCopy(TrainTiles, Train0, 16*16);
	// Sprite Pallete appears to be 512B offset to the BG Palette memory. See page 156
	dmaCopy(TrainPal, SPRITE_PALETTE, TrainPalLen);

	u16* Train1 = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_256Color);
	
	//Copy the sprite data to the VRAM allocated earlier  
	dmaCopy(TrainTiles+64, Train1, 16*16);
	// Sprite Pallete appears to be 512B offset to the BG Palette memory. See page 156
	dmaCopy(TrainPal, SPRITE_PALETTE, TrainPalLen);

	//256 Colors means 8bpp. This means every Pixel can store 256 different indexed Color addresses
	//However the indexed Pallette colors themself are 2 Bytes each, so a 16 bit color palatte
	// Use this to access each palette entry
	u16* bg_palette = (u16*)BG_PALETTE;
	bg_palette[3] = 0xffff;


	bgUpdate();

	int train_rattle_count = 0;
	int train_rattle = 1;


	while(true)
	{
		bgScroll(bg, 1, 0);
		bgScroll(bg1, 2, 0);
		bgScroll(bg2, 3, 0);
		bgScroll(bg3, 8, 0);
		bgUpdate();

		oamSet(
			&oamMain, // which display
			0, // the oam entry to set
			0, 134+train_rattle, // x & y location
			0, // priority
			0, // palette for 16 color sprite or alpha for bmp sprite
			SpriteSize_16x16, // size
			SpriteColorFormat_256Color, // color type
			Train0, // the oam gfx
			-1, //affine index
			false, //double the size of rotated sprites
			false, //don't hide the sprite
			false, false, //vflip, hflip
			false //apply mosaic
		);

		oamSet(
			&oamMain, // which display
			1, // the oam entry to set
			16, 134+train_rattle, // x & y location
			0, // priority
			0, // palette for 16 color sprite or alpha for bmp sprite
			SpriteSize_16x16, // size
			SpriteColorFormat_256Color, // color type
			Train0, // the oam gfx
			-1, //affine index
			false, //double the size of rotated sprites
			false, //don't hide the sprite
			false, false, //vflip, hflip
			false //apply mosaic
		);
	
		oamSet(
			&oamMain, // which display
			2, // the oam entry to set
			32, 134+train_rattle, // x & y location
			0, // priority
			0, // palette for 16 color sprite or alpha for bmp sprite
			SpriteSize_16x16, // size
			SpriteColorFormat_256Color, // color type
			Train1, // the oam gfx
			-1, //affine index
			false, //double the size of rotated sprites
			false, //don't hide the sprite
			false, false, //vflip, hflip
			false //apply mosaic
		);
		oamUpdate(&oamMain);
		swiWaitForVBlank();
		
		train_rattle_count++;
		if(train_rattle_count > 5){
			train_rattle = train_rattle ? 0 : 1;
			train_rattle_count = 0;
		}
	
	}

	

}
