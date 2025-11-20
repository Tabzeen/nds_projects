
//{{BLOCK(CharSet)

//======================================================================
//
//	CharSet, 256x256@8, 
//	+ palette 256 entries, not compressed
//	+ 141 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 9024 + 2048 = 11584
//
//	Time-stamp: 2025-10-06, 16:17:35
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_CHARSET_H
#define GRIT_CHARSET_H

#define CharSetTilesLen 9024
extern const unsigned int CharSetTiles[2256];

#define CharSetMapLen 2048
extern const unsigned short CharSetMap[1024];

#define CharSetPalLen 512
extern const unsigned short CharSetPal[256];

#endif // GRIT_CHARSET_H

//}}BLOCK(CharSet)
