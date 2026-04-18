#pragma once

#include <array>
#include <vector>
#include <cstdint>

struct HuffCode
{
  uint16_t code;
  uint8_t size;
};

static void build_canonical_huffman(
    const std::array<int,16>& bits,
    const std::vector<int>& symbols,
    std::vector<HuffCode>& out_table)
{
  // очистка
  for (auto& h : out_table)
    h = {0,0};

  int total = 0;
  for (int i = 0; i < 16; i++)
    total += bits[i];

  if (total != (int)symbols.size())
    throw std::runtime_error("Huffman bits/vals mismatch");

  uint32_t code = 0;
  int k = 0;

  for (int len = 1; len <= 16; len++)
  {
    int count = bits[len - 1];

    for (int i = 0; i < count; i++)
    {
      if (k >= (int)symbols.size())
        throw std::runtime_error("Huffman symbols overflow");

      int sym = symbols[k++];

      if (sym < 0 || sym >= (int)out_table.size())
        throw std::runtime_error("Huffman symbol out of range");

      out_table[sym].code = code;
      out_table[sym].size = len;

      code++;
    }

    code <<= 1;
  }
}

struct BitWriter
{
  std::vector<uint8_t>& out;
  uint8_t buf = 0;
  int bit_pos = 0;

  void write(uint32_t bits, int size)
  {
    for (int i = size - 1; i >= 0; i--)
    {
      buf = (buf << 1) | ((bits >> i) & 1);
      bit_pos++;

      if (bit_pos == 8)
      {
        out.push_back(buf);
        buf = 0;
        bit_pos = 0;
      }
    }
  }

  void flush()
  {
    if (bit_pos)
    {
      buf <<= (8 - bit_pos);
      out.push_back(buf);
    }
  }
};

static inline void huff_write(
    BitWriter& bw,
    const HuffCode& hc,
    uint16_t bits = 0,
    uint8_t bitlen = 0)
{
  bw.write(hc.code, hc.size);

  if (bitlen)
    bw.write(bits, bitlen);
} 

struct HuffDecode
{
  int16_t symbol;
};
struct HuffTable
{
  std::vector<int16_t> symbols;
  std::vector<int> min_code;
  std::vector<int> max_code;
  std::vector<int> val_ptr;
};
struct DecodeTable
{
  int min_code[17];
  int max_code[17];
  int val_ptr[17];
  int symbols[256];
};
static void build_decode(
    const std::array<int,16>& bits,
    const std::vector<int>& huffval,
    DecodeTable& t)
{
  int code = 0;
  int k = 0;

  for (int i = 1; i <= 16; i++)
  {
    int cnt = bits[i - 1];

    if (cnt == 0)
    {
      t.min_code[i] = -1;
      t.max_code[i] = -1;
      continue;
    }

    t.val_ptr[i] = k;
    t.min_code[i] = code;
    t.max_code[i] = code + cnt - 1;

    for (int j = 0; j < cnt; j++)
    {
      t.symbols[k] = huffval[k];
      k++;
    }

    code = (code + cnt) << 1;
  }
}
struct BitReader
{
  const uint8_t* data;
  size_t size;
  size_t byte_pos = 0;
  int bit_pos = 0;

  uint32_t read_bit()
  {
    uint32_t v = (data[byte_pos] >> (7 - bit_pos)) & 1;

    bit_pos++;
    if (bit_pos == 8)
    {
      bit_pos = 0;
      byte_pos++;
    }

    return v;
  }

  uint32_t read_bits(int n)
  {
    uint32_t v = 0;
    for (int i = 0; i < n; i++)
      v = (v << 1) | read_bit();
    return v;
  }
};
static int huff_decode(BitReader& br, const DecodeTable& t)
{
  int code = 0;

  for (int len = 1; len <= 16; len++)
  {
    code = (code << 1) | br.read_bit();

    if (t.min_code[len] != -1 &&
        code >= t.min_code[len] &&
        code <= t.max_code[len])
    {
      int idx = t.val_ptr[len] + (code - t.min_code[len]);
      return t.symbols[idx];
    }
  }

  return -1; // error
}