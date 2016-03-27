/**
  @file ktx.cpp
*/
#include "ktx.h"
#include <fstream>
#include <iostream>
#include <string>

namespace KTX {

/// KTX header identifier array.
static const uint8_t fileIdentifier[12] = {
  0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

/// Initialize KTX header for the compressed format.
void initialize(Header* ktx, uint32_t w, uint32_t h, uint16_t format)
{
  for (int i = 0; i < sizeof(fileIdentifier); ++i) {
    ktx->identifier[i] = fileIdentifier[i];
  }
  ktx->endianness = 0x04030201;
  ktx->glType = 0;
  ktx->glTypeSize = 0;
  ktx->glFormat = 0;
  ktx->glInternalFormat = format;
  ktx->glBaseInternalFormat = format;
  ktx->pixelWidth = w;
  ktx->pixelHeight = h;
  ktx->pixelDepth = 0;
  ktx->numberOfArrayElements = 0;
  ktx->numberOfFaces = 1;
  ktx->numberOfMipmapLevels = 1;
  ktx->bytesOfKeyValueData = 0;
}

/** Check the header is valid.
*/
bool is_ktx_header(const Header& h)
{
  for (int i = 0; i < sizeof(fileIdentifier); ++i) {
    if (h.identifier[i] != fileIdentifier[i]) {
      return false;
    }
  }
  return true;
}

/** get KTX endianness.
*/
Endian get_endian(const Header& h)
{
  const uint8_t* p = reinterpret_cast<const uint8_t*>(&h.endianness);
  if (p[0] == 0x01 && p[1] == 0x02 && p[2] == 0x03 && p[3] == 0x04) {
    return Endian_Little;
  } else if (p[0] == 0x04 && p[1] == 0x03 && p[2] == 0x02 && p[3] == 0x01) {
    return Endian_Big;
  }
  return Endian_Unknown;
}

/** get value with endian.

  @ref get_endian()
*/
uint32_t get_value(const uint32_t* pBuf, Endian e)
{
  const uint8_t* p = reinterpret_cast<const uint8_t*>(pBuf);
  if (e == Endian_Little) {
    return p[3] * 0x1000000 + p[2] * 0x10000 + p[1] * 0x100 + p[0];
  }
  return p[0] * 0x1000000 + p[1] * 0x10000 + p[2] * 0x100 + p[3];
}

/** set value with endian.

  @ref get_endian()
*/
void set_value(uint32_t* pBuf, uint32_t value, Endian e)
{
  uint8_t* p = reinterpret_cast<uint8_t*>(pBuf);
  if (e == Endian_Little) {
    p[0] = (value >>  0) & 0xff;
    p[1] = (value >>  8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
  } else {
    p[0] = (value >> 24) & 0xff;
    p[1] = (value >> 16) & 0xff;
    p[2] = (value >>  8) & 0xff;
    p[3] = (value >>  0) & 0xff;
  }
}

/** read texture file.
*/
bool read_texture(const std::string& filename, File& file)
{
  std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
  if (ifs.bad()) {
    std::cout << "can't open'" << filename << "'";
    return false;
  }

  ifs.read(reinterpret_cast<char*>(&file.header), sizeof(Header));
  if (ifs.bad()) {
    std::cout << "can't read '" << filename << "'";
    return false;
  }
  if (!is_ktx_header(file.header)) {
    std::cout << "it isn't KTX file '" << filename << "'";
    return false;
  }
  
  const Endian endianness = get_endian(file.header);
  const int faceCount = get_value(&file.header.numberOfFaces, endianness);
  const int mipCount = get_value(&file.header.numberOfMipmapLevels, endianness);
  if (faceCount != 1) {
    std::cout << "wrong face count(" << faceCount << "). it should be 1 '" << filename << "'";
    return false;
  }
  if (mipCount < 0 || mipCount > 32) {
    std::cout << "wrong mip count(" << faceCount << "). it should be 0 between 32 '" << filename << "'";
    return false;
  }
  
  const uint32_t off = get_value(&file.header.bytesOfKeyValueData, endianness);
  ifs.seekg(off, std::ios::cur);
  if (ifs.bad()) {
    std::cout << "can't read '" << filename << "'";
    return false;
  }

  std::vector<uint8_t>  data;
  for (int mipLevel = 0; mipLevel < (mipCount ? mipCount : 1); ++mipLevel) {
    uint32_t imageSize;
    ifs.read(reinterpret_cast<char*>(&imageSize), sizeof(uint32_t));
    if (ifs.bad()) {
      std::cout << "can't read(miplevel=" << mipLevel << "):'" << filename << "'";
      return false;
    }
    imageSize = get_value(&imageSize, endianness);
    const uint32_t imageSizeWithPadding = (imageSize + 3) & ~3;
    data.resize(imageSizeWithPadding * faceCount);
    ifs.read(reinterpret_cast<char*>(&data[0]), data.size());
    if (ifs.bad()) {
      std::cout << "can't read(miplevel=" << mipLevel << "):'" << filename << "'";
      return false;
    }
    file.data.push_back(File::Data());
    file.data.back().imageSize = imageSize;
    file.data.back().buf.swap(data);
  }
  return true;
}

/** Write texture file.
*/
bool write_texture(const std::string& filename, const File& ktxfile)
{
  Header  header = ktxfile.header;
  header.numberOfFaces = 1;
  header.bytesOfKeyValueData = 0;
  uint32_t dataSize = 0;
  for (auto& e : ktxfile.data) {
    dataSize += static_cast<uint32_t>(e.buf.size() + sizeof(uint32_t));
  }

  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(Header) + dataSize);

  buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&header), reinterpret_cast<uint8_t*>(&header) + sizeof(Header));
  
