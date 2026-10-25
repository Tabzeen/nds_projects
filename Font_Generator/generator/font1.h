
#pragma once

#include "CharData.h"
#include "font1_bin.h"

uint16_t map_fn(uint16_t) {
    // Describe the decoding function here
    return 0;
}

namespace Fonts::font1 {

    constexpr CharData Data = {
        .version = 512,
        .font_dim = 16,
        .font_stride_ptr = font1_bin + 8,
        .font_data_ptr = font1_bin + 104,
        .map_fn = &map_fn
    };

}
