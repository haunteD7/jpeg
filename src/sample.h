#pragma once

#include <vector>
#include <cstdint>

static std::vector<uint8_t> downsample_420(const std::vector<uint8_t> &in, int w, int h)
{
  size_t pixel_count = w * h;

  size_t cw = (w + 1) / 2;
  size_t ch = (h + 1) / 2;

  const uint8_t *Y = in.data();
  const uint8_t *Cb = Y + pixel_count;
  const uint8_t *Cr = Cb + pixel_count;

  std::vector<uint8_t> out(pixel_count * 3 / 2);

  uint8_t *Y_out = out.data();
  uint8_t *Cb_out = Y_out + pixel_count;
  uint8_t *Cr_out = Cb_out + (cw * ch);

  // copy Y
  std::copy(Y, Y + pixel_count, Y_out);

  for (size_t y = 0; y < ch; y++)
  {
    for (size_t x = 0; x < cw; x++)
    {
      int x0 = 2 * x;
      int y0 = 2 * y;

      auto get = [&](const uint8_t *c, int x, int y)
      {
        x = std::min(x, w - 1);
        y = std::min(y, h - 1);
        return c[y * w + x];
      };

      uint8_t c1 = get(Cb, x0, y0);
      uint8_t c2 = get(Cb, x0 + 1, y0);
      uint8_t c3 = get(Cb, x0, y0 + 1);
      uint8_t c4 = get(Cb, x0 + 1, y0 + 1);

      uint8_t r1 = get(Cr, x0, y0);
      uint8_t r2 = get(Cr, x0 + 1, y0);
      uint8_t r3 = get(Cr, x0, y0 + 1);
      uint8_t r4 = get(Cr, x0 + 1, y0 + 1);

      size_t idx = y * cw + x;

      Cb_out[idx] = (c1 + c2 + c3 + c4) / 4;
      Cr_out[idx] = (r1 + r2 + r3 + r4) / 4;
    }
  }

  return out;
}
static std::vector<uint8_t> upsample_420(const std::vector<uint8_t> &in, int w, int h)
{
  size_t pixel_count = w * h;

  size_t cw = (w + 1) / 2;
  size_t ch = (h + 1) / 2;

  size_t cb_size = cw * ch;

  std::vector<uint8_t> out(pixel_count * 3);

  uint8_t *Y = out.data();
  uint8_t *Cb = Y + pixel_count;
  uint8_t *Cr = Cb + pixel_count;

  const uint8_t *Y_in = in.data();
  const uint8_t *Cb_in = Y_in + pixel_count;
  const uint8_t *Cr_in = Cb_in + cb_size;

  // copy Y
  std::copy(Y_in, Y_in + pixel_count, Y);

  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      int sx = x / 2;
      int sy = y / 2;

      int sidx = sy * cw + sx;
      int didx = y * w + x;

      Cb[didx] = Cb_in[sidx];
      Cr[didx] = Cr_in[sidx];
    }
  }

  return out;
}