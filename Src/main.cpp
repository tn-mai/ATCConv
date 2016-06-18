/**
  @file main.cpp
*/
#include "ktx.h"
#include <TextureConverter.h>
#include <FreeImage.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>

/** Print the program usage.
*/
void PrintUsage() {
  std::cout << "ATCConv ver.0.2" << std::endl;
  std::cout << "Convert PNG(24/32bit) image to KTX(ATC/ETC1 compressed format) image." << std::endl;
  std::cout << std::endl;
  std::cout << "usage: atcconv.exe [-f format] [infile] [outfile]" << std::endl;
  std::cout << std::endl;
  std::cout << "  infile   : input PNG(24/32bit) file path." << std::endl;
  std::cout << "  outfile  : output KTX file path." << std::endl;
  std::cout << "             if not passed this option, use the infile that has" << std::endl;
  std::cout << "             replaced extention ot '.ktx'." << std::endl;
  std::cout << "  -f format: the output image format." << std::endl;
  std::cout << "             atci: ATC with interpolated alpha." << std::endl;
  std::cout << "             atce: ATC with explicit alpha." << std::endl;
  std::cout << "             etc1: ETC1." << std::endl;
  std::cout << "  -m count : the mipmap count." << std::endl;
  std::cout << "             if count is less than 2, a result has single image(no mipmap)." << std::endl;
  std::cout << "             if count is greater than 16, count is considered as 16." << std::endl;
  std::cout << "  -v       : flip virtucal." << std::endl;
  std::cout << "" << std::endl;
  std::cout << std::endl;
  std::cout << "If not passed -f option, the output format is selected by the BPP of the" << std::endl;
  std::cout << "input image. 'etc1' will be selected in the 24bit image, otherwize 'atci'." << std::endl;
  std::cout << "If infile doesn't have the alpha in 'atci' or 'atce', it is assumed to be 1.0." << std::endl;
  std::cout << "If infile has the alpha in 'etc1', ignored." << std::endl;
}

/** Get bytes per pixel from the format.

  @param format Q_FORMAT_???

  @return byte per pixel.
*/
uint32_t GetBytePerPixel(uint32_t format) {
  switch (format) {
  case Q_FORMAT_RGB_8I: return 3;
  default:
  case Q_FORMAT_RGBA_8I: return 4;
  }
}

/** Get the OpenGL format from Q_FORMAT_???.

  @param  qformat  Q_FORMAT_???. It is the TextureConverter format type.

  @return The corresponding OpenGL format of the qformat.
*/
uint32_t GetOpenGLTextureFormat(uint32_t qformat) {
  switch (qformat) {
  case Q_FORMAT_ETC1_RGB8: return KTX::Format_ETC1;
  case Q_FORMAT_ATC_RGBA_EXPLICIT_ALPHA: return KTX::Format_ATC_E;
  default:
  case Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA: return KTX::Format_ATC_I;
  }
}

/** Create TQonvertImage structure.

  @param data    the pointer to the raw image data.
  @param w       the pixel width of the image.
  @param h       the pixel height of the image.
  @param foramt  Q_FORMAT_???

  @return TQonvertImage object created with the parameter.
*/
TQonvertImage TQonvertImage_Create( void* data, uint32_t w, uint32_t h, uint32_t format) {
  TQonvertImage n;
  n.nWidth = w;
  n.nHeight = h;
  n.nFormat = format;
  n.pFormatFlags = nullptr;
  n.nDataSize = data ? w * h * GetBytePerPixel(format) : 0;
  n.pData = reinterpret_cast<unsigned char*>(data);
  n.compressionOptions = nullptr;
  return n;
}

struct ArgToFormat {
  const char* argname;
  uint32_t format;
};
static const ArgToFormat argToFormatList[] = {
  { "atce", Q_FORMAT_ATC_RGBA_EXPLICIT_ALPHA },
  { "atci", Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA },
  { "etc1", Q_FORMAT_ETC1_RGB8 },
};
static const uint32_t Q_FORMAT_UNKNOWN = 0xffffffffU;

/** The entry point.
*/
int main(int argc, char** argv) {
  std::string infilename;
  std::string outfilename;
  uint32_t outputFormat = Q_FORMAT_UNKNOWN;
  uint32_t maxLevel = 1;
  bool flipY = false;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if ((argv[i][1] == 'f' || argv[i][1] == 'F') && argv[i][2] == '\0' && (argc >= i + 1)) {
        static const ArgToFormat* const end = argToFormatList + sizeof(argToFormatList) / sizeof(argToFormatList[0]);
        for (const ArgToFormat* itr = argToFormatList; itr != end; ++itr) {
          if (strcmp(itr->argname, argv[i + 1]) == 0) {
            outputFormat = itr->format;
            break;
          }
        }
        if (outputFormat == Q_FORMAT_UNKNOWN) {
          std::cout << "Error: '" << argv[i + 1] << "' is unknown format." << std::endl;
          return 1;
        }
        ++i;
      } else if ((argv[i][1] == 'm' || argv[i][1] == 'M') && argv[i][2] == '\0' && (argc >= i + 1)) {
		maxLevel = std::max(1, std::min(16, std::atoi(argv[i + 1])));
		++i;
	  } else if ((argv[i][1] == 'v' || argv[i][1] == 'V') && argv[i][2] == '\0') {
		flipY = true;
	  }
      continue;
    }
	if (infilename.empty()) {
	  infilename = argv[i];
	} else if (outfilename.empty()) {
	  outfilename = argv[i];
	  break;
	}
  }
  if (infilename.empty()) {
	PrintUsage();
	return 0;
  }
  if (outfilename.empty()) {
	outfilename = infilename;
	auto dotPos = outfilename.find_last_of('.');
	if (dotPos != std::string::npos) {
	  outfilename.erase(dotPos, std::string::npos);
	}
	outfilename += ".ktx";
  }

