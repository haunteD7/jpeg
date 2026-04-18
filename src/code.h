#pragma once

#include "blocks.h"
#include "ycbcr.h"
#include "sample.h"
#include "dct.h"
#include "quant.h"
#include "compress.h"
#include "tables.h"

#include <vector>

struct Image
{
  int w, h;
  std::vector<uint8_t> data;
};

// auto encode_image(const Image &img, float qY[8][8], float qC[8][8])
// {
//   size_t pixels = img.w * img.h;

//   std::vector<uint8_t> y(pixels), cb(pixels), cr(pixels);
//   RGB_to_YCbCr(img.data, y, cb, cr);

//   // downsample 4:2:0
//   size_t w_down = (img.w + 1) / 2;
//   size_t h_down = (img.h + 1) / 2;

//   std::vector<uint8_t> cb_down(w_down * h_down);
//   std::vector<uint8_t> cr_down(w_down * h_down);
//   downsample_420(cb, cr, cb_down, cr_down, img.w, img.h);

//   // blocks
//   size_t w_blocks = (img.w + 7) / 8;
//   size_t h_blocks = (img.h + 7) / 8;
//   size_t w_down_blocks = (w_down + 7) / 8;
//   size_t h_down_blocks = (h_down + 7) / 8;

//   std::vector<Block<uint8_t>> y_blocks(w_blocks * h_blocks);
//   std::vector<Block<uint8_t>> cb_blocks(w_down_blocks * h_down_blocks);
//   std::vector<Block<uint8_t>> cr_blocks(w_down_blocks * h_down_blocks);

//   split_to_blocks(
//       std::span<const uint8_t>(y),
//       std::span<Block<uint8_t>>(y_blocks),
//       img.w, img.h);
//   split_to_blocks(
//       std::span<const uint8_t>(cb_down),
//       std::span<Block<uint8_t>>(cb_blocks),
//       w_down, h_down);
//   split_to_blocks(
//       std::span<const uint8_t>(cr_down),
//       std::span<Block<uint8_t>>(cr_blocks),
//       w_down, h_down);

//   // DCT + quant
//   std::vector<Block<int16_t>> y_quant(y_blocks.size());
//   std::vector<Block<int16_t>> cb_quant(cb_blocks.size());
//   std::vector<Block<int16_t>> cr_quant(cr_blocks.size());

//   for (size_t i = 0; i < y_blocks.size(); i++)
//   {
//     Block<float> tmp;
//     dct8x8(y_blocks[i], tmp);
//     quant(tmp, y_quant[i], qY);
//   }

//   for (size_t i = 0; i < cb_blocks.size(); i++)
//   {
//     Block<float> tmp;
//     dct8x8(cb_blocks[i], tmp);
//     quant(tmp, cb_quant[i], qC);
//   }

//   for (size_t i = 0; i < cr_blocks.size(); i++)
//   {
//     Block<float> tmp;
//     dct8x8(cr_blocks[i], tmp);
//     quant(tmp, cr_quant[i], qC);
//   }

//   // zigzag + DPCM + RLE
//   std::vector<int16_t> y_dc(w_blocks * h_blocks);
//   std::vector<int16_t> cb_dc(w_down_blocks * h_down_blocks);
//   std::vector<int16_t> cr_dc(w_down_blocks * h_down_blocks);
//   std::vector<int16_t> y_ac(w_blocks * h_blocks * 63);
//   std::vector<int16_t> cb_ac(w_down_blocks * h_down_blocks * 63);
//   std::vector<int16_t> cr_ac(w_down_blocks * h_down_blocks * 63);

//   zigzag_encode(y_quant, y_dc, y_ac);
//   zigzag_encode(cb_quant, cb_dc, cb_ac);
//   zigzag_encode(cr_quant, cr_dc, cr_ac);

//   dc_dpcm_encode(y_dc);
//   dc_dpcm_encode(cb_dc);
//   dc_dpcm_encode(cr_dc);

//   std::vector<AC_Symbol> y_rle(y_ac.size());
//   std::vector<AC_Symbol> cb_rle(cb_ac.size());
//   std::vector<AC_Symbol> cr_rle(cr_ac.size());

//   size_t y_rle_size = rle_encode(y_ac, y_rle);
//   size_t cb_rle_size = rle_encode(cb_ac, cb_rle);
//   size_t cr_rle_size = rle_encode(cr_ac, cr_rle);

//   // Huffman
//   std::vector<HuffCode> dc_y(256), dc_c(256);
//   std::vector<HuffCode> ac_y(256), ac_c(256);

//   build_canonical_huffman(dc_lum_bits, dc_lum_vals, dc_y);
//   build_canonical_huffman(dc_chroma_bits, dc_chroma_vals, dc_c);
//   build_canonical_huffman(ac_lum_bits, ac_lum_vals, ac_y);
//   build_canonical_huffman(ac_chroma_bits, ac_chroma_vals, ac_c);

//   std::vector<uint8_t> bitstream;
//   BitWriter bw{bitstream};

//   encode_dc(y_dc, dc_y, bw);
//   encode_ac({y_rle.data(), y_rle_size}, ac_y, bw);

//   encode_dc(cb_dc, dc_c, bw);
//   encode_ac({cb_rle.data(), cb_rle_size}, ac_c, bw);

//   encode_dc(cr_dc, dc_c, bw);
//   encode_ac({cr_rle.data(), cr_rle_size}, ac_c, bw);

//   bw.flush();

//   return bitstream;
// }

