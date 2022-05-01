import os
import time
import argparse
import importlib.util

from pathlib import Path

try:
    from rich.progress import track
except ModuleNotFoundError:
    def track(iterable, prefix = '', suffix = '', decimals = 1, length = 60, fill = '█', printEnd = "\r", total = None):
        """
        From https://stackoverflow.com/a/34325723

        Call in a loop to create terminal progress bar
        @params:
            iterable    - Required  : iterable object (Iterable)
            prefix      - Optional  : prefix string (Str)
            suffix      - Optional  : suffix string (Str)
            decimals    - Optional  : positive number of decimals in percent complete (Int)
            length      - Optional  : character length of bar (Int)
            fill        - Optional  : bar fill character (Str)
            printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
        """
        if total is None:
            total = len(iterable)
        # Progress Bar Printing Function
        def printProgressBar (iteration):
            percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
            filledLength = int(length * iteration // total)
            bar = fill * filledLength + '-' * (length - filledLength)
            print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
        # Initial Call
        printProgressBar(0)
        # Update Progress Bar
        for i, item in enumerate(iterable):
            yield item
            printProgressBar(i + 1)
        # Print New Line on Complete
        print()

def main(args):
    output_path = Path(args.output).resolve()
    
    # Switch to the script directory so relative sources load
    os.chdir(Path(args.script).parent)

    # Import script
    loader = importlib.machinery.SourceFileLoader('input_script', args.script)
    spec = importlib.util.spec_from_loader('input_script', loader)
    input_script = importlib.util.module_from_spec(spec)
    loader.exec_module(input_script)

    import vapoursynth as vs

    clip, alpha, alt_output = vs.get_output(0)
    clip = clip.resize.Spline36(format=vs.RGB24)
    start = time.perf_counter()
    clip = clip.fpng.Write(filename=(output_path / args.filename).with_suffix('.png'), alpha=alpha, overwrite=int(args.overwrite))
    for frame in track(clip.frames(close=True), total=len(clip)):
        pass
    print(f'Finished dumping in {time.perf_counter() - start} seconds')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Dump frames from VapourSynth script. Script will be converted to RGB24 using frame properties if they're available.", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('script', help='The VapourSynth script to fetch frames from')
    parser.add_argument('-o', '--output', required=True, help='The output directory for dumped images')
    parser.add_argument('--filename', help='The filename template for dumped images (output is always .png)', default='frame_%d')
    parser.add_argument('--overwrite', help='Set to overwrite existing files', action='store_true')
    main(parser.parse_args())