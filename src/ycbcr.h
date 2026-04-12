#pragma once

static auto RGB_to_YCbCr(const std::vector<uint8_t> &in) /* Input in interleaved, output in planar */
{
  std::vector<uint8_t> out(in.size());
  size_t pixel_count = in.size() / 3;
  size_t Y_offset = 0;
  size_t Cb_offset = pixel_count;
  size_t Cr_offset = pixel_count * 2;

  auto clamp = [](float v) -> uint8_t
  {
    if (v < 0.0f)
      return 0;
    if (v > 255.0f)
      return 255;
    return static_cast<uint8_t>(v);
  };

  for (size_t i = 0; i < pixel_count; i++)
  {
    uint8_t r = in[i * 3 + 0];
    uint8_t g = in[i * 3 + 1];
    uint8_t b = in[i * 3 + 2];

    float y = 0.299f * r + 0.587f * g + 0.114f * b;
    float cb = -0.1687f * r - 0.3313f * g + 0.5f * b + 128.0f;
    float cr = 0.5f * r - 0.4187f * g - 0.0813f * b + 128.0f;

    out[i + Y_offset] = clamp(y);
    out[i + Cb_offset] = clamp(cb);
    out[i + Cr_offset] = clamp(cr);
  }

  return out;
}

static std::vector<uint8_t> YCbCr_to_RGB(const std::vector<uint8_t> &in) /* Input in planar, output in interleaved */
{
  size_t pixel_count = in.size() / 3;

  std::vector<uint8_t> out(pixel_count * 3);

  size_t Y_offset = 0;
  size_t Cb_offset = pixel_count;
  size_t Cr_offset = pixel_count * 2;

  auto clamp = [](float v) -> uint8_t
  {
    if (v < 0.0f)
      return 0;
    if (v > 255.0f)
      return 255;
    return static_cast<uint8_t>(v);
  };

  for (size_t i = 0; i < pixel_count; i++)
  {
    float Y = in[Y_offset + i];
    float Cb = in[Cb_offset + i];
    float Cr = in[Cr_offset + i];

    float r = Y + 1.402f * (Cr - 128.0f);
    float g = Y - 0.344136f * (Cb - 128.0f) - 0.714136f * (Cr - 128.0f);
    float b = Y + 1.772f * (Cb - 128.0f);

    out[i * 3 + 0] = clamp(r);
    out[i * 3 + 1] = clamp(g);
    out[i * 3 + 2] = clamp(b);
  }

  return out;
}