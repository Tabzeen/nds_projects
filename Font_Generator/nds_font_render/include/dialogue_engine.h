#pragma once

#include <nds.h>
#include <array>
#include <cstdint>
#include "font.h"


constexpr u8 tile_size_u32 = 8;

class DialogueEngine {
public:
    DialogueEngine(const CharData* char_data_src, int bg_idx);
    ~DialogueEngine();

    void init_map_bounds(u8 y_start, u8 y_end, u8 x_start, u8 x_end);
    void push_char(int idx, bool advance);
    void push_hello();
    void set_cursor_position(u8 x, u8 y);
    void push_ascii_string(char* text);
    void clear();

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
    };

    // Private helper methods
    void build_ascii_lut();
    void write_char_to_buffer(int char_idx);
    void copy_rows_to_buffer(int cursor_x, int cursor_y);
    void copy_buffer_to_rows(int cursor_x, int cursor_y);

    // Member variables
    const CharData* char_data;
    u32* char_wram_row_data;
    u32* tileset_copy;

    int tile_offset = 0;
    int inner_tile_offset = 0;
    int mem_width;
    int bg_idx;

    TextMeta text_meta;
};