#include <nds.h>
#include <stdio.h> 
#include <string.h>

#include <array>
#include <vector> 

#include "dialogue_engine.h"


constexpr auto kTilesPerRow = 32;
constexpr auto kTilesPerCol = 24;

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


// Note: this needs some way for bondary checking so we do not write out of bounds,
// Currently it can do very scary stuff

// We can constraint writes, by simply just checing the write address range
void DialogueEngine::write_char_to_buffer(int char_idx){

    //fprintf(stderr, "Addr of Char data: %p \n", char_wram_row_data);
    int arr_steps = font_dim / 8;
    for(int i = 0; i < char_data->font_dim; i++){

        const u8* src_addr = &char_data->font_data[char_idx * font_data_elem_size + arr_steps * i];

        // Font size is just an 8b int which represents the "actual" font size and is not the real font size
        //fprintf(stderr, "reading tile data of pointer %p in iteration %i \n", (void*)src_addr, i);
        // 8px sections of a 16px Character
        u8 tile_a = *(src_addr);
        u8 tile_b = *(src_addr + 1);

        // assemble into a larger u32 Char
        u32 ext_char = ((static_cast<u32>(tile_a) << 24) | (static_cast<u32>(tile_b) << 16));
        //fprintf(stderr, "Tile data in iteration %i %lx \n", i, ext_char);
        // shift by the inner tile offset
        ext_char = ext_char >> text_meta.cursor_stride_x;
        //fprintf(stderr, "Tile data in iteration %i after shift %lx \n", i, ext_char);
        
        // extend 8b -> 32b via LUT and store in 3 registers 
        // Reverse order for writing to VRAM
        char_wram_row_data[0] = lut_1b_to_4b[static_cast<u8>(ext_char >> (tile_size_u32 * 3))];
        char_wram_row_data[1] = lut_1b_to_4b[static_cast<u8>(ext_char >> (tile_size_u32 * 2))];
        char_wram_row_data[2] = lut_1b_to_4b[static_cast<u8>(ext_char >>  tile_size_u32)];

        // Add start offset to iterator
        u8 offset_i = i + text_meta.cursor_stride_y;

        // Determine Tile Row Stride
        // Outer Row offset
        u8 row_stride = offset_i >> 3;
        // Inner row offset
        u8 row_off = offset_i % 8;

        // OR with the tileset WRAM copy
        tileset_copy[row_stride * tile_size_u32*3 + row_off      ] |= char_wram_row_data[0];
        tileset_copy[row_stride * tile_size_u32*3 + row_off + 8  ] |= char_wram_row_data[1];
        tileset_copy[row_stride * tile_size_u32*3 + row_off + 16 ] |= char_wram_row_data[2];

    }   
}


/* 
    Accepts a cursor x and y position and copies a 3x3 tile section from VRAM to WRAM
    Utilizes the initialized text_meta struct. 
    Expects the Tilemap to be initialized to the set boundary
*/ 
void DialogueEngine::copy_rows_to_buffer(){

    // We need to recast the pointer to 32b, as we store full tile rows
    u32* bg_base_offset = reinterpret_cast<u32*>(bg_tile_ptr);

    // We immediately add one to the curstor postion to skip the zero tile
    size_t cursor_offset = tile_size_u32 * (text_meta.cursor_tile_x + 1 + text_meta.cursor_tile_y * text_meta.x_width);
    u32* cpy_offset = bg_base_offset + cursor_offset; 

    // Calculate the max offset for memory safety
    u32* bg_end = bg_base_offset + text_meta.x_width * text_meta.y_width * tile_size_u32;  

    const u32 cpy_size = tile_size_u32 * 3;

    for(int i = 0; i < 3; i++){
        u32* src = cpy_offset + i * tile_size_u32 * text_meta.x_width;
        u32* dest = tileset_copy + i * cpy_size;

        if(src + cpy_size <= bg_end)
            swiCopy(src, dest, cpy_size | COPY_MODE_COPY | COPY_MODE_WORD);
    }

}

