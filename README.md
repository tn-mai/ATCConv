# ATCConv
Convert PNG(24/32bit) image to KTX(ETC1/ATC compressed format) image.

usage: ATCConv.exe [-f format] [infile] [outfile]

By default, 24bit PNG image is converted to ETC1 image. 32bit PNG image is converted to ATC(Interporated) image.  
You can select the output format using -f option.

The -f option can accepts following type.
- atci: Outputs ATC Interpolated format.
- atce: Outputs ATC Explicit format.
- etc1: Outpus ETC1 format.

The -m option sets maximum mip level.
It accepts from 1 to 16.
A value less than 1, regurd as 1, and greater than 16, as 16.

The -v option generate the virtucal flipped image.

If the outfile doesn't set, it will use the infile,its extension has replaced to 'ktx'.
