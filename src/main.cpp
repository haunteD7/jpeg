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

#include "blocks.h"
#include "ycbcr.h"
#include "sample.h"
#include "dct.h"
#include "quant.h"
#include "compress.h"
#include "tables.h"

#include "code.h"

Image load_image(const std::string &path)
{
  Image result;
  int channels;

  uint8_t *data = stbi_load(path.c_str(), &result.w, &result.h, &channels, 3);
  result.data.resize(result.w * result.h * 3);
  std::memcpy(result.data.data(), data, result.data.size());
  stbi_image_free(data);

  if (!data)
  {
    throw std::runtime_error("Cannot open file: " + path);
  }

  return result;
}
void save_image(const std::string &path, const Image &image)
{
  if (!stbi_write_bmp(path.c_str(), image.w, image.h, 3, image.data.data()))
  {
    throw std::runtime_error("Cannot save file: " + path);
  }
}
static void load_gepj(
    const std::string &path,
    int &w, int &h,
    float qY[8][8],
    float qC[8][8],
    std::vector<uint8_t> &bitstream)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("open failed");

  char magic[4];
  in.read(magic, 4);
  if (std::memcmp(magic, "GEPJ", 4) != 0)
    throw std::runtime_error("bad format");

  in.read((char *)&w, 4);
  in.read((char *)&h, 4);

  // квантование
  in.read((char *)qY, 64 * sizeof(float));
  in.read((char *)qC, 64 * sizeof(float));

  int ydc, cbdc, crdc, bs;
  in.read((char *)&ydc, 4);
  in.read((char *)&cbdc, 4);
  in.read((char *)&crdc, 4);
  in.read((char *)&bs, 4);

  bitstream.resize(bs);

  in.read((char *)bitstream.data(), bs);
}
static void save_gepj(
    const std::string &path,
    int w, int h,
    const float qY[8][8],
    const float qC[8][8],
    std::span<const uint8_t> bitstream)
{
  std::ofstream out(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("open failed");

  out.write("GEPJ", 4);

  out.write((char *)&w, 4);
  out.write((char *)&h, 4);

  // квантование
  out.write((char *)qY, 64 * sizeof(float));
  out.write((char *)qC, 64 * sizeof(float));

  int bs = bitstream.size();

  out.write((char *)&bs, 4);

  out.write((char *)bitstream.data(), bs);
}

std::string files[] =
    {
        "lena.png",
        "img.bmp",
        "img_grey.bmp",
        "img_bw.bmp",
        "img_dither.bmp"};
float step = 10;
float initial = 10;

int main(int argc, char const *argv[])
{
  init_dct();

  
  for (const auto &name : files)
  {
    std::stringstream ss;
    ss << "Quality;Compression\n";
    for (float quality = initial; quality < 100; quality += 10)
    {

      Image img = load_image(name);
      size_t pixels = img.w * img.h;

      std::vector<uint8_t> y(pixels);
      std::vector<uint8_t> cb(pixels);
      std::vector<uint8_t> cr(pixels);

      RGB_to_YCbCr(img.data, y, cb, cr);

      size_t w_blocks = (img.w + 7) / 8;
      size_t h_blocks = (img.h + 7) / 8;
      size_t w_down = (img.w + 1) / 2;
      size_t h_down = (img.h + 1) / 2;
      size_t w_down_blocks = (w_down + 7) / 8;
      size_t h_down_blocks = (h_down + 7) / 8;
      std::vector<uint8_t> cb_down(w_down * h_down);
      std::vector<uint8_t> cr_down(w_down * h_down);
      downsample_420(cb, cr, cb_down, cr_down, img.w, img.h);
      std::vector<Block<uint8_t>> y_blocks(w_blocks * h_blocks);
      std::vector<Block<uint8_t>> cb_blocks(w_down_blocks * h_down_blocks);
      std::vector<Block<uint8_t>> cr_blocks(w_down_blocks * h_down_blocks);
      split_to_blocks(
          std::span<const uint8_t>(y),
          std::span<Block<uint8_t>>(y_blocks),
          img.w, img.h);
      split_to_blocks(
          std::span<const uint8_t>(cb_down),
          std::span<Block<uint8_t>>(cb_blocks),
          w_down, h_down);
      split_to_blocks(
          std::span<const uint8_t>(cr_down),
          std::span<Block<uint8_t>>(cr_blocks),
          w_down, h_down);

      std::vector<Block<float>> y_dct(w_blocks * h_blocks);
      std::vector<Block<float>> cb_dct(w_down_blocks * h_down_blocks);
      std::vector<Block<float>> cr_dct(w_down_blocks * h_down_blocks);
      std::vector<Block<int16_t>> y_quant(w_blocks * h_blocks);
      std::vector<Block<int16_t>> cb_quant(w_down_blocks * h_down_blocks);
      std::vector<Block<int16_t>> cr_quant(w_down_blocks * h_down_blocks);

      float q_mat_Y[8][8];
      float q_mat_C[8][8];
      calculate_quant_mat(mat_Y, q_mat_Y, quality);
      calculate_quant_mat(mat_C, q_mat_C, quality);
      for (size_t i = 0; i < y_blocks.size(); i++)
      {
        dct8x8(y_blocks[i], y_dct[i]);
        quant(y_dct[i], y_quant[i], q_mat_Y);
      }
      for (size_t i = 0; i < cb_blocks.size(); i++)
      {
        dct8x8(cb_blocks[i], cb_dct[i]);
        quant(cb_dct[i], cb_quant[i], q_mat_C);
      }
      for (size_t i = 0; i < cr_blocks.size(); i++)
      {
        dct8x8(cr_blocks[i], cr_dct[i]);
        quant(cr_dct[i], cr_quant[i], q_mat_C);
      }

      std::vector<int16_t> y_dc(w_blocks * h_blocks);
      std::vector<int16_t> cb_dc(w_down_blocks * h_down_blocks);
      std::vector<int16_t> cr_dc(w_down_blocks * h_down_blocks);
      std::vector<int16_t> y_ac(w_blocks * h_blocks * 63);
      std::vector<int16_t> cb_ac(w_down_blocks * h_down_blocks * 63);
      std::vector<int16_t> cr_ac(w_down_blocks * h_down_blocks * 63);
      zigzag_encode(y_quant, y_dc, y_ac);
      zigzag_encode(cb_quant, cb_dc, cb_ac);
      zigzag_encode(cr_quant, cr_dc, cr_ac);
      dc_dpcm_encode(y_dc);
      dc_dpcm_encode(cb_dc);
      dc_dpcm_encode(cr_dc);
      std::vector<AC_Symbol> y_rle(w_blocks * h_blocks * 63);
      std::vector<AC_Symbol> cb_rle(w_down_blocks * h_down_blocks * 63);
      std::vector<AC_Symbol> cr_rle(w_down_blocks * h_down_blocks * 63);
      size_t y_rle_size = rle_encode(y_ac, y_rle);
      size_t cb_rle_size = rle_encode(cb_ac, cb_rle);
      size_t cr_rle_size = rle_encode(cr_ac, cr_rle);

      std::vector<HuffCode> dc_y(256), dc_c(256);
      std::vector<HuffCode> ac_y(256), ac_c(256);

      build_canonical_huffman(dc_lum_bits, dc_lum_vals, dc_y);
      build_canonical_huffman(dc_chroma_bits, dc_chroma_vals, dc_c);
      build_canonical_huffman(ac_lum_bits, ac_lum_vals, ac_y);
      build_canonical_huffman(ac_chroma_bits, ac_chroma_vals, ac_c);

      DecodeTable dc_y_dec;
      DecodeTable dc_c_dec;
      DecodeTable ac_y_dec;
      DecodeTable ac_c_dec;

      build_decode(dc_lum_bits, dc_lum_vals, dc_y_dec);
      build_decode(dc_chroma_bits, dc_chroma_vals, dc_c_dec);
      build_decode(ac_lum_bits, ac_lum_vals, ac_y_dec);
      build_decode(ac_chroma_bits, ac_chroma_vals, ac_c_dec);
      std::vector<uint8_t> bitstream;
      BitWriter bw{bitstream};
      encode_dc(y_dc, dc_y, bw);
      encode_dc(cb_dc, dc_c, bw);
      encode_dc(cr_dc, dc_c, bw);
      encode_ac(std::span(y_rle.data(), y_rle_size), ac_y, bw);
      encode_ac(std::span(cb_rle.data(), cb_rle_size), ac_c, bw);
      encode_ac(std::span(cr_rle.data(), cr_rle_size), ac_c, bw);
      bw.flush();

      BitReader br{bitstream.data(), bitstream.size()};
      decode_dc(y_dc, br, dc_y_dec);
      decode_dc(cb_dc, br, dc_c_dec);
      decode_dc(cr_dc, br, dc_c_dec);
      decode_ac(br, ac_y_dec, y_ac);
      decode_ac(br, ac_c_dec, cb_ac);
      decode_ac(br, ac_c_dec, cr_ac);
      size_t rle_pos = 0;
      rle_decode(y_rle, rle_pos, y_ac);
      rle_pos = 0;
      rle_decode(cb_rle, rle_pos, cb_ac);
      rle_pos = 0;
      rle_decode(cr_rle, rle_pos, cr_ac);
      dc_dpcm_decode(y_dc);
      dc_dpcm_decode(cb_dc);
      dc_dpcm_decode(cr_dc);
      zigzag_decode(y_dc, y_ac, y_quant);
      zigzag_decode(cb_dc, cb_ac, cb_quant);
      zigzag_decode(cr_dc, cr_ac, cr_quant);

      for (size_t i = 0; i < y_blocks.size(); i++)
      {
        dequant(y_quant[i], y_dct[i], q_mat_Y);
        idct8x8(y_dct[i], y_blocks[i]);
      }
      for (size_t i = 0; i < cb_blocks.size(); i++)
      {
        dequant(cb_quant[i], cb_dct[i], q_mat_C);
        idct8x8(cb_dct[i], cb_blocks[i]);
      }
      for (size_t i = 0; i < cr_blocks.size(); i++)
      {
        dequant(cr_quant[i], cr_dct[i], q_mat_C);
        idct8x8(cr_dct[i], cr_blocks[i]);
      }
      build_from_blocks(
          std::span<const Block<uint8_t>>(y_blocks),
          std::span<uint8_t>(y),
          img.w, img.h);
      build_from_blocks(
          std::span<const Block<uint8_t>>(cb_blocks),
          std::span<uint8_t>(cb_down),
          w_down, h_down);
      ;
      build_from_blocks(
          std::span<const Block<uint8_t>>(cr_blocks),
          std::span<uint8_t>(cr_down),
          w_down, h_down);

      upsample_420(cb_down, cr_down, cb, cr, img.w, img.h);
      YCbCr_to_RGB(y, cb, cr, img.data);

      std::cout << name << " " << quality << "\n";

      ss << quality << ";" << (float)img.data.size() / (float)bitstream.size() << "\n";
      save_image(name + "_" + std::to_string(quality) + ".bmp", img);
    }
    std::ofstream benchmark_fs("benchmark_" + name + ".csv");
    benchmark_fs << ss.rdbuf();
  }
  return 0;
}
