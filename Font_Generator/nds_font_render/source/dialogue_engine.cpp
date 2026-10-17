#include <nds.h>
#include <stdio.h> 
#include <string.h>

#include <array>
#include <vector> 

#include "dialogue_engine.h"


constexpr auto kTilesPerRow = 32;

// We need to reverse inner byte order, as the VRAM reads 
// the Byte LSb first
// Maybe this could be done on the encoding HW, but a LUT is free, so this is fine for the moment
constexpr uint32_t expand_byte(uint8_t byte) {
    uint32_t val = 0;
    for (int bit = 0; bit < 8; bit++) {
        if (byte & (1 << ( 7 - bit))) {
            val |= (0x1u << (bit * 4));
        }
    }
    return val;
}

constexpr auto make_lut() {
    std::array<uint32_t, 256> table{};
    for (int i = 0; i < 256; ++i) {
        table[i] = expand_byte(static_cast<uint8_t>(i));
    }
    return table;
}

// Baked into read-only binary at compile time:
constexpr auto lut_1b_to_4b = make_lut();


// The Dialogue engine handling everything from loading tiles from memory to drawing the tiles into VRAM
// Reserves memory once at initialization 

void DialogueEngine::build_ascii_lut(){

}

// Note: this needs some way for bondary checking so we do not write out of bounds,
// Currently it can do very scary stuff

// We can constraint writes, by simply just checing the write address range
void DialogueEngine::write_char_to_buffer(int char_idx){

    fprintf(stderr, "Addr of Char data: %p \n", char_wram_row_data);
    int arr_steps = font_dim / 8;
    for(int i = 0; i < char_data->font_dim; i++){

        const u8* src_addr = &char_data->font_data[char_idx * font_data_elem_size + arr_steps * i];

        // Font size is just an 8b int which represents the "actual" font size and is not the real font size
        fprintf(stderr, "reading tile data of pointer %p in iteration %i \n", (void*)src_addr, i);
        // 8px sections of a 16px Character
        u8 tile_a = *(src_addr);
        u8 tile_b = *(src_addr + 1);

        // assemble into a larger u32 Char
        u32 ext_char = ((static_cast<u32>(tile_a) << 24) | (static_cast<u32>(tile_b) << 16));
        fprintf(stderr, "Tile data in iteration %i %lx \n", i, ext_char);
        // shift by the inner tile offset
        ext_char = ext_char >> inner_tile_offset;
        fprintf(stderr, "Tile data in iteration %i after shift %lx \n", i, ext_char);
        
        // extend 8b -> 32b via LUT and store in 3 registers 
        char_wram_row_data[0] = lut_1b_to_4b[static_cast<u8>(ext_char >> 24)];
        char_wram_row_data[1] = lut_1b_to_4b[static_cast<u8>(ext_char >> 16)];
        char_wram_row_data[2] = lut_1b_to_4b[static_cast<u8>(ext_char >> 8)];

        // Determine Tile Row Stride
        u8 row_stride = i >> 3;
        u8 row_off = i % 8;

        // OR with the tileset WRAM copy
        tileset_copy[row_stride * 8*3 + row_off]      |= char_wram_row_data[0];
        tileset_copy[row_stride * 8*3 + row_off + 8]  |= char_wram_row_data[1];
        tileset_copy[row_stride * 8*3 + row_off + 16] |= char_wram_row_data[2] | 0x11111111;

    }   
}


