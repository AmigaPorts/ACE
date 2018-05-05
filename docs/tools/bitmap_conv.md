# Bitmap conversion

After you've got yourself a palette, you may want to use it to display some images. For this task there's a `bitmap_conv` command line tool. Currently it supports only PNG input files, but other formats will be supported some day.

If you ever get stuck, just type `bitmap_conv` and it will display detailed info about its switches and params.

## `.bm` file format

`.bm` files are ACE-specific and currently implemented as raw bitplane data preceeded by minimal header. It supports planar and interleaved encoding for loading times optimization, which also currently determines bitmap use mode in game - bitmap functions won't be able to load interleaved bitmap into portion of non-interleaved one, and vice versa.

The format was born since @tehKaiN was not happy with IFF - it seemed too bloated for him and required additional time to understand all of its features, which are rarily needed. If you need something more fancy than plain bitplane storage, you are strongly encouraged to go with IFF files by using `iffparse.library`.

## How to...

First argument is always `.plt` palette, second is source image. Then you can throw some additional switches. Input file doesn't have to be indexed, but needs to have only colors present in palette. Colors will be outputted with same indices as order of appearance in palette.

### Convert plan bitmap

- This will save image in current directory:

  `bitmap_conv path/to/palette.plt path/to/image.png`

- If you want to save image somewhere else, use `-o` (_output_):

  `bitmap_conv path/to/palette.plt path/to/image.png -o path/to/output/file.bm`

- If you want to save in interleaved mode, just throw `-i` (_interleaved_) somewhere:

  `bitmap_conv path/to/palette.plt path/to/image.png -o path/to/output/file.bm -i`

### Convert bitmap with transparency color

To define transparency mask, use in your image one more color than defined in palette, say `#f0f`. Then, during conversion add `-mc #ff00ff` (_mask color_) switch so that `bitmap_conv` will threat this color as transparency mask. Mask will be outputted to `.msk` file which currently is just raw bitplane with width/height header.

- Simple conversion:

  `bitmap_conv path/to/palette.plt path/to/image.png -o path/to/output/file.bm -mc #ff00ff`

- Save mask somewhere else: `-mo` (_mask output_)

  `bitmap_conv path/to/palette.plt path/to/image.png -o path/to/output/file.bm -mc #ff00ff -mo path/to/mask/file.msk`
