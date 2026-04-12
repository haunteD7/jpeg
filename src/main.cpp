#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "ycbcr.h"
#include "sample.h"
#include "dct.h"

struct Image
{
  int w, h;
  std::vector<uint8_t> data;
};

Image load_image(const std::string& path)
{
  Image result;
  int channels;

  uint8_t* data = stbi_load(path.c_str(), &result.w, &result.h, &channels, 3);
  result.data.resize(result.w * result.h * channels);
  std::memcpy(result.data.data(), data, result.data.size());
  stbi_image_free(data);

  if(!data)
  {
    throw std::runtime_error("Cannot open file: " + path);
  }

  return result;
}
void save_image(const std::string& path, const Image& image)
{
  if(!stbi_write_bmp(path.c_str(), image.w, image.h, 3, image.data.data()))
  {
    throw std::runtime_error("Cannot save file: " + path);
  }
}

int main(int argc, char const *argv[])
{
  Image img = load_image("test.bmp");

  img.data = YCbCr_to_RGB(upsample_420(downsample_420(RGB_to_YCbCr(img.data), img.w, img.h), img.w, img.h));
  
  save_image("result.bmp", img);
  return 0;
}