// auto decode_image(const std::vector<uint8_t> &bitstream, int w, int h, float qY[8][8], float qC[8][8])
// {
//   Image result;
//   result.w = w;
//   result.h = h;

//   size_t pixels = w * h;

//   std::vector<uint8_t> y(pixels), cb(pixels), cr(pixels);

//   size_t w_down = (w + 1) / 2;
//   size_t h_down = (h + 1) / 2;

//   size_t w_blocks = (w + 7) / 8;
//   size_t h_blocks = (h + 7) / 8;
//   size_t w_down_blocks = (w_down + 7) / 8;
//   size_t h_down_blocks = (h_down + 7) / 8;

//   // Huffman decode tables
//   DecodeTable dc_y, dc_c, ac_y, ac_c;

//   build_decode(dc_lum_bits, dc_lum_vals, dc_y);
//   build_decode(dc_chroma_bits, dc_chroma_vals, dc_c);
//   build_decode(ac_lum_bits, ac_lum_vals, ac_y);
//   build_decode(ac_chroma_bits, ac_chroma_vals, ac_c);

//   std::vector<int16_t> y_dc(w_blocks * h_blocks);
//   std::vector<int16_t> cb_dc(w_down_blocks * h_down_blocks);
//   std::vector<int16_t> cr_dc(w_down_blocks * h_down_blocks);

//   std::vector<int16_t> y_ac(w_blocks * h_blocks * 63);
//   std::vector<int16_t> cb_ac(w_down_blocks * h_down_blocks * 63);
//   std::vector<int16_t> cr_ac(w_down_blocks * h_down_blocks * 63);

//   BitReader br{bitstream.data(), bitstream.size()};

//   // Y
//   std::vector<AC_Symbol> y_rle(w_blocks * h_blocks * 63);
//   size_t y_rle_size;

//   decode_dc(y_dc, br, dc_y);
//   decode_ac(br, ac_y, y_rle, y_rle_size, w_blocks * h_blocks);
//   size_t pos = 0;
//   rle_decode(
//       std::span<const AC_Symbol>(y_rle.data(), y_rle_size),
//       pos,
//       y_ac);

//   // Cb
//   std::vector<AC_Symbol> cb_rle(w_down_blocks * h_down_blocks * 63);
//   size_t cb_rle_size;

//   decode_dc(cb_dc, br, dc_c);
//   decode_ac(br, ac_c, cb_rle, cb_rle_size, w_down_blocks * h_down_blocks);
//   pos = 0;
//   rle_decode(
//       std::span<const AC_Symbol>(cb_rle.data(), cb_rle_size),
//       pos,
//       cb_ac);

//   // Cr
//   std::vector<AC_Symbol> cr_rle(w_down_blocks * h_down_blocks * 63);
//   size_t cr_rle_size;

//   decode_dc(cr_dc, br, dc_c);
//   decode_ac(br, ac_c, cr_rle, cr_rle_size, w_down_blocks * h_down_blocks);
//   pos = 0;
//   rle_decode(
//       std::span<const AC_Symbol>(cr_rle.data(), cr_rle_size),
//       pos,
//       cr_ac);

//   dc_dpcm_decode(y_dc);
//   dc_dpcm_decode(cb_dc);
//   dc_dpcm_decode(cr_dc);

//   std::vector<Block<int16_t>> y_quant(w_blocks * h_blocks);
//   std::vector<Block<int16_t>> cb_quant(w_down_blocks * h_down_blocks);
//   std::vector<Block<int16_t>> cr_quant(w_down_blocks * h_down_blocks);

//   zigzag_decode(y_dc, y_ac, y_quant);
//   zigzag_decode(cb_dc, cb_ac, cb_quant);
//   zigzag_decode(cr_dc, cr_ac, cr_quant);

//   std::vector<Block<uint8_t>> y_blocks(y_quant.size());
//   std::vector<Block<uint8_t>> cb_blocks(cb_quant.size());
//   std::vector<Block<uint8_t>> cr_blocks(cr_quant.size());

//   for (size_t i = 0; i < y_quant.size(); i++)
//   {
//     Block<float> tmp;
//     dequant(y_quant[i], tmp, qY);
//     idct8x8(tmp, y_blocks[i]);
//   }

//   for (size_t i = 0; i < cb_quant.size(); i++)
//   {
//     Block<float> tmp;
//     dequant(cb_quant[i], tmp, qC);
//     idct8x8(tmp, cb_blocks[i]);
//   }

//   for (size_t i = 0; i < cr_quant.size(); i++)
//   {
//     Block<float> tmp;
//     dequant(cr_quant[i], tmp, qC);
//     idct8x8(tmp, cr_blocks[i]);
//   }

//   std::vector<uint8_t> cb_down(w_down * h_down);
//   std::vector<uint8_t> cr_down(w_down * h_down);

//   build_from_blocks(
//       std::span<const Block<uint8_t>>(y_blocks),
//       std::span<uint8_t>(y),
//       w, h);
//   build_from_blocks(
//       std::span<const Block<uint8_t>>(cb_blocks),
//       std::span<uint8_t>(cb_down),
//       w_down, h_down);
//   ;
//   build_from_blocks(
//       std::span<const Block<uint8_t>>(cr_blocks),
//       std::span<uint8_t>(cr_down),
//       w_down, h_down);

//   upsample_420(cb_down, cr_down, cb, cr, w, h);

//   Image img{w, h, std::vector<uint8_t>(w * h * 3)};
//   YCbCr_to_RGB(y, cb, cr, img.data);

//   return img;
// }