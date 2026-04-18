#pragma once

#include <span>
#include <cstdint>

static void RGB_to_YCbCr(
  std::span<const uint8_t> rgb_in,
  std::span<uint8_t> y_out,
  std::span<uint8_t> cb_out,
  std::span<uint8_t> cr_out) /* Input in interleaved, output in planar */
{
  for (size_t i = 0; i < rgb_in.size() / 3; i++)
  {
    int r = rgb_in[i * 3 + 0];
    int g = rgb_in[i * 3 + 1];
    int b = rgb_in[i * 3 + 2];

    int y  = ( 77 * r + 150 * g +  29 * b) >> 8;
    int cb = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
    int cr = ((128 * r - 107 * g - 21 * b) >> 8) + 128;

    y_out[i]  = static_cast<uint8_t>(std::clamp(y,  0, 255));
    cb_out[i] = static_cast<uint8_t>(std::clamp(cb, 0, 255));
    cr_out[i] = static_cast<uint8_t>(std::clamp(cr, 0, 255));
  }
}

static void YCbCr_to_RGB(
  std::span<const uint8_t> y_in,
  std::span<const uint8_t> cb_in, 
  std::span<const uint8_t> cr_in, 
  std::span<uint8_t> rgb_out) /* Input in planar, output in interleaved */
{
  for (size_t i = 0; i < rgb_out.size() / 3; i++)
  {
    int y  = y_in[i];
    int cb = cb_in[i] - 128;
    int cr = cr_in[i] - 128;

    int r = y + ((359 * cr) >> 8);
    int g = y - ((88 * cb + 183 * cr) >> 8);
    int b = y + ((453 * cb) >> 8);

    rgb_out[i * 3 + 0] = static_cast<uint8_t>(std::clamp(r, 0, 255));
    rgb_out[i * 3 + 1] = static_cast<uint8_t>(std::clamp(g, 0, 255));
    rgb_out[i * 3 + 2] = static_cast<uint8_t>(std::clamp(b, 0, 255));
  }
}