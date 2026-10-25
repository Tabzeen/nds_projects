#include <nds.h>
#include <stdio.h> 
#include <string.h>
#include <array>
#include "CharData.h"

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


void DialogueEngine::write_char_to_buffer(u32 char_idx, u8 color, bool shadow){

    // Dont write invalid Character idx values
    sassert(char_idx < char_data->char_cnt, "Invalid Character Accessed!");
    //fprintf(stderr, "Addr of Char data: %p \n", char_wram_row_data);
    int arr_steps = char_data->font_dim / 8;

    // Perform three passes:
    // The first one shifts x by +1 to draw a shadow to the right
    // The second one shifts y by +1 to draw a shadow below
    // The third pass prints the actual character
    for(int pass = 0; pass < 3; pass++){

        bool shadow_off_x = (pass == 0);
        bool shadow_off_y = (pass == 1);

        if(!shadow && pass < 2)
            continue;

        // Iterate over Rows
        for(int i = 0; i < char_data->font_dim; i++){
            const u8* src_addr = &char_data->pixels[char_idx * char_data->char_size_u8 + arr_steps * i];

            // Add start offset to iterator
            u8 offset_i = i + text_meta.cursor_stride_y + shadow_off_y;

            // Determine Tile Row Stride
            // Outer Row offset
            u8 row_stride = offset_i >> 3;
            // Inner row offset
            u8 row_off = offset_i % 8;

            // Font size is just an 8b int which represents the "actual" font size and is not the real font size
            //fprintf(stderr, "reading tile data of pointer %p in iteration %i \n", (void*)src_addr, i);
            // 8px sections of a 16px Character
            u8 tile_a = *(src_addr);
            u8 tile_b = *(src_addr + 1);

            // assemble into a larger u32 Char
            u32 ext_char = ((static_cast<u32>(tile_a) << 24) | (static_cast<u32>(tile_b) << 16));

            // shift by the inner tile offset
            ext_char = ext_char >> (text_meta.cursor_stride_x + shadow_off_x);

            // iterate over cols 
            for(int j = 0 ; j < 3; j++){

                // extend 8b -> 32b via LUT and store in 3 registers 
                // Reverse order for writing to VRAM
                char_wram_row_data[j] = 
                    lut_1b_to_4b[static_cast<u8>(ext_char >> (tile_size_u32 * (3 - j)))];

                // Set the override mask, to override the shadow values
                u32 mask = char_wram_row_data[j] * 0xF;

                // Set Character color, or fix to shadow color
                char_wram_row_data[j] *= pass < 2 ? 1 : color;

                // Merge with the tileset WRAM copy
                tileset_copy[row_stride * tile_size_u32*3 + row_off + 8*j ] = 
                    (tileset_copy[row_stride * tile_size_u32*3 + row_off + 8*j] & ~mask) | char_wram_row_data[j];
            }
        }
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

    if((y_start >= kTilesPerCol) || (y_end >= kTilesPerCol) || (x_start >= kTilesPerRow) || (x_end >= kTilesPerRow))
        sassert(0, "Text area out of tilemap bounds!");    

    if((y_start >= y_end) || (x_start >= x_end))
        sassert(0, "Invalid text area size!");

    // Populate the metadata
    text_meta.y_start = y_start;
    text_meta.y_end = y_end;
    text_meta.x_start = x_start;
    text_meta.x_end = x_end;
    // This needs bounds checking
    text_meta.x_width = x_end - x_start;
    text_meta.y_width = y_end - y_start;


    clear_tilemap();
    clear_tileset();

    u16 tile_idx_acc = 1;
    for(int i = text_meta.y_start; i < text_meta.y_end; i++){
        for(int j = text_meta.x_start; j < text_meta.x_end; j++){
            *(bg_map_ptr + i * 32 + j) = tile_idx_acc;
            tile_idx_acc++;         
        }
    }
}


// Accepts internal char idx. Does not know the ASCII representation of the character
void DialogueEngine::push_char(int idx, bool advance){
    sassert(idx < char_data->char_cnt, "Invalid Char Idx accessed!");

    copy_rows_to_buffer();
    //write_char_to_buffer(idx);
    write_char_to_buffer(idx, 2, 1);
    copy_buffer_to_rows();

    if(advance){
        u8 idx_stride = char_data->strides[idx] + text_meta.kerning;
        // We do a somewhat unnecessary convert here just to convert it back. We might wanna adjust that
        u8 target_stride_x = idx_stride + 8 * text_meta.cursor_tile_x + text_meta.cursor_stride_x;   
        u8 target_y = 8 * text_meta.cursor_tile_y + text_meta.cursor_stride_y;
        set_cursor_position(target_stride_x, target_y);
    }
}



// TODO, we need to export a decoder function here, which maps an ASCII string (or any index for what it matters)
// To the internal Representation
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

    if(x / 8 >= text_meta.x_width || y / 8 >= text_meta.y_width)
        sassert(0, "Invalid cursor position!");

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

// Clears the tileset entires. Does not perform a reconstruction
// May be called from outside to clear the canvas 
void DialogueEngine::clear_tileset(){

    // Clear the entire tileset
    u32 zero_hw = 0;
    swiCopy(&zero_hw, bg_tile_ptr, (kTilesPerCol * kTilesPerRow * 8) | COPY_MODE_WORD | COPY_MODE_FILL);
}

// Clears the tilemap enties. Does not perform reconstruction
// Should only be called by the initialization and never be used from the outside directly 
void DialogueEngine::clear_tilemap(){

    // Clear entire tilemap first
    u32 zero_hw = 0;
    swiCopy(&zero_hw, bg_map_ptr, (kTilesPerCol * kTilesPerRow  / 2) | COPY_MODE_WORD | COPY_MODE_FILL);

}


// Approximates from cursor position and inner tile stride if there is still space in a given row
// Does not accept a character, but just checks if a 16x16 character fits inside the area
bool DialogueEngine::get_row_full(){
    return (text_meta.x_width - text_meta.cursor_tile_x) > 2;  
}
