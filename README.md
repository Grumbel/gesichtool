gesichtool - Face Extraction Tool
=================================

Simple face extraction tool for OpenCVs CascadeClassifier.

Usage
-----

```
$ ./gesichtool --help
Usage: gesichtool [OPTIONS] IMAGE... -o OUTDIR
Extract faces from image files

General Options:
  -h, --help                Print this help
  -v, --verbose             Be more verbose

Face Detect Options:
  -n, --min-neighbors INT   Higher values reduce false positives (default: 3)
  --min-size WxH            Minimum sizes for detected faces
  --max-size WxH            Maximum sizes for detected faces

Output Options:
  -o, --output DIR          Output directory
  --size WxH         Rescale output images to WxH (default: 512x512)
```
