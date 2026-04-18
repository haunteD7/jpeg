#pragma once

#include <cstdint>
#include <span>

static constexpr size_t N = 8;
static constexpr size_t M = 8;
template <typename T>
struct Block
{
  T v[N*M];

  T& operator()(size_t x, size_t y)
  {
    return v[y * M + x];
  }
  const T& operator()(size_t x, size_t y) const
  {
    return v[y * M + x];
  }
};

template <typename T>
static inline T get_clamped(
    std::span<const T> img,
    size_t x, size_t y, size_t w, size_t h)
{
  x = std::min(x, w - 1);
  y = std::min(y, h - 1);
  return img[y * w + x];
}

template <typename T>
void split_to_blocks(std::span<const T> in, std::span<Block<T>> out, size_t w, size_t h)
{
  size_t width_blocks = ((w + N - 1) / N);
  size_t height_blocks = ((h + M - 1) / M);

  // safety
  if (out.size() < width_blocks * height_blocks)
    throw std::runtime_error("out too small");

  for (size_t by = 0; by < height_blocks; by++)
    for (size_t bx = 0; bx < width_blocks; bx++)
    {
      // load block
      for (size_t y = 0; y < M; ++y)
        for (size_t x = 0; x < N; ++x)
        {
          T v = get_clamped(in, bx * N + x, by * M + y, w, h);
          out[by * width_blocks + bx](x, y) = v;
        }
    }
}
template <typename T>
void build_from_blocks(std::span<const Block<T>> in, std::span<T> out, size_t w, size_t h)
{
  size_t width_blocks = ((w + N - 1) / N);
  size_t height_blocks = ((h + M - 1) / M);

  for (size_t by = 0; by < height_blocks; by++)
    for (size_t bx = 0; bx < width_blocks; bx++)
    {
      size_t block_idx = by * width_blocks + bx;
      for (size_t y = 0; y < M; ++y)
        for (size_t x = 0; x < N; ++x)
        {
          size_t X = bx * N + x;
          size_t Y = by * M + y;
          if(X >= w || Y >= h) continue;

          size_t pixel_idx = Y * w + X;
          out[pixel_idx] = in[block_idx](x, y);
        }
    }
}