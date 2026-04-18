#pragma once

#include "blocks.h"

#include <cstdint>

static constexpr int zigzag[8][8] = {
    {0, 1, 5, 6, 14, 15, 27, 28},
    {2, 4, 7, 13, 16, 26, 29, 42},
    {3, 8, 12, 17, 25, 30, 41, 43},
    {9, 11, 18, 24, 31, 40, 44, 53},
    {10, 19, 23, 32, 39, 45, 52, 54},
    {20, 22, 33, 38, 46, 51, 55, 60},
    {21, 34, 37, 47, 50, 56, 59, 61},
    {35, 36, 48, 49, 57, 58, 62, 63}};

static void zigzag_encode(
    std::span<const Block<int16_t>> in,
    std::span<int16_t> dc_out,
    std::span<int16_t> ac_out)
{
  for (size_t i = 0; i < in.size(); i++)
  {
    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        int idx = zigzag[y][x];

        if (idx == 0)
        {
          // DC factor
          dc_out[i] = in[i](x, y);
        }
        else
        {
          // AC factor
          ac_out[i * 63 + (idx - 1)] = in[i](x, y);
        }
      }
    }
  }
}

static void zigzag_decode(
    std::span<const int16_t> dc_in,
    std::span<const int16_t> ac_in,
    std::span<Block<int16_t>> out)
{
  for (size_t i = 0; i < out.size(); i++)
  {
    for (int y = 0; y < 8; ++y)
    {
      for (int x = 0; x < 8; ++x)
      {
        int idx = zigzag[y][x];

        if (idx == 0)
        {
          // DC
          out[i](x, y) = dc_in[i];
        }
        else
        {
          // AC
          out[i](x, y) = ac_in[i * 63 + (idx - 1)];
        }
      }
    }
  }
}