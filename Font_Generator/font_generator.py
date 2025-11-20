import cv2
import numpy as np

bmp_font = cv2.imread("CharSet.png", cv2.IMREAD_UNCHANGED)

alpha_mask = (bmp_font[:, :, 3] > 0).astype(np.uint8) * 255

alpha_mask = cv2.dilate(alpha_mask, kernel = np.ones((3,3), np.uint8))

num_labels, lables, stats, centroids = cv2.connectedComponentsWithStats(alpha_mask)

max_width = np.max(stats[1:,2])
max_height = np.max(stats[1:,3])
print(max_width, max_height)
for i in range(1, num_labels):
    x, y, w, h, area = stats[i]
    glyph = np.zeros((max_height, max_width, 4))
    glyph[0:h-2, 0:w-2] = bmp_font[y+1:y+h-1, x+1:x+w-1]  # Crop the blob
    cv2.imwrite(f"glyphs/glyph_{i}.png", glyph)