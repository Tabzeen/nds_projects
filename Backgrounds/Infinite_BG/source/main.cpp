#include <nds.h>

#include <stdio.h>
#include "ExportedTilemap.h"
#include "emap.h"
#include "BG_Tiles.h"


#define RED 0
#define GREEN 6
#define BLUE 12

#define SCREEN_SIZE_X 256
#define SCREEN_SIZE_Y 192
#define BG_SIZE 512
#define BG_CENTER 256

#define SCREEN_CENTER_X (SCREEN_SIZE_X >> 1)
#define SCREEN_CENTER_Y (SCREEN_SIZE_Y >> 1)

#define SCREEN_CENTER_X_TILES (SCREEN_CENTER_X >> 3)
#define SCREEN_CENTER_Y_TILES (SCREEN_CENTER_Y >> 3) 

#define NO_TILES (512>>3) 
#define VRAM_BLOCK_SIZE (32*32)

static u16 tilemap[(NO_TILES) * (NO_TILES)];


static u16 metatile[9] = {
	0x0001, 0x0002, 0x0003,
	0x0004, 0x0005, 0x0006,
	0x0007, 0x0008, 0x0009
};

static u16 test_tile[9] = {
	0x01, 0x00, 0x00,
	0x00, 0x01, 0x00,
	0x01, 0x00, 0x01
};

template <typename T>
struct Vector_2D{
	T x;
	T y;
};


//TODO Rename to "Render Boundry" or just "Boundry"
typedef union{
	struct {
		u16 n;
		u16 s;
		u16 e;
		u16 w;
	} sides;
	u16 mem[4];
} Screen_Border;

// Converts axes coordinates, to coordinates of the tile in VRAM
// For more information see Obsidian entry, as the implementation is pretty cryptic
// This one assumes that the BG Register is set to 512x512
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

template <typename T>
Vector_2D<T> vec_add(Vector_2D<T> a, Vector_2D<T> b){   
	return {(a.x + b.x), (a.y + b.y)}; 
}

template <typename T, typename U>
Vector_2D<U> vec_sub(Vector_2D<T> a, Vector_2D<T> b){   
    return Vector_2D<U>{
        static_cast<U>(a.x - b.x),
        static_cast<U>(a.y - b.y)
    };
}



// This needs TILEMAP ENTRIES!! (NOT Pixel coords)
// This whole dest_start and dest_end thing might be somewhat stupid, maybe replace it with the usual "No of Rows and Cols as a Vector 2D"
void drawStripe(int bg_id, u16* src_tilemap, u16 src_tilemap_len, Vector_2D<u16> dest_start, Vector_2D<u16> dest_end, Vector_2D<u16> src_start, int src_width, int src_height){
	
	// The Problem here is the wraparound not being handled in the loop
	// If the dest_start is larger than dest_end than nothing will happen here, so modulo conversion needs to happen before the loop
	// This worked before, because we did not handle modulo and overflows before and just let the values roll over to huge numbers
	
	
	// We need normal calculation of we are not around the edge, and special wrap around calculation if we are
	u32 stripe_width  =  dest_start.x < dest_end.x ? dest_end.x - dest_start.x :
		dest_end.x + NO_TILES - (dest_start.x & (NO_TILES - 1));
   	u32 stripe_height = dest_start.y < dest_end.y ? dest_end.y - dest_start.y :  
		dest_end.y + NO_TILES - (dest_start.y & (NO_TILES - 1));			
	for(int i = 0; i < stripe_width; i++){
		for(int j = 0; j < stripe_height; j++){
			// For single bytes a CPU copy is faster. Assumes 8bpp
			// Begin at the end of the VRAM to build a buffer around the visible area
			// convert_coords contains a mod to wrap around the VRAM
		
			// Non DMA access has to be u16 aligned, afaik
			// Conversion should not be needed anymore when we actually use a chuncked map (thats an assumtion)
			// Converts the normal mapping to the 32x32 chunck mapping of tile_idx values loaded by the 2D engine from the VRAM
			u16 addr = convert_coords(dest_start.x + i, dest_start.y + j);
			// Here we assume the tilemap in ROM follows the normal mapping
			// THis time we actually need 32 bits, as the src tilemap could theoretically be infinite
			// Wee need to mod the X and Y directions seperately to be able to wrap around in each direction
			int src_addr = ((src_start.x + i) % src_width) + ((src_start.y + j) % src_height) * src_width;
			*(bgGetMapPtr(bg_id) + addr) = (u16)src_tilemap[src_addr];
			fprintf(stderr, "	Written to tile %i, %i\n", dest_start.x + i, dest_start.y + j);
		}
		fprintf(stderr, "Finished writing Row %i \n", dest_start.y + i);
	}

	fprintf(stderr, "\n ####### End of drawStripe ###### \n");
	
}


