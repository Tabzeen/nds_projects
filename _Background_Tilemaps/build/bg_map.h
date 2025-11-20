
//{{BLOCK(bg_map)

//======================================================================
//
//	bg_map, 256x512@8, 
//	+ palette 256 entries, not compressed
//	+ 6 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x64 
//	Total size: 512 + 384 + 4096 = 4992
//
//	Time-stamp: 2025-05-23, 21:02:16
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_BG_MAP_H
#define GRIT_BG_MAP_H

#define bg_mapTilesLen 384
extern const unsigned int bg_mapTiles[96];

#define bg_mapMapLen 4096
extern const unsigned short bg_mapMap[2048];

#define bg_mapPalLen 512
extern const unsigned short bg_mapPal[256];

#endif // GRIT_BG_MAP_H

//}}BLOCK(bg_map)
