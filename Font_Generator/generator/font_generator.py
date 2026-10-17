import cv2
from PIL import Image, ImageDraw, ImageFont
import numpy as np
import matplotlib.pyplot as plt

import struct


# TODO
# Create a 1 Bit font, and add an optional dropshadow, which is just the font shifted by (1,1)
# Different Character have different heights, which need to be relatively correct to each other
# What we need to do is to write all characters in one large image strip.
# We then Collapse the X axis of the strip and get the min and max values to get the general
# height of of all characters, which gives us the total bound of the character set 
# (Although, ill probably cut it to fixed multiple of 2 sizes anyway for aligned access)

# ok here is how we actually implement this:
# We define a temporary bondary size and plot one character per bounds to avoid kerning of letters.
# This gives us the same PNG where we can deduct the character max height and relative posiitoning
# by Collapsing the X and Y axis respectively.  


# Ok, new Idea. We take the base size of that font, which appears to be at around 16em (Dont ask what that is). 
# Throw it into the Image thingy, Cut the empty space Horizontally, Scale that fucker up by x2 or x3
# And then render it into a larger Canvas
# For the sake of getting to anything today, Ill jst do the normal 8x8 to get a single fucking binary out today 

ttf_font = ImageFont.truetype("generator//PMD.ttf", 16)

ascent, _ = ttf_font.getmetrics()
baseline_y = ascent

def render_glyph(ascii_idx):
    ascii_char = chr(ascii_idx)
    stride = int(ttf_font.getlength(ascii_char))
    img = Image.new("1", (16, 16))
    draw = ImageDraw.Draw(img)

    draw.text((0, baseline_y), ascii_char, font=ttf_font, fill=255, anchor="ls", fontmode="1")
    
    return {
        "bmp" : img,
        "atlas": ascii_idx,
        "stride": stride 
    }


img = Image.new("1", (200, 16))


ascii_render = [render_glyph(ascii_idx=ascii_idx) for ascii_idx in range(0x21, 0x7F)]


# Display Test
total_width = sum(i["stride"] for i in ascii_render)
sum_image = Image.new("1", (min(600, total_width), 16))

current_x = 0
for char_idx in ascii_render:
    glyph_crop = char_idx["bmp"].crop((0, 0, char_idx["stride"], 16))

    sum_image.paste(glyph_crop, (current_x, 0))
    current_x += char_idx ["stride"]

#sum_image.show()


img_exp = Image.new("1", (16,16))
img_exp.paste(ascii_render[54]["bmp"], (0,0))
#img_exp.show()




'''
 Writing to a binary:
 We need a file header
 In total: Magic Number, Font Dim, 
 Byte 0-3 Magic Number 4E 46 4E 54 = N F N T (NitroFoNT)
'''

font_binary = bytearray()

char_cnt = len(ascii_render)
 
char_cnt_rem = char_cnt % 2
char_cnt_padded = char_cnt + (char_cnt_rem == 1)

char_size = 16

magic = 0x4E464E54
version = 0x0100
# 0x0 = 8x8 0x1 = 16x16
font_dim = 0x10


# Header is 64b in size
font_binary.extend(struct.pack(">I", magic))
font_binary.extend(struct.pack("<HBB", version, font_dim, char_cnt_padded))


for i in range(char_cnt):
    font_binary.extend(struct.pack("<B", ascii_render[i]["atlas"]))
    font_binary.extend(struct.pack("<B", ascii_render[i]["stride"]))

if char_cnt_rem:
    font_binary.extend(struct.pack("<B", 0xFF))
    font_binary.extend(struct.pack("<B", 0xFF))



for i in range(char_cnt):
    bytecpy = ascii_render[i]["bmp"].tobytes()
    font_binary.extend(ascii_render[i]["bmp"].tobytes())

# Add a Dummy Character 
if char_cnt_rem:
    for _ in range(8):
        font_binary.extend(struct.pack("<I", 0))
    

# 2x 32b
# Data must be 32b aligned 
with open("generator/font.bin", "wb") as binary_file:
    binary_file.write(font_binary)