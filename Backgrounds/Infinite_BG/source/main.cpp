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
template <typename T>
union Boundary{
	struct {
		T n;
		T s;
		T e;
		T w;
	} sides;
	T mem[4];
};

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

template <typename T>
T math_mod(T a, T b){
	if(b < 0) return 0;
	T rem = a % b;
	rem = (rem < 0) ? (b + rem) : rem;
	return rem;
}

// Draws a stripe of predefined dimensions from ROM to VRAM 
//NOTE, The Mod calculation here might be redundant now, now that we actually modulo in the main loop
void drawStripe(int bg_id, u16* src_tilemap, Vector_2D<u16> dest_start, Vector_2D<u16> dest_end, Vector_2D<int> src_start, Vector_2D<int> src_dim){
	u32 stripe_width  =  dest_start.x < dest_end.x ? dest_end.x - dest_start.x :
		dest_end.x + NO_TILES - (dest_start.x & (NO_TILES - 1));
   	u32 stripe_height = dest_start.y < dest_end.y ? dest_end.y - dest_start.y :  
		dest_end.y + NO_TILES - (dest_start.y & (NO_TILES - 1));			
	for(int i = 0; i < stripe_width; i++){
		for(int j = 0; j < stripe_height; j++){
			// For single bytes a CPU copy is faster. Assumes 8bpp
			// Converts the normal mapping to the 32x32 chunck mapping of tile_idx values loaded by the 2D engine from the VRAM
			u16 addr = convert_coords(dest_start.x + i, dest_start.y + j);
			// Here we assume the tilemap in ROM follows the normal mapping
			// Wee need to mod the X and Y directions seperately to be able to wrap around in each direction
			int src_addr = ((src_start.x + i) % src_dim.x) + ((src_start.y + j) % src_dim.y) * src_dim.x;
			*(bgGetMapPtr(bg_id) + addr) = (u16)src_tilemap[src_addr];
		}
	}
}


