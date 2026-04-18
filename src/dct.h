#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "blocks.h"

static constexpr float PI = 3.14159265358979323846f;

static float C[N][N];
static float Ct[N][N];

static inline float alpha(int u)
{
  return (u == 0) ? (1.0f / std::sqrt(8.0f)) : 0.5f;
}

static void init_dct()
{
  for (int u = 0; u < N; ++u)
    for (int x = 0; x < N; ++x)
      C[u][x] =
          alpha(u) *
          std::cos(((2 * x + 1) * u * PI) / 16.0f);

  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      Ct[j][i] = C[i][j];
}
static void matMul(const float A[N][N],
                   const float B[N][N],
                   float R[N][N])
{
  for (int i = 0; i < N; ++i)
  {
    for (int j = 0; j < N; ++j)
    {
      float sum = 0.0f;
      for (int k = 0; k < N; ++k)
        sum += A[i][k] * B[k][j];

      R[i][j] = sum;
    }
  }
}
static void dct8x8(const Block<uint8_t> &in, Block<float> &out)
{
  float src[N][N];
  float tmp[N][N];
  float dst[N][N];

  // level shift
  for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x)
      src[y][x] = (float)in(x, y) - 128.0f;

  matMul(C, src, tmp);
  matMul(tmp, Ct, dst);

  for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x)
      out(x, y) = dst[y][x];
}
static void idct8x8(const Block<float> &in, Block<uint8_t> &out)
{
  float src[N][N];
  float tmp[N][N];
  float dst[N][N];

  for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x)
      src[y][x] = in(x, y);

  matMul(Ct, src, tmp);
  matMul(tmp, C, dst);

  for (int y = 0; y < N; ++y)
  {
    for (int x = 0; x < N; ++x)
    {
      float v = dst[y][x] + 128.0f;
      v = std::clamp(v, 0.0f, 255.0f);
      out(x, y) = (uint8_t)std::lround(v);
    }
  }
}