/* 
    Accepts a cursor x and y position and copies a 3x3 tile section from VRAM to WRAM
    Utilizes the initialized text_meta struct. 
    Expects the Tilemap to be initialized to the set boundary
*/ 
void DialogueEngine::copy_rows_to_buffer(int cursor_x, int cursor_y){

    // We immediately add one to the curstor postion to skip the zero tile
    cursor_x += 1;

    // We need to recast the pointer to 32b, as we store full tile rows
    u32* bg_base_offset = reinterpret_cast<u32*>(bgGetGfxPtr(bg_idx));

    // Offset of the first target tile
    // Always shifted by one, as the zeroth tile is empty
    // It remains 4B aligned as it is offset by 32b
    iprintf("Bg base offset: %p \n", (void*)bg_base_offset);

    size_t cursor_offset = tile_size_u32 * (cursor_x + cursor_y * text_meta.x_width);
    u32* cpy_offset = bg_base_offset + cursor_offset; 

    // Copy the 3 adjacent tiles from the VRAM to WRAM from the first row 
    swiCopy(cpy_offset, tileset_copy, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);
    // Copy the second row into tileset memory
    swiCopy(cpy_offset + tile_size_u32 * text_meta.x_width , tileset_copy + tile_size_u32 * 3, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);
    // Copy the third row into tileset memory
    swiCopy(cpy_offset + tile_size_u32 * text_meta.x_width * 2, tileset_copy + tile_size_u32 * 3 * 2, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);

}

void DialogueEngine::copy_buffer_to_rows(int cursor_x, int cursor_y){

    // We immediately add one to the curstor postion to skip the zero tile
    cursor_x += 1;

    u32* bg_base_offset = reinterpret_cast<u32*>(bgGetGfxPtr(bg_idx));

    iprintf("Bg base offset: %p \n", (void*)bg_base_offset);

    size_t cursor_offset = tile_size_u32 * (cursor_x + cursor_y * text_meta.x_width);
    u32* cpy_offset = bg_base_offset + cursor_offset; 

    swiCopy(tileset_copy, cpy_offset, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);
    swiCopy(tileset_copy + tile_size_u32 * 3, cpy_offset + tile_size_u32 * text_meta.x_width, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);
    swiCopy(tileset_copy + tile_size_u32 * 3 * 2, cpy_offset + tile_size_u32 * text_meta.x_width * 2, tile_size_u32 * 3 | COPY_MODE_COPY | COPY_MODE_WORD);
}

DialogueEngine::DialogueEngine(const CharData* char_data_src, int bg_idx){            

    this->char_data = char_data_src;
    
    // Reserve WRAM data for 16x16 fonts + overhead
    char_wram_row_data = new u32[2 + 1];
    // Reserve Tilemap copy for a 3x2 block of 8x8 tiles at 4bpp
    tileset_copy = new u32[(2 + 1) * 16];

    this->bg_idx = bg_idx;

    this->text_meta = {};

}

DialogueEngine::~DialogueEngine(){
    delete[] char_wram_row_data;
    delete[] tileset_copy;
}


void DialogueEngine::init_map_bounds(u8 y_start, u8 y_end, u8 x_start, u8 x_end){

    // Populate the metadata
    text_meta.y_start = y_start;
    text_meta.y_end = y_end;
    text_meta.x_start = x_start;
    text_meta.x_end = x_end;
    // This needs bounds checking
    text_meta.x_width = x_end - x_start;
    text_meta.y_width = y_end - y_start;

    // Libnds only returns the local offset of a BG, we need to add the 64KB global offset as well
    u16* base_map_offset = bgGetMapPtr(bg_idx) + 0x8000;

    // Flush the VRAM Tilemap with zeros
    // Each entry is 2B wide
    // Lower section of the 32x32 tiles are in overscan
    u32 zero_hw = 0;
    swiCopy(&zero_hw, base_map_offset, (24*32 / 2) | COPY_MODE_WORD | COPY_MODE_FILL);
    iprintf("Base Map offset: %p \n", (void*)base_map_offset);

    // Accumulator for tile indices. Starts at 1 as zero tile is blank
    u16 tile_idx_acc = 1;
    for(int i = y_start; i < y_end; i++){
        for(int j = x_start; j < x_end; j++){
            *(base_map_offset + i * 32 + j) = tile_idx_acc;
            tile_idx_acc++;         
        }
    }
}

void DialogueEngine::push_char(int idx){
    copy_rows_to_buffer(text_meta.cursor_tile_x, text_meta.cusor_tile_y);
    write_char_to_buffer(17);
    copy_buffer_to_rows(text_meta.cursor_tile_x, text_meta.cusor_tile_y);

}

void DialogueEngine::clear(){

}