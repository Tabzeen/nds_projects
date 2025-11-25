import cv2
from PIL import Image, ImageDraw, ImageFont
import numpy as np


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

ttf_font = ImageFont.truetype("./ByteBounce.ttf", 28)

bound_w = 30
bound_h = 50

ascii_arr = [chr(i) for i in list(range(0x21, 0x7F))]

image = Image.new('1', (bound_w * len(ascii_arr), bound_h), color='white')

draw = ImageDraw.Draw(image)

for i in range(len(ascii_arr)):
    draw.text((bound_w * i,bound_h // 2), ascii_arr[i], font = ttf_font, fill='black', anchor='lm')


image_np = np.array(image)

col_h = not image_np.all(axis=1) 
min_y, max_y = (np.where(col_h)[0], np.where(col_h)[-1])

col_w = not image_np.all(axis=0)
