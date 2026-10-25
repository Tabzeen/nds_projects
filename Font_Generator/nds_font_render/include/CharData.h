#pragma once

#include <cstdint>
#include <optional>

struct CharData {
    using MapFn = std::optional<uint16_t> (*)(uint16_t input_char);

    const uint16_t version;
    const uint8_t font_dim;
    const uint8_t char_cnt;
    const uint8_t char_size_u8;
    const uint8_t* strides;
    const uint8_t* pixels;
    const MapFn map_fn;

    CharData() = delete;
};