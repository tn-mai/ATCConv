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
  std::cout << "usage: atcconv.exe [infile] [outfile]" << std::endl;
  std::cout << std::endl;
  std::cout << "  infile : input PNG(24/32bit) file path." << std::endl;
  std::cout << "  outfile: output KTX file path." << std::endl;
  std::cout << "           if not passed this option, use the infile that has replaced" << std::endl;
  std::cout << "           extention ot '.ktx'." << std::endl;
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

/** The entry point.
*/
int main(int argc, char** argv) {
  std::string infilename;
  std::string outfilename;
  for (int i = 1; i < argc; ++i) {
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

  const uint32_t width = FreeImage_GetWidth(dib);
  const uint32_t height = FreeImage_GetHeight(dib);
  const uint32_t bitPerPixel = FreeImage_GetBPP(dib);
  const uint32_t destFormat = bitPerPixel == 32 ? Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA : Q_FORMAT_ETC1_RGB8;

  TQonvertImage  src = TQonvertImage_Create(FreeImage_GetBits(dib), width, height);
  TQonvertImage  dest = TQonvertImage_Create(nullptr, width, height, destFormat);
  std::vector<uint8_t> buf;
  buf.resize(width * height * 4, 0);
  dest.nDataSize = static_cast<unsigned int>(buf.size());
  dest.pData = &buf[0];
  {
    const int result = Qonvert(&src, &dest);
    if (result != Q_SUCCESS) {
	  std::cout << "Can't convert '" << infilename << "'." << std::endl;
	  FreeImage_Unload(dib);
	  return 2;
    }
  }
  FreeImage_Unload(dib);

  KTX::Header ktxheader;
  KTX::initialize(&ktxheader, width, height, bitPerPixel == 32 ? KTX::Format_ATC_I : KTX::Format_ETC1);
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
