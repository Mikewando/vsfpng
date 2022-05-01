# fpng for VapourSynth

This is a small plugin which allows [fpng](https://github.com/richgel999/fpng) to be used with [VapourSynth](https://github.com/vapoursynth/vapoursynth).

The motivation for this plugin was to dump frames of a VapourSynth script to PNGs faster than was possible with [imwri](https://github.com/vapoursynth/vs-imwri).

## Usage

### Plugin Documentation

Usage is similar to `imwri.Write()` with ImageMagick options replaced with fpng options.

```
.. function:: Write(clip clip, string filename[, int firstnum=0, int compression=1, bint overwrite=False, clip alpha])
   :module: fpng

   Write will write each frame to disk as it's requested. If a frame is never requested it's also never written to disk.

   Parameters:
      clip
         Input clip. Only RGB24 is supported.

      filename
         The filename string must have one or more frame number substitutions. The syntax is printf style. For example "image%06d.png" or "/images/%d.png" is common usage.

      firstnum
         The first image number in the sequence to write.

      compression
         Matches fpng flags which are:
         0 - fast compression
         1 - slow compression (smaller output file)
         2 - uncompressed

      overwrite
         Overwrite already existing files. This option also disables the requirement that output filenames contain a number.

      alpha
         A grayscale clip containing the alpha channel for the image to write. Apart from being grayscale, its properties must be identical to the main *clip*.
```

### Example Python Script

The `examples/dump_frames.py` script included in this repository is not necessary to use the plugin, but may be useful either to directly dump frames or as a reference for the plugin usage.

```
usage: dump_frames.py [-h] -o OUTPUT [--filename FILENAME] [--overwrite] script

Dump frames from VapourSynth script. Script will be converted to RGB24 using frame properties if they're available.

positional arguments:
  script                The VapourSynth script to fetch frames from

optional arguments:
  -h, --help            show this help message and exit
  -o OUTPUT, --output OUTPUT
                        The output directory for dumped images (default: None)
  --filename FILENAME   The filename template for dumped images (output is always .png) (default: frame_%d)
  --overwrite           Set to overwrite existing files (default: False)
```

## Building

The author isn't overly familiar with C++ "build systems" so details may be missing. Broadly the only dependency not included with the source should be the VapourSynth SDK. The build system tested is MSYS2's mingw-w64 on Windows 10. It is quite likely that building is possible on other systems given sufficent experience building C++ libraries using those other systems.

```sh
meson setup -Dstatic=true build
cd build/
ninja

# If all goes well libfpng.dll should exist within the build directory
```

## Bundled Dependencies

Some dependencies are directly copied into the `src/` directory from their respective projects. It is the author's understanding that this usage is compatible with the applicable licenses.

From [fpng](https://github.com/richgel999/fpng/tree/6926f5a0a78f22d42b074a0ab8032e07736babd4) fetched 2022-04-30
 - `fpng.h`
 - `fpng.cpp`

From [libp2p](https://github.com/sekrit-twc/libp2p/tree/ed0a37adf0fdab2af95845fc80e31a6b59debebe) fetched 2022-04-30
 - `p2p.h`
 - `p2p_api.h`
 - `p2p_api.cpp`
 - `v210.cpp`

From [vs-imwri](https://github.com/vapoursynth/vs-imwri/tree/3042a327739e44b929f5ab02ff1da4d8de5ee061) fetched 2022-04-30
 - `vsutf16.h`
 - Several methods within `plugin.cpp` also directly come from `imwri.cpp`