  const Endian endianness = get_endian(header);
  const int mipCount = get_value(&header.numberOfMipmapLevels, endianness);
  for (int mipLevel = 0; mipLevel < (mipCount ? mipCount : 1); ++mipLevel) {
    uint32_t imageSize;
    set_value(&imageSize, ktxfile.data[mipLevel].imageSize, endianness);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&imageSize), reinterpret_cast<uint8_t*>(&imageSize) + sizeof(imageSize));
    buffer.insert(buffer.end(), ktxfile.data[mipLevel].buf.begin(), ktxfile.data[mipLevel].buf.end());
  }

  std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::binary);
  if (ofs.bad()) {
    std::cout << "can't open'" << filename << "'";
    return false;
  }
  ofs.write(reinterpret_cast<char*>(&buffer[0]), buffer.size());
  return true;
}

/** Write cubemap texture file.
*/
bool write_cubemap(const std::string& filename, const std::vector<File>& ktxfiles)
{
  Header  header = ktxfiles[0].header;
  header.numberOfFaces = 6;
  header.bytesOfKeyValueData = 0;
  uint32_t dataSizePerFace = 0;
  for (auto& e : ktxfiles[0].data) {
    dataSizePerFace += static_cast<uint32_t>(e.buf.size() + sizeof(uint32_t));
  }

  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(Header) + dataSizePerFace * 6);

  buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&header), reinterpret_cast<uint8_t*>(&header) + sizeof(Header));
  
  const Endian endianness = get_endian(header);
  const int mipCount = get_value(&header.numberOfMipmapLevels, endianness);
  for (int mipLevel = 0; mipLevel < (mipCount ? mipCount : 1); ++mipLevel) {
    uint32_t imageSize;
    set_value(&imageSize, ktxfiles[0].data[mipLevel].imageSize, endianness);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&imageSize), reinterpret_cast<uint8_t*>(&imageSize) + sizeof(imageSize));
    for (auto& e : ktxfiles) {
      buffer.insert(buffer.end(), e.data[mipLevel].buf.begin(), e.data[mipLevel].buf.end());
    }
  }

  std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::binary);
  if (ofs.bad()) {
    std::cout << "can't open'" << filename << "'";
    return false;
  }
  ofs.write(reinterpret_cast<char*>(&buffer[0]), buffer.size());
  return true;
}

} // namespace KTX
