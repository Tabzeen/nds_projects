
#pragma once

#include "CharData.h"
#include "PmdFont_bin.h"

uint16_t map_fn(uint16_t) {
    // Describe the decoding function here
    return 0;
}

namespace Fonts::PmdFont {

    constexpr CharData Data = {
        .version        = 512,
        .font_dim       = 16,
        .char_size_u8   = 0,
        .strides        = PmdFont_bin + 8,
        .pixels         = PmdFont_bin + 104,
        .map_fn         = &map_fn
    };

}
