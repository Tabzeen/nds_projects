#include <nds.h>

#include <stdio.h>
#include "ExportedTilemap.h"

#include "BG_Tiles.h"


#define SCREEN_SIZE_X 256
#define SCREEN_SIZE_Y 192
#define BG_SIZE 512
#define BG_CENTER 256

#define SCREEN_CENTER_X (BG_CENTER - (SCREEN_SIZE_X >> 1))
#define SCREEN_CENTER_Y (BG_CENTER - (SCREEN_SIZE_Y >> 1))


#define NO_TILES (512>>3) 
#define VRAM_BLOCK_SIZE (32*32)

static u16 tilemap[(NO_TILES) * (NO_TILES)];

static u16 metatile[9] = {
	0x0001, 0x0002, 0x0003,
	0x0004, 0x0005, 0x0006,
	0x0007, 0x0008, 0x0009
};

struct Vector_2D{
	u16 x;
	u16 y;
};

// Converts axes coordinates, to coordinates of the tile in VRAM
// For more information see Obsidian entry, as the implementation is pretty cryptic
u16 convert_coords(u16 x, u16 y){
	// Wrap around VRAM size. Assumes 64 tiles in each direction
	x &= (NO_TILES - 1); y &= (NO_TILES - 1); 
	int ch_off = (x >> 5) + ((y >> 5) << 1);
	// Same as x % 31, but doesn't crash
	x &= 31; y &= 31;
	return x + (y<<5) + ch_off * VRAM_BLOCK_SIZE;
}

void tilemap_writer(u16* target_tilemap, u16* source_tilemap, int bg_idx, int offset_x, int offset_y){
	for(u16 i = 0; i < NO_TILES; i++)
		for(u16 j = 0; j < NO_TILES; j++)
			//TODO Assert, that Array Memory is not left
			// Cannot write directly to DMA due to conversion of coordinates
			// In the best case, the source tilemap should be preprocessed to convert at compile time
			target_tilemap[convert_coords(i,j)] = source_tilemap[(i+offset_x + (j+offset_y)*128)];

	dmaCopy(target_tilemap, bgGetMapPtr(bg_idx), NO_TILES * NO_TILES);
}

struct Vector_2D vec_add(Vector_2D a, Vector_2D b){
	return {a.x + b.x, a.y + b.y};
}


//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------


	consoleDemoInit();
	defaultExceptionHandler();
	// Sets the Video mode flag to mode 0 for the main screen (4xText BG)
	videoSetMode(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);

	// See page 101
	// Set BG Flag registers for expected data
	// Returns the id of the BG, which is a libnds concept used for BG functions 
	int bg = bgInit(0, BgType_Text8bpp, BgSize_T_512x512, 0, 1);
	

	// Instructs the DMA Unit to copy data from ROM to VRAM Bank B
	dmaCopy(BG_TilesTiles, bgGetGfxPtr(bg), BG_TilesTilesLen);

	// Write first segment of the tilemap
	//tilemap_writer(tilemap, ExportedTilemap_tilemap, bg, 0, 0);
	

	//bgScroll(bg, SCREEN_CENTER_X, SCREEN_CENTER_Y);
	bgScroll(bg, BG_SIZE - SCREEN_SIZE_X, 0);
	bgUpdate();

	dmaCopy(BG_TilesPal, BG_PALETTE, BG_TilesPalLen);

	// Local HW scroll level
	int scroll_x = SCREEN_CENTER_X;
	int scroll_y = SCREEN_CENTER_Y;
	// Global abstract scrol level
	int gscroll_x = 0;
	int gscroll_y = 0;


	// Init Global and Local positons
	// Global pos describes the position on the full tilemap
	Vector_2D global_pos = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
	// Previous global pos at the last load to calculate delta
	Vector_2D prev_global_pos = {0,0};
	// Local position describes the position on the VRAM, should just be the global pos mod' with the BG size
	Vector_2D local_pos = {0, 0};
	// BGScroll describes the scrolling position and is derived from the scrolling position (may be redundant to local pos)
	Vector_2D bg_scroll = {0,0};
	
	
	u16 h_diff = SCREEN_SIZE_X - global_pos.x; //TODO +1 to capture the border around the screen
	u16 v_diff = SCREEN_SIZE_Y - global_pos.y;
	
	
	// NOTE The write part is still placeholder code, but it actually seems to work now
	// The real mistake I did was to just not scroll the camera back to 0. It now works fine without any problems

	// Perform initial rendering of the visible area
	// Could be a lot more efficient if we store data VRAM aligned by not requiering a conversion   
	for(int i = 0; i < (SCREEN_SIZE_X >> 3) + 2; i++)
	for(int j = 0; j < (SCREEN_SIZE_Y >> 3) + 2; j++){
		// For single bytes a CPU copy is faster. Assumes 8bpp
		// Begin at the end of the VRAM to build a buffer around the visible area
		// convert_coords contains a mod to wrap around the VRAM
		
		// Non DMA access has to be u16 aligned, afaik
		// Conversion should not be needed anymore when we actually use a chuncked map
		u16 addr = convert_coords(global_pos.x - h_diff + i,global_pos.y - v_diff + j);
		// We need to flush the memory we want to read from
		DC_FlushRange(ExportedTilemap_tilemap + i + j * NO_TILES, sizeof(u16));
		*(bgGetMapPtr(bg) + addr)= (u16)ExportedTilemap_tilemap[i + j * NO_TILES];
		
	}
	

	while(1){

		scanKeys();
		
		u16 kHeld = keysHeld();
        if (kHeld & KEY_UP) gscroll_y -= 1;
        else if (kHeld & KEY_DOWN) gscroll_y += 1; 
        if (kHeld & KEY_LEFT) gscroll_x -= 1;
        else if (kHeld & KEY_RIGHT) gscroll_x += 1;


		// Backgroud update logic 
		bgSetScroll(bg, gscroll_x, gscroll_y);
		bgUpdate();

		swiWaitForVBlank();

	};

	
	

}
