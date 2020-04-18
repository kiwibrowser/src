## visualmetrics

Calculate visual performance metrics from a video (Speed Index, Perceptual Speed Index, Visual Complete, Incremental progress, etc).  This is a command-line port of the [WebPagetest](https://github.com/WPO-Foundation/webpagetest) mobile video processing and metrics code.

## Requirements

visualmetrics requires several image processing tools be installed and available in the path:

* **[ffmpeg](https://www.ffmpeg.org/)**: Required for video extraction
* **[compare](http://www.imagemagick.org/)**: Part of [Imagemagick](http://www.imagemagick.org/)
* **[convert](http://www.imagemagick.org/)**: Part of [Imagemagick](http://www.imagemagick.org/)
* **[Pillow](https://github.com/python-pillow/Pillow)**: Python imaging library
* **[pyssim](https://github.com/jterrace/pyssim)**: Python module for Structural Similarity Image Metric (SSIM)

The dependencies can be verified by running:
```bash
$python visualmetrics.py --check
ffmpeg:   OK
convert:  OK
compare:  OK
Pillow:   OK
SSIM:     OK
```

## Command Line

### Help
```bash
$ python visualmetrics.py -h
```

### Options
* **-i, --video**: Input video file. Required if existing frames or histograms are not available.
* **-d, --dir**: Directory of video frames (as input if exists or as output if a video file is specified).  If a directory is not specified and video needs to be processed a temporary directory will be used for processing and deleted automatically.
* **-g, --histogram**: Histogram file (as input if exists or as output if histograms need to be calculated).  If a histogram file is not specified and histograms need to be calculated a temporary histograms file will be created for processing and deleted automatically.
* **-m, --timeline**: Timeline capture from Chrome dev tools. Used to synchronize the video start time and only applies when orange frames are removed (see --orange). The timeline file can be gzipped if it ends in .gz. It can also be a trace file captured with the devtools.timeline events.
* **-q, --quality**: JPEG quality to use when exporting the video frames (if video is being processed).  If not specified the video frames will be saved as PNGs.
* **-l, --full**: Keep full-resolution images instead of resizing to 400x400 pixels.
* **-f, --force**: Force re-processing of the provided video. By default if a directory or histogram file already exists then those will be used and the video will not be extracted, even if provided.  With force enabled it will overwrite any images in the video frame directory and overwrite any existing histograms.
* **-o, --orange**: Remove any orange frames at the beginning of the video.  These are usually used for synchronizing video capture with other data sources.
* **-p, --viewport**: Find the viewport from the virst video frame and use only the viewport for frame de-duplication and metrics calculation.
* **-s, --start**: Specify a starting time (in milliseconds) for calculating the various metrics.  All of the metrics except for Speed Index will still report an absolute time but will process the video frames in the requested interval.  Speed Index will be calculated relative to the starting point.
* **-e, --end**: Specify an ending time (in milliseconds) for calculating the various metrics.
* **-v, --verbose**: Display debug messages while processing. Specify multiple times (up to 4) to increase detail.
* **-k, --perceptual**: Calculates perceptual speed Index for a given input video.
* **-j, --json**: Sets output format to JSON.

## Examples

```bash
$ python visualmetrics.py --video video.mp4 --dir frames -q 75 --histogram histograms.json.gz --orange --viewport
First Visual Change: 1806
Last Visual Change: 1955
Visually Complete: 1955
Speed Index: 1835
Visual Progress: 0=0%, 1806=45%, 1827=48%, 1844=53%, 1861=99%, 1879=99%, 1901=98%, 1922=99%, 1938=99%, 1955=100%
```

```bash
$ python visualmetrics.py --video tests/data/lemons/video.mp4 --dir frames --histogram histograms.json.gz --orange --viewport
First Visual Change: 768
Last Visual Change: 2884
Speed Index: 1840
Visual Progress: 0=0%, 768=23%, 785=24%, 1510=61%, 1545=61%, 1981=61%, 2015=61%, 2033=61%, 2069=61%, 2086=61%, 2105=61%, 2140=61%, 2175=61%, 2572=61%, 2589=61%, 2606=62%, 2623=62%, 2641=63%, 2658=57%, 2676=58%, 2694=58%, 2713=57%, 2731=58%, 2749=58%, 2770=89%, 2789=87%, 2809=87%, 2828=87%, 2884=100%
```

```bash
$ python visualmetrics.py --video tests/data/lemons/video.mp4 --dir frames --histogram histograms.json.gz --orange --viewport --perceptual
First Visual Change: 768
Last Visual Change: 2884
Speed Index: 1840
Perceptual Speed Index: 892
Visual Progress: 0=0%, 768=23%, 785=24%, 1510=61%, 1545=61%, 1981=61%, 2015=61%, 2033=61%, 2069=61%, 2086=61%, 2105=61%, 2140=61%, 2175=61%, 2572=61%, 2589=61%, 2606=62%, 2623=62%, 2641=63%, 2658=57%, 2676=58%, 2694=58%, 2713=57%, 2731=58%, 2749=58%, 2770=89%, 2789=87%, 2809=87%, 2828=87%, 2884=100%
```
