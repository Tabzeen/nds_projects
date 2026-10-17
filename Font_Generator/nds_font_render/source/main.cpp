
#include <nds.h>
#include <stdio.h>

#include "dialogue_engine.h"

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	consoleDemoInit();
	consoleDebugInit(DebugDevice_NOCASH);

	// Set the DISPCNT register to
	// Graphics Display mux'd to the 2D Engines
	// BG Mode [2-0] set to 0 for BG 0
	// Set Tile data to the first 64KB block and Screen data to the second 64KB block
	videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_CHAR_BASE(0) | DISPLAY_SCREEN_BASE(1));

	// Set WVRAMCNT -> VRAM-A MST to 1 which corresponds to Graphics engine A's BG assignment and mapped to 0x06000000
	// Which is the fist slot, taking up the first 128KB of the 512KB of VRAM
	vramSetBankA(VRAM_A_MAIN_BG);

	// Create a bg libnds struct which stores configured bg params and inits the bg
	// Writes to the BGxCNT register
	// layer = Prioirty, 4bpp = Color Mode, Size = Screen Size, Map and tile base = Chapter 5.2.3.1
	// Important: bgInit does not allow setting seperate global block offsets of Screen and Char data
	// The "Global offset" of 64KB of each is given by DSPCNT in [29:27] and [26:24]
	// IMPORTANT: Libnds does actually not track the DISPCNT register and just always
	// returns the pointer offset from 0x0600 0000, so if we map something to another 
	// Screen block, it returns an invalid address
	int main_bg = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 0);

	*(BG_PALETTE + 1) = 0xFFFF;
	*(BG_PALETTE + 2) = 0xFF00;


	const CharData* char_data = get_char_data();

	DialogueEngine dialogue_engine(char_data, main_bg);

	dialogue_engine.init_map_bounds(0, 10, 0, 15);
	dialogue_engine.push_char(0);


	iprintf("Hello World\n");
	while(1){
		// Wait for blanking interval
		swiWaitForVBlank();

	};
}