#ifdef FREE_IMAGE_STATIC_LIBRARY
  FreeImage_Initialise(FALSE);
#endif // FREE_ISTATIC_LIBRARY 

  FIBITMAP* dib = FreeImage_Load(FIF_PNG, infilename.c_str(), PNG_DEFAULT);
  if (!dib) {
	std::cout << "Can't read '" << infilename << "'." << std::endl;
	return 1;
  }

  uint32_t bitPerPixel = FreeImage_GetBPP(dib);
  const FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);
  if ((bitPerPixel != 24 && bitPerPixel != 32) || (colorType != FIC_RGB && colorType != FIC_RGBALPHA)) {
    FIBITMAP* dib2;
    if (FreeImage_IsTransparent(dib)) {
      dib2 = FreeImage_ConvertTo32Bits(dib);
      bitPerPixel = 32;
    } else {
      dib2 = FreeImage_ConvertTo24Bits(dib);
      bitPerPixel = 24;
    }
    FreeImage_Unload(dib);
    if (!dib2) {
      std::cout << "Can't convert '" << infilename << "'." << std::endl;
      return 1;
    }
    dib = dib2;
  }

  uint32_t width = FreeImage_GetWidth(dib);
  uint32_t height = FreeImage_GetHeight(dib);
  if (outputFormat == Q_FORMAT_UNKNOWN) {
    outputFormat = bitPerPixel == 24 ? Q_FORMAT_ETC1_RGB8 : Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA;
  }

  TFormatFlags srcFlags = { 0 };
  srcFlags.nMaskRed   = FreeImage_GetRedMask(dib);
  srcFlags.nMaskGreen = FreeImage_GetGreenMask(dib);
  srcFlags.nMaskBlue  = FreeImage_GetBlueMask(dib);
  srcFlags.nMaskAlpha = bitPerPixel == 24 ? 0 : 0xff000000U;

  TFormatFlags destFlags = { 0 };
  destFlags.nFlipY = !flipY; // NOTE: dib has a virtucal reversed image.

  KTX::File ktx;
  KTX::initialize(&ktx.header, width, height, GetOpenGLTextureFormat(outputFormat));
  for (uint32_t level = 0; ;) {
	TQonvertImage  src = TQonvertImage_Create(FreeImage_GetBits(dib), width, height, bitPerPixel == 24 ? Q_FORMAT_RGB_8I : Q_FORMAT_RGBA_8I);
	src.pFormatFlags = &srcFlags;

	TQonvertImage  dest = TQonvertImage_Create(nullptr, width, height, outputFormat);
	dest.pFormatFlags = &destFlags;
	{
	  // At First, dest.pData is nullptr.
	  // Qonvert return the required buffer size in dest.nDataSize.
	  const int result = Qonvert(&src, &dest);
	  if (result != Q_SUCCESS) {
		std::cout << "Can't convert '" << infilename << "'." << std::endl;
		FreeImage_Unload(dib);
		return 2;
	  }
	}
	// Allocate the destination buffer using the size information returnd from Qonvert.
	std::vector<uint8_t> buf;
	buf.resize(dest.nDataSize);
	dest.pData = &buf[0];
	{
	  // This time, Qonvert can really convert the image.
	  const int result = Qonvert(&src, &dest);
	  if (result != Q_SUCCESS) {
		std::cout << "Can't convert '" << infilename << "'." << std::endl;
		FreeImage_Unload(dib);
		return 2;
	  }
	}
	ktx.data.push_back(KTX::File::Data());
	ktx.data.back().imageSize = dest.nDataSize;
	ktx.data.back().buf.swap(buf);
	if ((++level >= maxLevel) || (width == 1 || height == 1)) {
	  ktx.header.numberOfMipmapLevels = level;
	  break;
	}
	width /= 2;
	height /= 2;
	FIBITMAP* dib2 = FreeImage_Rescale(dib, width, height);
	FreeImage_Unload(dib);
	if (!dib2) {
	  std::cout << "Can't convert '" << infilename << "'." << std::endl;
	  return 2;
	}
	dib = dib2;
  }
  FreeImage_Unload(dib);

  if (!KTX::write_texture(outfilename, ktx)) {
	return 3;
  }

#ifdef FREE_IMAGE_STATIC_LIBRARY
  FreeImage_Deinitialise();
#endif // FREE_ISTATIC_LIBRARY 
}
