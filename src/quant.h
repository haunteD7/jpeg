#pragma once

#include <span>
#include <cstdint>

static constexpr float mat_Y[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};
static constexpr float mat_C[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
};

static void calculate_quant_mat(const float in[8][8], float out[8][8], float quality)
{
  float S;
  if (quality < 50)
    S = 5000.0f / quality;
  else
    S = 200.0f - 2.0f * quality;

  S /= 100.0f;

  for(size_t j = 0; j < 8; j++)
  {
    for(size_t i = 0; i < 8; i++) 
    {
      out[j][i] = in[j][i] * S;
    }
  }
}
static void quant(const Block<float>& in, Block<int16_t>& out, const float mat[8][8])
{
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < 8; ++x)
      out(x, y) = (int16_t)std::round(in(x, y) / mat[y][x]);
}
static void dequant(const Block<int16_t>& in, Block<float>& out, const float mat[8][8])
{
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < 8; ++x)
      out(x, y) = in(x, y) * mat[y][x];
}