import os
import logging
import vapoursynth as vs
from pathlib import Path
from enum import IntEnum, auto

try:
    from rich.progress import track
except ModuleNotFoundError:
    def track(iterable, *args, **kwargs):
        return iterable

class Compression(IntEnum):
    FAST = 0
    SLOW = auto()
    UNCOMPRESSED = auto()

logger = logging.getLogger(__name__)

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
    if isinstance(clip, vs.VideoNode):
        alpha = None
    else:
        clip, alpha = clip

    if clip.format.id != vs.RGB24:
        clip = clip.resize.Bicubic(format=vs.RGB24)

    output_directory = Path(output_directory)
    if not output_directory.is_dir():
        os.makedirs(output_directory, exist_ok=True)
    logger.info(f"Dumping PNGs into '{output_directory.resolve()}'")
    output_path = (output_directory / filename_template).with_suffix('.png')

    clip = clip.fpng.Write(filename=output_path, firstnum=first_num, alpha=alpha, overwrite=overwrite, compression=compression)

    try:
        for _ in track(clip.frames(close=True), description="Dumping...", total=len(clip)):
            pass
    except vs.Error as e:
        if "Resize error" in e.value:
            logger.error("The input clip could not be converted to RGB before dumping. Please set the appropriate frame properties.")
        raise e
