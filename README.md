# fpng for VapourSynth

This is a small plugin which allows [fpng](https://github.com/richgel999/fpng) to be used with [VapourSynth](https://github.com/vapoursynth/vapoursynth).

The motivation for this plugin was to dump frames of a VapourSynth script to PNGs faster than was possible with [imwri](https://github.com/vapoursynth/vs-imwri).

## Installation

The plugin is available on PyPI and can be installed with:

```sh
python -m pip install vsfpng
```

## Usage

### Python Helper

The raw plugin only writes frames when they are requested. This complicates the most common scenario where a user wants to dump all frames from a clip. To make dumping easier, a python helper is included and can be invoked from your script. See below example.

To get progress feedback, install [rich](https://pypi.org/project/rich/).

```py
def dump(
        clip: vs.VideoNode | tuple[vs.VideoNode, vs.VideoNode],
        output_directory: str | Path,
        filename_template: str = "frame_%d",
        first_num: int = 0,
        overwrite: bool = True,
        compression: Compression = Compression.SLOW,
    ) -> None:
    """
    Dump all frames of a clip to .png files.

    Args:
        clip: A single input clip if no transparency is needed. Otherwise
            a tuple of (clip, alpha) where alpha is a gray clip with the same
            dimensions.
        output_directory: Where to dump PNGs. Will attempt to create the
            directory if it doesn't already exist.
        filename_template: A printf style template for the filename. For
            example "frame_%d" or "%06d". Suffix will always be .png and does
            not need to be explicitly specified.
        first_num: The number to start counting from. Useful, for example, to
            preserve frame numbers from a source clip if the input clip has
            been trimmed.
        overwrite: If existing files should be overwritten. When set to False,
            existing files are left untouched, and no exception is raised.
        compression: The fpng compression level. The default is fine for most
            purposes, but can be set if other speed and size trade-offs are
            desired.
    """
```

#### Example

<details open>
<summary>example.py</summary>

```py
import vapoursynth as vs

clip = vs.core.bs.VideoSource("example.mkv")
# Set output so clip can be previewed or used with vspipe normally
clip.set_output()

# Only dump if the script is invoked directly
if __name__ == "__main__":
   from vsfpng import dump
   dump(clip, "./output")
```

</details>

<details open>
<summary>output</summary>

```console
$ vspipe example.py --info
Output Index: 0
Type: Video
Width: 1920
Height: 1080
Frames: 38072
FPS: 60/1 (60.000 fps)
Format Name: YUV420P8
Color Family: YUV
Alpha: No
Sample Type: Integer
Bits: 8
SubSampling W: 1
SubSampling H: 1

$ python example.py
Dumping... ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 100% 0:01:49
```

</details>

### Plugin Documentation

Usage is similar to `imwri.Write()` with ImageMagick options replaced with fpng options.

```rst
.. function:: Write(clip clip, string filename[, int firstnum=0, int compression=1, bint overwrite=False, clip alpha])
   :module: fpng

   Write will write each frame to disk as it's requested. If a frame is never
   requested it's also never written to disk.

   Parameters:
      clip
         Input clip. Only RGB24 is supported.

      filename
         The filename string must have one or more frame number substitutions.
         The syntax is printf style. For example "image%06d.png" or
         "/images/%d.png" is common usage.

      firstnum
         The first image number in the sequence to write.

      compression
         Matches fpng flags which are:
         0 - fast compression
         1 - slow compression (smaller output file)
         2 - uncompressed

      overwrite
         Overwrite already existing files. This option also disables the
         requirement that output filenames contain a number.

      alpha
         A grayscale clip containing the alpha channel for the image to write.
         Apart from being grayscale, its properties must be identical to the
         main *clip*.
```

## Vendored Dependencies

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

[vapoursynth](https://github.com/vapoursynth/vapoursynth) as a submodule for
- `VapourSynth4.h`