int calc_wrapped_delta(int curr, int prev, int dim){
	int delta = curr - prev;
	delta = math_mod<int>(delta + (dim >> 1), dim) - (dim >> 1);
	return delta;
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	consoleDemoInit();
	// Pritn Debug outputs to NoCash
	consoleDebugInit(DebugDevice_NOCASH);
	defaultExceptionHandler();
	// Sets the Video mode flag to mode 0 for the main screen (4xText BG)
	videoSetMode(MODE_0_2D);

	// Assign VRAM A to Main Engine BG
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);

	// See page 101
	// Set BG Flag registers for expected data
	// Returns the id of the BG, which is a libnds concept used for BG functions 
	int bg = bgInit(0, BgType_Text8bpp, BgSize_T_512x512, 0, 1); 
	
	// For debugging we just override the original palette and tiles for now
	// override the tiles to point to a simple color value each
	static u8 i0i1_tile_data[126] = {0};
	memset((void*)i0i1_tile_data + 64, 0x01, 64);
	dmaCopy((void *)i0i1_tile_data, bgGetGfxPtr(bg), 128);

	// Write the palette data to contain a simple color value
	*(BG_PALETTE) = (0x1F << RED) | (0x0A << GREEN) | (0x00 << BLUE);
	*(BG_PALETTE + 1) = (0x00 << RED) | (0x1F << GREEN) | (0x1F << BLUE);  
	

	// Init Global and Local positons
	// Global pos describes the position on the full tilemap
	Vector_2D<u16> global_pos = {SCREEN_CENTER_X, SCREEN_CENTER_Y};
	// Previous global pos at the last load to calculate delta
	Vector_2D<u16> prev_global_pos = global_pos; 
	// Local position describes the position on the VRAM, should just be the global pos mod' with the BG size
	Vector_2D<u16> local_pos = {0, 0};
	// BGScroll describes the scrolling position and is derived from the scrolling position (may be redundant to local pos)
	Vector_2D<u16> bg_scroll = {0,0};
	// Holds the Position of the current center tile	
	Vector_2D<u16> center_tile = {0,0};

	Vector_2D<u16> dest_start = {0, 0};
	Vector_2D<u16> dest_end = {(SCREEN_SIZE_X >> 3) + 1, (SCREEN_SIZE_Y >> 3) + 1};
	Vector_2D<int> src_start = {0, 0};
	Vector_2D<int> src_dim = {128, 128};
	// Define all local borders, initialized during loop
	Boundary<u16> local_borders = {0, 0, 0, 0};
	Boundary<int> global_borders = {0, 0, 0, 0};
	Boundary<int> prev_global_borders = global_borders;
	Boundary<int> border_delta = global_borders;
	// Check if The Screen has rolled over around the ROM map, for loop over calculations
	// Holds the absolute bounds which is either INT32_MAX or the size of the ROM map 
	//NOTE The issue here is, that while we support very big BGs, they dont allow looping around infinitely
	drawStripe(bg, e_map, dest_start, dest_end, src_start, src_dim);

	//Just used for Debugging
	int last_dir = 0;
	// Supports a Max scroll speed of 8 pixels (Could easily support more, if we take the right delta value)
	u16 scroll_speed = 3;
	
	u16 limit_scroll = 1;

	center_tile = {global_pos.x >> 3, global_pos.y >> 3};

	global_borders.sides.n = (center_tile.y - SCREEN_CENTER_Y_TILES - 1) & (src_dim.y - 1); 
	global_borders.sides.s = (center_tile.y + SCREEN_CENTER_Y_TILES + 1) & (src_dim.y - 1);
	global_borders.sides.e = (center_tile.x + SCREEN_CENTER_X_TILES + 1) & (src_dim.x - 1);
	global_borders.sides.w = (center_tile.x - SCREEN_CENTER_X_TILES - 1) & (src_dim.x - 1);

	prev_global_borders.sides = global_borders.sides;

	while(1){
		
		center_tile = {global_pos.x >> 3, global_pos.y >> 3};
		
		global_borders.sides.n = (center_tile.y - SCREEN_CENTER_Y_TILES - 1) & (src_dim.y - 1); 
		global_borders.sides.s = (center_tile.y + SCREEN_CENTER_Y_TILES + 1) & (src_dim.y - 1);
		global_borders.sides.e = (center_tile.x + SCREEN_CENTER_X_TILES + 1) & (src_dim.x - 1);
		global_borders.sides.w = (center_tile.x - SCREEN_CENTER_X_TILES - 1) & (src_dim.x - 1);
		
		scanKeys();
			
		u16 kHeld = keysHeld();
		// We need to watch out here, so the scroll speed does not vary and jump over the threshold anyway
		// It does some wierd shaking when we are at the border and hold L + R or U + D
		//TODO Scroll limit should be determined by global_pos and screen sizes instead of the border 
        if (kHeld & KEY_UP && (!limit_scroll || (global_pos.y - SCREEN_CENTER_Y > 0))) global_pos.y -= (1 << scroll_speed);
        else if (kHeld & KEY_DOWN && (!limit_scroll || (global_pos.y + SCREEN_CENTER_Y < (src_dim.y << 3)))) global_pos.y += (1 << scroll_speed);
        if (kHeld & KEY_LEFT && (!limit_scroll || (global_pos.x - SCREEN_CENTER_X > 0))) global_pos.x -= (1 << scroll_speed);
        else if (kHeld & KEY_RIGHT && (!limit_scroll || (global_pos.x + SCREEN_CENTER_X < (src_dim.x << 3)))) global_pos.x += (1 << scroll_speed);


		// Adjust for VRAM size
		for(int i = 0; i < 4; i++) 
			local_borders.mem[i] = static_cast<u16>(math_mod<int>(global_borders.mem[i], NO_TILES));
			
		// Backgroud update logic 
		bgSetScroll(bg, global_pos.x - SCREEN_CENTER_X, global_pos.y - SCREEN_CENTER_Y);
		
		border_delta.sides.e = calc_wrapped_delta(global_borders.sides.e, prev_global_borders.sides.e, src_dim.x);
		border_delta.sides.w = calc_wrapped_delta(global_borders.sides.w, prev_global_borders.sides.w, src_dim.x);


		// At scroll speeds > 8px, we have offset errors by 1 when scrolling east and south
		// My idea is, that this is because, for this direction we need to adjust the end part as well
		if(border_delta.sides.e > 0){
			dest_end.x = local_borders.sides.e;
			dest_start.x = (dest_end.x - 1) & (NO_TILES - 1); 
			dest_start.x = static_cast<u16>(math_mod<int>((int)dest_end.x - border_delta.sides.e, NO_TILES)); 

			dest_start.y = local_borders.sides.n; 
			dest_end.y = local_borders.sides.s; 

			// We might not need math_mod, regular "& mod should suffice, if we never go below 0"
			//FIX For east and south, we need to subtract the delta, as we write from left to right
			// for w this is not needed, as the border already starts from the left
			src_start.x = math_mod<int>(global_borders.sides.e - border_delta.sides.e, src_dim.x);
			src_start.y = math_mod<int>(global_borders.sides.n, src_dim.y);


			drawStripe(bg, e_map, dest_start, dest_end, src_start, src_dim);
			last_dir = 3;
		}
		else if(border_delta.sides.w < 0){
			dest_start.x = local_borders.sides.w; 
			dest_end.x = (dest_start.x + 1) & (NO_TILES - 1);
			dest_end.x = static_cast<u16>(math_mod<int>((int)dest_start.x - border_delta.sides.w, NO_TILES)); 

			dest_start.y = local_borders.sides.n; 
			dest_end.y = local_borders.sides.s; 

			src_start.x = math_mod<int>(global_borders.sides.w, src_dim.x);
			src_start.y = math_mod<int>(global_borders.sides.n, src_dim.y);			

			drawStripe(bg, e_map, dest_start, dest_end, src_start, src_dim);
			last_dir = 9;
	
		}

		border_delta.sides.n = calc_wrapped_delta(global_borders.sides.n, prev_global_borders.sides.n, src_dim.y);
		border_delta.sides.s = calc_wrapped_delta(global_borders.sides.s, prev_global_borders.sides.s, src_dim.y);

		if(border_delta.sides.s > 0){
			dest_end.y = local_borders.sides.s;
			dest_start.y = (dest_end.y - 1) & (NO_TILES - 1);
			dest_start.y = static_cast<u16>(math_mod<int>((int)dest_end.y - border_delta.sides.s, NO_TILES)); 

			dest_start.x = local_borders.sides.w; 
			dest_end.x = local_borders.sides.e; 

			src_start.x = math_mod<int>(global_borders.sides.w, src_dim.x);
			src_start.y = math_mod<int>(global_borders.sides.s - border_delta.sides.s, src_dim.y);

			drawStripe(bg, e_map, dest_start, dest_end, src_start, src_dim);
			last_dir = 6;
	
		}
		else if(border_delta.sides.n < 0){
			dest_start.y = local_borders.sides.n;
			dest_end.y = (dest_start.y + 1) & (NO_TILES - 1);
			dest_end.y = static_cast<u16>(math_mod<int>((int)dest_start.y - border_delta.sides.n, NO_TILES)); 

			dest_start.x = local_borders.sides.w; 
			dest_end.x = local_borders.sides.e; 

			src_start.x = math_mod<int>(global_borders.sides.w, src_dim.x);
			src_start.y = math_mod<int>(global_borders.sides.n, src_dim.y);

			drawStripe(bg, e_map, dest_start, dest_end, src_start, src_dim);
			last_dir = 12;
		}

		for(int i = 0; i < 4; i++)
			prev_global_borders.mem[i] = global_borders.mem[i];
		

		consoleClear();
		iprintf("Pos: %i %i\n", global_pos.x, global_pos.y);
		iprintf("Dest Start: %i %i\n", dest_start.x, dest_start.y);
		iprintf("Dest End: %i %i\n", dest_end.x, dest_end.y);
		iprintf("LB: N%i, S%i, E%i, W%i\n", local_borders.sides.n, local_borders.sides.s, local_borders.sides.e, local_borders.sides.w);
		iprintf("GB: N%i, S%i, E%i, W%i\n", global_borders.sides.n, global_borders.sides.s, global_borders.sides.e, global_borders.sides.w);
		iprintf("P.GB: N%i, S%i, E%i, W%i\n", prev_global_borders.sides.n, prev_global_borders.sides.s, prev_global_borders.sides.e, prev_global_borders.sides.w);
		iprintf("Bound.D N%i, S%i, E%i, W%i\n", border_delta.sides.n, border_delta.sides.s, border_delta.sides.e, border_delta.sides.w);
		iprintf("Last dir: %i\n", last_dir);
		bgUpdate();

		swiWaitForVBlank();

	};

}
