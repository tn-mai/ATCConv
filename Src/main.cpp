/**
  @file main.cpp
*/
#include "ktx.h"
#include <TextureConverter.h>
#include <FreeImage.h>
#include <stdio.h>
#include <string>
#include <iostream>

/** Print the program usage.
*/
void PrintUsage() {
  std::cout << "ATCConv ver.0.1" << std::endl;
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
  std::cout << std::endl;
  std::cout << "If infile doesn't have the alpha in atci or atce, it is assumed to be 1.0." << std::endl;
  std::cout << "If infile has the alpha in etc1, ignored." << std::endl;
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
TQonvertImage TQonvertImage_Create( void* data, uint32_t w, uint32_t h, uint32_t format = Q_FORMAT_RGBA_8I) {
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
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'f' || argv[i][1] == 'F' && (argc >= i + 1)) {
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
  if (outputFormat == Q_FORMAT_UNKNOWN) {
    outputFormat = Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA;
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

  const uint32_t width = FreeImage_GetWidth(dib);
  const uint32_t height = FreeImage_GetHeight(dib);
  const uint32_t bitPerPixel = FreeImage_GetBPP(dib);

  TFormatFlags srcFlags = { 0 };
  srcFlags.nMaskRed   = FreeImage_GetRedMask(dib);
  srcFlags.nMaskGreen = FreeImage_GetGreenMask(dib);
  srcFlags.nMaskBlue  = FreeImage_GetBlueMask(dib);
  srcFlags.nMaskAlpha = 0xff000000U;
  TQonvertImage  src = TQonvertImage_Create(FreeImage_GetBits(dib), width, height);
  src.pFormatFlags = &srcFlags;

  TFormatFlags destFlags = { 0 };
  destFlags.nFlipY = TRUE;
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
  FreeImage_Unload(dib);

  KTX::Header ktxheader;
  KTX::initialize(&ktxheader, width, height, GetOpenGLTextureFormat(outputFormat));
  FILE* fp;
  {
	const errno_t result = fopen_s(&fp, outfilename.c_str(), "wb");
	if (result != 0) {
	  std::cout << "Can't open '" << outfilename << std::endl;
	  return 3;
	}
  }
  fwrite(&ktxheader, 1, sizeof(KTX::Header), fp);
  fwrite(&dest.nDataSize, 1, 4, fp);
  fwrite(dest.pData, 1, dest.nDataSize, fp);
  fclose(fp);

#ifdef FREE_IMAGE_STATIC_LIBRARY
  FreeImage_Deinitialise();
#endif // FREE_ISTATIC_LIBRARY 
}
