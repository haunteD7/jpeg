#pragma once

#include "zigzag.h"
#include "huffman.h"

#include <span>
#include <vector>

static void dc_dpcm_encode(std::span<int16_t> dc)
{
  int16_t prev = 0;

  for (size_t i = 0; i < dc.size(); i++)
  {
    int16_t curr = dc[i];
    dc[i] = curr - prev;
    prev = curr;
  }
}
static void dc_dpcm_decode(std::span<int16_t> dc)
{
  int16_t prev = 0;

  for (size_t i = 0; i < dc.size(); i++)
  {
    dc[i] = dc[i] + prev;
    prev = dc[i];
  }
}
static inline int category(int16_t v)
{
  if (v == 0)
    return 0;

  int a = std::abs(v);
  int n = 0;

  while (a)
  {
    a >>= 1;
    n++;
  }

  return n;
}

static inline uint16_t encode_value(int16_t v, int cat)
{
  if (cat == 0)
    return 0;

  if (v >= 0)
    return (uint16_t)v;

  // negative
  return (uint16_t)(v + ((1 << cat) - 1));
}
static inline int16_t decode_value(uint16_t bits, int cat)
{
  if (cat == 0)
    return 0;

  if (bits >= (1u << (cat - 1)))
    return bits;

  // negative
  return bits - ((1 << cat) - 1);
}

struct AC_Symbol
{
  uint8_t symbol;  // (run << 4) | category
  uint16_t bits;   // encoded value bits
  uint8_t bit_len; // category
};
static size_t rle_encode(
    std::span<const int16_t> in, // N * 63
    std::span<AC_Symbol> out)
{
  size_t out_pos = 0;
  size_t blocks = in.size() / 63;

  for (size_t b = 0; b < blocks; b++)
  {
    int zero_count = 0;

    for (int i = 0; i < 63; i++)
    {
      int16_t v = in[b * 63 + i];

      if (v == 0)
      {
        zero_count++;

        if (zero_count == 16)
        {
          // ZRL
          out[out_pos++] = {
              0xF0, // (15,0)
              0,
              0};
          zero_count = 0;
        }
      }
      else
      {
        int cat = category(v);

        uint8_t run = zero_count;
        zero_count = 0;

        uint8_t symbol = (run << 4) | cat;
        uint16_t bits = encode_value(v, cat);

        out[out_pos++] = {
            symbol,
            bits,
            (uint8_t)cat};
      }
    }

    // EOB
    if(zero_count)
      out[out_pos++] = {0x00, 0, 0};
  }

  return out_pos;
}
static void rle_decode(
    std::span<const AC_Symbol> in,
    size_t &in_pos,
    std::span<int16_t> out)
{
  size_t blocks = out.size() / 63;

  for (size_t b = 0; b < blocks; b++)
  {
    int pos = 0;

    while (pos < 63)
    {
      AC_Symbol s = in[in_pos++];

      uint8_t run = s.symbol >> 4;
      uint8_t cat = s.symbol & 0x0F;

      if (s.symbol == 0x00) // EOB
      {
        while (pos < 63)
          out[b * 63 + pos++] = 0;
        break;
      }

      if (s.symbol == 0xF0) // ZRL
      {
        for (int i = 0; i < 16; i++)
          out[b * 63 + pos++] = 0;
        continue;
      }

      for (int i = 0; i < run; i++)
        out[b * 63 + pos++] = 0;

      int16_t v = decode_value(s.bits, cat);
      out[b * 63 + pos++] = v;
    }
  }
}
static void encode_dc(
    std::span<const int16_t> dc_diff,
    const std::vector<HuffCode>& table,
    BitWriter& bw)
{
  for (int16_t v : dc_diff)
  {
    int cat = category(v);

    const HuffCode& hc = table[cat];

    bw.write(hc.code, hc.size);

    if (cat > 0)
    {
      uint16_t bits = encode_value(v, cat);
      bw.write(bits, cat);
    }
  }
}
static void decode_dc(
    std::span<int16_t> dc,
    BitReader& br,
    const DecodeTable& table)
{
  for (size_t i = 0; i < dc.size(); i++)
  {
    int cat = huff_decode(br, table);

    int16_t v = 0;

    if (cat > 0)
    {
      uint16_t bits = br.read_bits(cat);
      v = decode_value(bits, cat);
    }

    dc[i] = v;
  }
}
static void encode_ac(
    std::span<const AC_Symbol> ac,
    const std::vector<HuffCode>& table,
    BitWriter& bw)
{
  for (auto s : ac)
  {
    uint8_t run = s.symbol >> 4;
    uint8_t cat = s.symbol & 0x0F;

    uint8_t sym = s.symbol;

    const HuffCode& hc = table[sym];

    bw.write(hc.code, hc.size);

    if (cat > 0)
      bw.write(s.bits, cat);
  }
}
static void decode_ac(
    BitReader& br,
    const DecodeTable& t,
    std::span<int16_t> out)
{
  int pos = 0;

  while (pos < 63)
  {
    int sym = huff_decode(br, t);

    int run = sym >> 4;
    int cat = sym & 0x0F;

    if (sym == 0x00) // EOB
    {
      while (pos < 63)
        out[pos++] = 0;
      break;
    }

    if (sym == 0xF0) // ZRL
    {
      for (int i = 0; i < 16; i++)
        out[pos++] = 0;
      continue;
    }

    for (int i = 0; i < run; i++)
      out[pos++] = 0;

    uint32_t bits = br.read_bits(cat);
    out[pos++] = decode_value(bits, cat);
  }
}