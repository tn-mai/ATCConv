/**
  @file ktx.h

  @author tn_mai(mai.tanaka.cpper@gmail.com)

  KTX is a format for storing textures for OpenGL and OpenGL ES applications.
  It is distinguished by the simplicity of the loader required to instantiate
  a GL texture object from the file contents.

  @sa https://www.khronos.org/opengles/sdk/tools/KTX/
*/
#ifndef KTX_H_INCLUDED
#define KTX_H_INCLUDED
#include <cstdint>
#include <vector>

namespace KTX {

/** the KTX file header.
*/
struct Header {
  uint8_t identifier[12];
  uint32_t endianness;
  uint32_t glType;
  uint32_t glTypeSize;
  uint32_t glFormat;
  uint32_t glInternalFormat;
  uint32_t glBaseInternalFormat;
  uint32_t pixelWidth;
  uint32_t pixelHeight;
  uint32_t pixelDepth;
  uint32_t numberOfArrayElements;
  uint32_t numberOfFaces;
  uint32_t numberOfMipmapLevels;
  uint32_t bytesOfKeyValueData;
};

/** the eidianness.
*/
enum Endian {
  Endian_Little,
  Endian_Big,
  Endian_Unknown,
};

/** the internal format.
*/
enum Format {
  Format_ETC1 = 0x8d64,
  Format_ATC_E = 0x8c93,
  Format_ATC_I = 0x87ee,
};

/** the KTX file.
*/
struct File {
  /// Image data for each mip level.
  struct Data {
    uint32_t imageSize;
    std::vector<uint8_t> buf;
    Data() {}
  };
  Header header;
  std::vector<Data> data;
};

void initialize(Header* ktx, uint32_t w, uint32_t h, uint16_t format);
bool is_header(const Header& h);
uint32_t get_value(const uint32_t* pBuf, Endian e);
void set_value(uint32_t* pBuf, uint32_t value, Endian e);
bool read_texture(const std::string& filename, File& file);
bool write_texture(const std::string& filename, const File& file);

} // namespace KTX

#endif // KTX_H_INCLUDED
