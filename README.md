# ATCConv
Convert PNG(24/32bit) image to KTX(ETC1/ATC compressed format) image.

usage: ATCConv [-f format] [infile] [outfile]

By default, 24bit PNG image is converted to ETC1 image. 32bit PNG image is converted to ATC(Interporated) image.  
You can select the output format using -f option.

The -f option can accepts following type.
- atci: Outputs ATC Interpolated format.
- atce: Outputs ATC Explicit format.
- etc1: Outpus ETC1 format.

If the outfile doesn't set, it will use the infile,its extension has replaced to 'ktx'.
