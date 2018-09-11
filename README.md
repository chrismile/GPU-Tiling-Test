# GPU Tiling Test

to https://github.com/nlguillemot/trianglebin
The program offers two internal modes: TILING_MODE_TILES and TILING_MODE_INDICES.
- TILING_MODE_INDICES is only supported on NVIDIA graphis cards. It uses GL_NV_shader_thread_group to output the SM (streaming multiprocessor) number, warp number or thread number of a fragment. The following modes are supported (and activated by pressing the corresponding number key on the keyboard):
   - 1: Combined thread number (red channel), warp number (green channel) and SM number (blue channel)
   - 2: Thread number (multiplied with color.rgb)
   - 3: Warp number (multiplied with color.rgb)
   - 4: SM number (multiplied with color.rgb)
- TILING_MODE_TILES works similar to the DirectX program https://github.com/nlguillemot/trianglebin and uses an atomic counter in order to render only a limited number of pixels. By rendering only e.g. 50% of the pixels of seven triangles filling half of the screen, it becomes possible to see whether the scene is rendered in tiles internally on the graphics card. This mode is activated by pressing '0' on the keyboard.

Prerequisites (build currently only supported on Linux):
- sgl: https://github.com/chrismile/sgl (use sudo make install to install this library on your system)

## Building and running the programm
After installing sgl (see above) execute in the repository directory:

```
mkdir build
cd build
cmake ..
make -j 4
ln -s ../Data .
```
(Alternatively, use 'cp -R ../Data .' to copy the Data directory instead of creating a soft link to it).

To run the program, execute (if sgl was installed to /usr/local/lib):
```
export LD_LIBRARY_PATH=/usr/local/lib
./GPUTilingTest
```
