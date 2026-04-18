#pragma once

#include <span>
#include <cstdint>

static inline uint8_t get_px(
  std::span<const uint8_t> c, int x, int y, int w, int h)
{
  x = std::min(x, w - 1);
  y = std::min(y, h - 1);
  return c[y * w + x];
}

static void downsample_420(
  std::span<const uint8_t> in_cb,
  std::span<const uint8_t> in_cr,
  std::span<uint8_t> out_cb,
  std::span<uint8_t> out_cr,
  int w, int h)
{
  size_t cw = (w + 1) / 2;
  size_t ch = (h + 1) / 2;

  for (size_t y = 0; y < ch; y++)
  {
    for (size_t x = 0; x < cw; x++)
    {
      int x0 = x * 2;
      int y0 = y * 2;

      uint8_t c1 = get_px(in_cb, x0, y0, w, h);
      uint8_t c2 = get_px(in_cb, x0 + 1, y0, w, h);
      uint8_t c3 = get_px(in_cb, x0, y0 + 1, w, h);
      uint8_t c4 = get_px(in_cb, x0 + 1, y0 + 1, w, h);

      uint8_t r1 = get_px(in_cr, x0, y0, w, h);
      uint8_t r2 = get_px(in_cr, x0 + 1, y0, w, h);
      uint8_t r3 = get_px(in_cr, x0, y0 + 1, w, h);
      uint8_t r4 = get_px(in_cr, x0 + 1, y0 + 1, w, h);

      size_t idx = y * cw + x;

      out_cb[idx] = (c1 + c2 + c3 + c4 + 2) >> 2;
      out_cr[idx] = (r1 + r2 + r3 + r4 + 2) >> 2;
    }
  }
}
static void upsample_420(
  std::span<const uint8_t> in_cb,
  std::span<const uint8_t> in_cr,
  std::span<uint8_t> out_cb,
  std::span<uint8_t> out_cr,
  int w, int h)
{
  size_t cw = (w + 1) / 2;

  for (size_t y = 0; y < (size_t)h; y++)
  {
    for (size_t x = 0; x < (size_t)w; x++)
    {
      size_t sx = x / 2;
      size_t sy = y / 2;
      size_t sidx = sy * cw + sx;
      size_t didx = y * w + x;

      out_cb[didx] = in_cb[sidx];
      out_cr[didx] = in_cr[sidx];
    }
  }
}