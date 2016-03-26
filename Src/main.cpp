/**
  @file main.cpp
*/
#include "ktx.h"
#include <FreeImage.h>
#include <stdio.h>
#include <string>
#include <iostream>

void PrintUsage() {
  std::cout << "atcconv.exe [infile] [outfile]" << std::endl;
}

uint32_t GetBitDepth(uint32_t format) {
  switch (format) {
  case Q_FORMAT_RGB_888: return 24;
  default:
  case Q_FORMAT_RGBA_8888: return 32;
  }
}

int FreeImageBppToQFormat(int bpp) {
  switch (bpp) {
  case 

TQonvertImage TQonvertImage_Create( void* data, uint32_t w, uint32_t h, uint32_t format = Q_FORMAT_RGBA_8888) {
  TQonvertImage n;
  n.nWidth = w;
  n.nHeight = h;
  n.nFormat = format;
  n.nDataSize = width * height * GetBitDepth(format);
  n.pData = reinterpret_cast<unsigned char*>(data);
  return n;
}

int main(int argc, void** argv) {
  std::string infilename;
  std::string outfilename;
  for (int i = 1; i < argc; ++i) {
    if (!infilename) {
      infilename = argv[i];
    } else if (outfilename.empty()) {
      outfilename = argv[i];  
      break;
    }
  }
  if (!infilename) {
    PrintUsage();
    return RESULT_SUCCESS;
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
  const uint32_t bitPerPixel = FreeImage_Getbpp(dib);
  const uint32_t destFormat = bitPerPixel == 32 ? Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA : Q_FORMAT_ETC1_RGB8;

  TQonvertImage  src = TQonvertImage_Create(FreeImage_GetBits(dib), width, height);
  TQonvertImage  dest = TQonvertImage_Create(nullptr, width, height, destFormat);
  const int result = Qonvert(&src, &dest);
  if (result != Q_SUCCESS) {
    std::cout << "Can't convert '" << infilename << "'." << std::endl;
    FreeImage_Unload(dib);
    return 2;
  }
  FreeImage_Unload(dib);

  KTX::Header ktxheader;
  KTX::initialize(&ktxheader, width, height, bitPerPixel, bitPerPixel == 32 ? KTX::Format_ATC_I : KTX::Format_ETC1);
  FILE* fp = fopen(outfilename.c_str(), "wb");
  if (!fp) {
    std::cout << "Can't open '" << outfilename << std::endl;
    return 3;
  }
  fwrite(&ktxheader, 1, sizeof(KTX::Header), fp);
  fwrite(&dest.nDataSize, 1, 4, fp);
  fwrite(dest.pData, 1, dest.nDataSize, fp);
  fclose(fp);

#ifdef FREE_IMAGE_STATIC_LIBRARY
  FreeImage_Deinitialise();
#endif // FREE_ISTATIC_LIBRARY 
}
