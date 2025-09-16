The goal of this code is to test if we can dynamically load data from WRAM into VRAM and reset scrolling.
Essentially "streaming" tilemaps into VRAM so they get picked up by the GPU.
THis allows us to have Backgrounds larger than the max BG size on the console, which was something like 512x256.

Here I also wrote a converter which allows me to write directly to the backgroudn by converting x and y coordinates to the internal VRAM mapping in which a background of larger than 32x32 size gets rendered. Its probably incredibly inefficient but for the purpose of this simpel demo its good enough