void DialogueEngine::copy_buffer_to_rows() {
    u32* bg_base_offset = reinterpret_cast<u32*>(bg_tile_ptr);

    size_t cursor_offset = tile_size_u32 * (text_meta.cursor_tile_x + 1 + text_meta.cursor_tile_y * text_meta.x_width);
    u32* cpy_offset = bg_base_offset + cursor_offset; 

    u32* bg_end = bg_base_offset + text_meta.x_width * text_meta.y_width * tile_size_u32;  

    const u32 cpy_size = tile_size_u32 * 3;

    for (int i = 0; i < 3; i++) {
        u32* src  = tileset_copy + i * cpy_size;
        u32* dest = cpy_offset + i * tile_size_u32 * text_meta.x_width;

        if (dest + cpy_size <= bg_end) {
            swiCopy(src, dest, cpy_size | COPY_MODE_COPY | COPY_MODE_WORD);
        }
    }
}

DialogueEngine::DialogueEngine(const CharData* char_data_src, u16* bg_map_ptr, u16* bg_tile_ptr){            

    this->bg_map_ptr = bg_map_ptr;
    this->bg_tile_ptr = bg_tile_ptr;

    this->char_data = char_data_src;
    
    // Reserve WRAM data for 16x16 fonts + overhead
    char_wram_row_data = new u32[3];
    // Reserve Tilemap copy for a 3x3 block of 8x8 tiles at 4bpp
    tileset_copy = new u32[3 * 3 * tile_size_u32];


    this->text_meta = {};

}

DialogueEngine::~DialogueEngine(){
    delete[] char_wram_row_data;
    delete[] tileset_copy;
}


/*
    Initializes the field in which the data is written to
*/
void DialogueEngine::init_map_bounds(u8 y_start, u8 y_end, u8 x_start, u8 x_end){

    // Populate the metadata
    text_meta.y_start = y_start;
    text_meta.y_end = y_end;
    text_meta.x_start = x_start;
    text_meta.x_end = x_end;
    // This needs bounds checking
    text_meta.x_width = x_end - x_start;
    text_meta.y_width = y_end - y_start;

    clear();
}



void DialogueEngine::push_char(int idx, bool advance){
    copy_rows_to_buffer();
    write_char_to_buffer(idx);
    copy_buffer_to_rows();

    if(advance){
        u8 idx_stride = char_data->char_meta[idx].advance;
        // We do a somewhat unnecessary convert here just to convert it back. We might wanna adjust that
        u8 target_stride_x = idx_stride + 8 * text_meta.cursor_tile_x + text_meta.cursor_stride_x;   
        u8 target_y = 8 * text_meta.cursor_tile_y + text_meta.cursor_stride_y;
        set_cursor_position(target_stride_x, target_y);
    }
}




void DialogueEngine::push_hello(){
    u8 arr[] = {39, 68, 75, 75, 78, 17, 54, 78, 81, 75, 67};
    for(int i = 0; i < sizeof(arr); i++){
        push_char(arr[i], 1);
    }
}

void DialogueEngine::push_ascii_string(char* text){
    int i = 0;
    while(text[i] != '\0'){
        if(i == ' '){
            
        }
        else {
            //fprintf("%c \n", text[i])
            push_char(text[i] - 0x21, true);    
        };
        i++;
    };
}


void DialogueEngine::set_cursor_position(u8 x, u8 y){

    u8 tile_x = x >> 3;
    u8 stride_x = x % 8;

    u8 tile_y = y >> 3;
    u8 stride_y = y % 8;

    //TODO add some safety checks
    
    text_meta.cursor_tile_x = tile_x;
    text_meta.cursor_stride_x = stride_x;
    text_meta.cursor_tile_y = tile_y;
    text_meta.cursor_stride_y = stride_y;
}

void DialogueEngine::clear(){

    // Clear entire tilemap first
    u32 zero_hw = 0;
    swiCopy(&zero_hw, bg_map_ptr, (kTilesPerCol * kTilesPerRow  / 2) | COPY_MODE_WORD | COPY_MODE_FILL);
    iprintf("Base Map offset: %p \n", (void*)bg_map_ptr);

    // Clear the entire tileset
    swiCopy(&zero_hw, bg_tile_ptr, (kTilesPerCol * kTilesPerRow * 8) | COPY_MODE_WORD | COPY_MODE_FILL);

    // Rebuild the tilemap
    // Accumulator for tile indices. Starts at 1 as zero tile is blank
    u16 tile_idx_acc = 1;
    for(int i = text_meta.y_start; i < text_meta.y_end; i++){
        for(int j = text_meta.x_start; j < text_meta.x_end; j++){
            *(bg_map_ptr + i * 32 + j) = tile_idx_acc;
            tile_idx_acc++;         
        }
    }
}