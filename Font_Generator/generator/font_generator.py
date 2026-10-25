import struct
from pathlib import Path
from string import Template
from PIL import Image, ImageDraw, ImageFont

# ==========================================
# Configuration & Font Setup
# ==========================================

FONT_PATH = Path("generator/PMD.ttf")
OUTPUT_DIR_BIN = Path("generator/gen/data")
OUTPUT_DIR_HEADER = Path("generator/gen/include")
FONT_NAME = "PmdFont"
FONT_DIM = 16
CHAR_SIZE_U8 = (FONT_DIM ** 2) // 8 
FONT_VERSION = 0x0200
MAGIC_HEADER = 0x4E464E54

OUTPUT_DIR_BIN.mkdir(parents=True, exist_ok=True)
OUTPUT_DIR_HEADER.mkdir(parents=True, exist_ok=True)
ttf_font = ImageFont.truetype(str(FONT_PATH), FONT_DIM)
ascent, _ = ttf_font.getmetrics()

# ==========================================
# Glyph Rendering
# ==========================================

def render_glyph(ascii_idx):
    char = chr(ascii_idx)
    stride = int(ttf_font.getlength(char))
    img = Image.new("1", (FONT_DIM, FONT_DIM))
    draw = ImageDraw.Draw(img)
    draw.text((0, ascent), char, font=ttf_font, fill=255, anchor="ls", fontmode="1")
    return {"bmp": img, "atlas": ascii_idx, "stride": stride}

ascii_render = [render_glyph(idx) for idx in range(0x21, 0x7F)]

# ==========================================
# Display Test & Visualizer
# ==========================================

total_width = sum(g["stride"] for g in ascii_render)
sum_image = Image.new("1", (min(600, total_width), FONT_DIM))

current_x = 0
for glyph in ascii_render:
    crop = glyph["bmp"].crop((0, 0, glyph["stride"], FONT_DIM))
    sum_image.paste(crop, (current_x, 0))
    current_x += glyph["stride"]

# sum_image.show()

img_exp = Image.new("1", (FONT_DIM, FONT_DIM))
img_exp.paste(ascii_render[54]["bmp"], (0, 0))
# img_exp.show()

# ==========================================
# Writing the Binary
# ==========================================

char_cnt = len(ascii_render)
pad_len = (4 - (char_cnt % 4)) % 4

font_binary = bytearray()
font_binary.extend(struct.pack(">I", MAGIC_HEADER))
font_binary.extend(struct.pack("<HH", FONT_VERSION, 0x0))

# Stride data
for g in ascii_render:
    font_binary.extend(struct.pack("<B", g["stride"]))
font_binary.extend(b"\xFF" * pad_len)

# Pixel bitmap data
for g in ascii_render:
    font_binary.extend(g["bmp"].tobytes())

# Character space padding
font_binary.extend(b"\x00" * (pad_len * 4))

with open(OUTPUT_DIR_BIN / f"{FONT_NAME}.bin", "wb") as binary_file:
    binary_file.write(font_binary)

# ==========================================
# Writing the Header
# ==========================================

stride_offset = 8
pixel_data_offset = stride_offset + char_cnt + pad_len

header_template = Template('''
#pragma once

#include <optional>
#include "CharData.h"
#include "${fontname}_bin.h"

std::optional<uint16_t> map_fn(uint16_t) {
    // Describe the decoding function here
    return 0;
}

namespace Fonts::${fontname} {

    constexpr CharData Data = {
        .version        = ${version},
        .font_dim       = ${font_dim},
        .char_size_u8   = ${char_size_u8},
        .strides        = ${fontname}_bin + ${stride_offset},
        .pixels         = ${fontname}_bin + ${pixel_data_offset},
        .map_fn         = &map_fn
    };

}
''')

header_content = header_template.substitute(
    fontname=FONT_NAME,
    version=FONT_VERSION,
    font_dim=FONT_DIM,
    char_size_u8=0,
    stride_offset=stride_offset,
    pixel_data_offset=pixel_data_offset,
)

with open(OUTPUT_DIR_HEADER / f"{FONT_NAME}.h", "w", encoding="utf-8") as header_file:
    header_file.write(header_content)