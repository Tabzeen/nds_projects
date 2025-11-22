import cv2
from PIL import Image, ImageDraw, ImageFont
import numpy as np


# TODO
# Create a 1 Bit font, and add an optional dropshadow, which is just the font shifted by (1,1)

ttf_font = ImageFont.truetype("./ByteBounce.ttf", 28)

image = Image.new('1', (100,100), color='white')

draw = ImageDraw.Draw(image)

draw.text((0,0), "Hello", font = ttf_font, fill='black', anchor='lt')

image_np = np.array(image)
black_px = image_np[image_np == 1]
min_y, max_y = (np.min(black_px[0]), np.max(black_px[0]))
min_x, max_x = (np.min(black_px[1]), np.max(black_px[1]))


