#pragma once

#include <nds.h>
#include <array>
#include <cstdint>
#include "CharData.h"


constexpr u8 tile_size_u32 = 8;

class DialogueEngine {
public:
    DialogueEngine(const CharData* char_data_src, u16* bg_map_ptr, u16* bg_tile_ptr);
    ~DialogueEngine();

    void init_map_bounds(u8 y_start, u8 y_end, u8 x_start, u8 x_end);
    void push_char(int idx, bool advance);
    void push_hello();
    void set_cursor_position(u8 x, u8 y);
    void push_ascii_string(char* text);
    void clear_tileset();
    void clear_tilemap();

private:
    struct TextMeta {
        u8 y_start;
        u8 y_end;
        u8 x_start;
        u8 x_end;
        u8 x_width;
        u8 y_width;
        u8 cursor_tile_x;
        u8 cursor_tile_y;
        u8 cursor_stride_x;
        u8 cursor_stride_y; 
        u8 kerning;         // Additional kerning added to the encoded char. advance    
        u8 space_width;     // How many x pixels to advance on a space 
        u8 line_advance;    // How many y pixels to advance on \n 
    };

    // Private helper methods
    void write_char_to_buffer(u32 char_idx, u8 color, bool shadow);
    void copy_rows_to_buffer();
    void copy_buffer_to_rows();
    bool get_row_full();

    // Current placeholder, should point to a decoding array down the line
    // The array should map input idx -> push_char, space, or newline
    void decode_char(int char_val);
    void insert_space();
    void insert_nl();


    // Member variables
    const CharData* char_data;
    u32* char_wram_row_data;
    u32* tileset_copy;

    // Tile Management
    u16* bg_map_ptr;
    u16* bg_tile_ptr;   

    TextMeta text_meta;
};