/*
This is all absolutely stupid and I should have noticed earlier
There is a reason why tilemaps are exported in 32x32 chunks.
Nobody streams tiles just aroudn the border. This wastes CPU time for each write and is highly inefficient
Instead people use the DMA to just copy full 32x32 Chunks into VRAM. This has zero CPU untilization and makes it incredibly
easy to load larger tilemaps without any conversion nonsence needed. I maybe just finish this as is just for the hell of it
but efficiently this is never done in HW 


...nevermind they do. Just booted up Emerald in No$GBA and it actually loads 

On another note, a 512x512 Background might be unnecessary if im streaming the data in anyway and dont keep track of "visited"
No we NEED an 512x512 Backgorund, or else we cannot have an out of screen buffer, as the full width of the BG is visible
*/

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------


	consoleDemoInit();
	consoleDebugInit(DebugDevice_NOCASH);
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
	// Write first segment of the tilemapBG_Tiles
	//tilemap_writer(tilemap, ExportedTilemap_tilemap, bg, 0, 0);
	
	dmaCopy(BG_TilesPal, BG_PALETTE, BG_TilesPalLen);
	
	// For debugging we just override the original palette and tiles for now
	// override the tiles to point to a simple color value each
	static u8 i0i1_tile_data[126] = {0};
	memset((void*)i0i1_tile_data + 64, 0x01, 64);
	dmaCopy((void *)i0i1_tile_data, bgGetGfxPtr(bg), 128);

	



	// Write the pallate data to contain a simple color value
	*(BG_PALETTE) = (0x00 << RED) | (0x1F << GREEN) | (0x1F << BLUE);
	*(BG_PALETTE + 2) = (0x1F << RED) | (0x1F << GREEN) | (0x00 << BLUE);  
	
	//bgScroll(bg, SCREEN_CENTER_X, SCREEN_CENTER_Y);
	bgScroll(bg, BG_SIZE - SCREEN_SIZE_X, 0);
	bgUpdate();


	// Local HW scroll level
	int scroll_x = SCREEN_CENTER_X;
	int scroll_y = SCREEN_CENTER_Y;
	// Global abstract scrol level
	int gscroll_x = 0;
	int gscroll_y = 0;


	// Init Global and Local positons
	// Global pos describes the position on the full tilemap
	Vector_2D<u16> global_pos = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
	// Previous global pos at the last load to calculate delta
	Vector_2D<u16> prev_global_pos = global_pos; 
	// Delta position for checking if new parts of the map should be generated
	Vector_2D<s16> delta_pos = {0,0};
	// Local position describes the position on the VRAM, should just be the global pos mod' with the BG size
	Vector_2D<u16> local_pos = {0, 0};
	// BGScroll describes the scrolling position and is derived from the scrolling position (may be redundant to local pos)
	Vector_2D<u16> bg_scroll = {0,0};
	
	
	u16 h_diff = SCREEN_SIZE_X - global_pos.x; //TODO +1 to capture the border around the screen
	u16 v_diff = SCREEN_SIZE_Y - global_pos.y;



	Vector_2D<u16> dest_start = {0, 0};
	Vector_2D<u16> dest_end = {(SCREEN_SIZE_X >> 3) + 2, (SCREEN_CENTER_Y >> 3) + 2};
	Vector_2D<u16> src_start = {0, 0};
	Vector_2D<u16> src_end = {(SCREEN_SIZE_X >> 3) + 2, (SCREEN_CENTER_Y >> 3) + 2};
	Vector_2D<u16> center_tile = {0,0};

	Screen_Border local_borders = {0, 0, 0, 0};
	Screen_Border global_borders = local_borders;

	drawStripe(bg, e_map, e_map_len, dest_start, dest_end, src_start, 128, 128);


	bool update_canvas = 0;
	int last_dir = 0;
	while(1){

		// The general bug seems to be fully solved now. However it still rarely creates some wierd artifacts
		scanKeys();
		
		u16 kHeld = keysHeld();
        if (kHeld & KEY_UP) global_pos.y -= 1;
        else if (kHeld & KEY_DOWN) global_pos.y += 1;
        if (kHeld & KEY_LEFT) global_pos.x -= 1;
        else if (kHeld & KEY_RIGHT) global_pos.x += 1;
		

		center_tile = {global_pos.x >> 3, global_pos.y >> 3};
		//NOTE: This identity only works because the a mod m*k mod m == a mod m
		// Global borders (All that are actually needed)
		// Placeholder magic number for exported map size
		// Local Borders


		global_borders.sides.n = (center_tile.y - SCREEN_CENTER_Y_TILES - 1) & (128 - 1);
		global_borders.sides.s = (center_tile.y + SCREEN_CENTER_Y_TILES + 1) & (128 - 1);
		global_borders.sides.e = (center_tile.x + SCREEN_CENTER_X_TILES + 1) & (128 - 1);
		global_borders.sides.w = (center_tile.x - SCREEN_CENTER_X_TILES - 1) & (128 - 1);


		// Adjust for VRAM size
		for(int i = 0; i < 4; i++)
			local_borders.mem[i] = global_borders.mem[i] & (NO_TILES - 1);

		// Backgroud update logic 
		bgSetScroll(bg, global_pos.x - SCREEN_CENTER_X, global_pos.y - SCREEN_CENTER_Y);

		delta_pos = vec_sub<u16, s16>(global_pos, prev_global_pos);

		
		if(delta_pos.x >= 8){
			dest_end.x = local_borders.sides.e;
			dest_start.x = (dest_end.x - 1) & (NO_TILES - 1); 


			// For X we also draw the corner tiles, to avoid missing out tiles, when scrolling diagonally
			// This might need additional special handling, when we scroll more than one tile
			dest_start.y = local_borders.sides.n; 
			dest_end.y = local_borders.sides.s; 

			src_start = {global_borders.sides.e - 1, global_borders.sides.n};			
			drawStripe(bg, e_map, e_map_len, dest_start, dest_end, src_start, 128, 128);
			last_dir = 3;
			delta_pos.x = 0;
			prev_global_pos.x = global_pos.x;
		}
		else if(delta_pos.x <= -8){
			dest_start.x = local_borders.sides.w; 
			dest_end.x = dest_start.x + 1 & (NO_TILES - 1);

			dest_start.y = local_borders.sides.n; 
			dest_end.y = local_borders.sides.s; 

			src_start = {global_borders.sides.w, global_borders.sides.n};
			drawStripe(bg, e_map, e_map_len, dest_start, dest_end, src_start, 128, 128);
			last_dir = 9;
			delta_pos.x = 0;
			prev_global_pos.x = global_pos.x;
	
		}
		if(delta_pos.y >= 8){
			dest_end.y = local_borders.sides.s;
			dest_start.y = dest_end.y - 1 & (NO_TILES - 1);

			dest_start.x = local_borders.sides.w; 
			dest_end.x = local_borders.sides.e; 

			src_start = {global_borders.sides.w, global_borders.sides.s - 1};			
			drawStripe(bg, e_map, e_map_len, dest_start, dest_end, src_start, 128, 128);
			last_dir = 6;
			delta_pos.y = 0;
			prev_global_pos.y = global_pos.y;
	
		}
		else if(delta_pos.y <= -8){
			dest_start.y = local_borders.sides.n;
			dest_end.y = dest_start.y + 1 & (NO_TILES - 1);


			dest_start.x = local_borders.sides.w; 
			dest_end.x = local_borders.sides.e; 

			src_start = {global_borders.sides.w, global_borders.sides.n};			
			drawStripe(bg, e_map, e_map_len, dest_start, dest_end, src_start, 128, 128);
			last_dir = 12;
			delta_pos.y = 0;
			prev_global_pos.y = global_pos.y;
	
		}
		

		consoleClear();
		iprintf("Pos: %i %i\n", global_pos.x, global_pos.y);
		iprintf("Prev: %i %i\n", prev_global_pos.x, prev_global_pos.y);
		iprintf("Delta: %i %i\n", delta_pos.x, delta_pos.y);
		iprintf("Dest Start: %i %i\n", dest_start.x, dest_start.y);
		iprintf("Dest End: %i %i\n", dest_end.x, dest_end.y);
		iprintf("Borders: N%i, S%i, E%i, W%i\n", local_borders.sides.n, local_borders.sides.s, local_borders.sides.e, local_borders.sides.w);
		iprintf("Last dir: %i\n", last_dir);
		bgUpdate();

		swiWaitForVBlank();

	};

	
	

}
