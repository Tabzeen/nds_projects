#pragma once

#include <cstdint>
#include <array>
#include "font_bin.h"


constexpr size_t padded_char_size = 0x5e;
constexpr size_t font_dim = 16;
constexpr size_t font_data_elem_size = (font_dim * font_dim) / 8;
constexpr size_t font_data_packed_size = font_data_elem_size * padded_char_size;   

typedef struct __attribute__((packed)) {
    uint8_t atlas;
    uint8_t advance;
} CharMeta;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint8_t font_dim;
    uint8_t char_cnt;
    std::array<CharMeta, padded_char_size> char_meta;
    std::array<uint8_t, font_data_packed_size> font_data;
} CharData;

inline const CharData* get_char_data() {
    return reinterpret_cast<const CharData*>(font_bin);
}