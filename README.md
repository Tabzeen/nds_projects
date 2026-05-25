## A Collection of Small Proof-of-Concept Projects Targeting Nintendo DS Hardware
This repository contains projects aimed at exploring and learning different aspects of the Nintendo DS hardware.

### Directory Structure
* **Backgrounds:** Projects targeting the device's fixed graphics pipeline and VRAM management.
  * **Parallax:** A simple demo utilizing all VRAM banks to display a parallax scrolling animation.
  * **Infinite_Background:** A demo that renders a larger tilemap than the architecture natively supports by streaming it into VRAM on demand.
  * **Flex_Dialogue:** A demo showcasing a custom compact font format and a custom rendering engine. This engine allows the runtime display of two-color, variable-width custom fonts at various scaling resolutions.
* **Sprite Modes:** Projects targeting the Nintendo DS sprite rendering engine and its animation capabilities.
* **Font_Generator:** A tool that converts TrueType fonts (.ttf) into a custom NTFT format, which can then be parsed by the custom rendering engine.

### How to run?
All projects come with a precompiled binary that can be executed on real NDS hardware or using an emulator. 
If you want to compile the projects from source, follow the instructions below:

1. Download and install devkitPro and the `nds-dev` environment. A detailed guide can be found [here](https://devkitpro.org/wiki/Getting_Started).
2. Open your terminal or command prompt and navigate to the subdirectory of the project you want to compile.
3. Run the `make` command in the root of that project directory to execute the Makefile.
4. Copy the resulting `.nds` file to a flashcard or run it in an emulator.
