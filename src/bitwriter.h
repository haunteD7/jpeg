#pragma once

#include <vector>
#include <cstdint>
#include <format>
#include <stdexcept>

class BitWriter
{
public:
  static constexpr uint8_t BYTE_LEN = 8;

  const std::vector<uint8_t>& get_data() const
  {
    return _data;
  }
  std::vector<uint8_t> take_data()
  {
    _pos = 0;
    _len = 0;
    return std::move(_data);
  }
  
  void write(bool bit)
  {
    if(eof())
      _len++;

    size_t byte_pos = _pos / BYTE_LEN;
    size_t bit_pos = _pos % BYTE_LEN;
  
    if(byte_pos >= _data.size())
      _data.push_back(0);

    if(bit)
      _data[byte_pos] |= (1 << (BYTE_LEN - 1 - bit_pos));
    else
      _data[byte_pos] &= ~(1 << (BYTE_LEN - 1 - bit_pos));
  }
  void put(bool bit)
  {
    write(bit);
    _pos++;
    
  }
  template <typename T>
  void write_bits(uint8_t n, T bits)  
  {
    if(n == 0 || n > sizeof(T) * 8)
      throw std::invalid_argument(std::format("{}: arguments must n must be 1..{}", __func__, sizeof(T) * 8));

    size_t bits_left = remaining_bits();
    if(bits_left < n)
      _len += n - bits_left;

    size_t bits_written = 0;

    while(bits_written < n)
    {
      size_t byte_offset = (_pos + bits_written) / BYTE_LEN;
      uint8_t bit_offset = (_pos + bits_written) % BYTE_LEN;

      if(byte_offset >= _data.size())
        _data.push_back(0); 

      uint8_t bits_left_in_byte = BYTE_LEN - bit_offset;
      uint8_t bits_left_to_write = n - bits_written;
      uint8_t bits_now = std::min(bits_left_in_byte, bits_left_to_write);

      /* Create mask for current bits */
      uint8_t mask = ((1u << bits_now) - 1);

      /* Take biggest bits */
      uint8_t shift = bits_left_to_write - bits_now;
      uint8_t value_to_write = (bits >> shift) & mask;

      /* Write in current byte */
      _data[byte_offset] &= ~(mask << (bits_left_in_byte - bits_now)); /* Clear bits */
      _data[byte_offset] |= value_to_write << (bits_left_in_byte - bits_now);

      bits_written += bits_now;
    } 
  }
  template <typename T>
  void put_bits(uint8_t n, T bits)
  {
    write_bits<T>(n, bits);
    _pos += n;
  }

  bool forward()
  {
    if(eof())
      return false;

    _pos++;

    return true;
  }
  bool back()
  {
    if(is_start())
      return false;
    
    _pos--;

    return true;
  }
  void move_forward(size_t n)
  {
    size_t new_pos = _pos + n; 
    if(new_pos > _len)
    {
      _pos = _len;
      return;
    }
    
    _pos = new_pos; 
  }
  void move_back(size_t n)
  {
    if(n > _len)
    {
      _pos = 0;
      return;
    }
    
    _pos = _pos - n; 
  }
  size_t get_pos() const
  {
    return _pos;
  }
  void set_pos(size_t pos)
  {
    if(pos > _len)
    {
      _pos = _len;
      return;
    }
    _pos = pos;
  }

  bool is_start() const
  {
    return _pos == 0; 
  }
  bool eof() const
  {
    return _pos >= _len;
  }
  size_t get_len() const /* In bits */
  {
    return _len;
  }
  uint8_t get_padding() const
  {
    return _len % BYTE_LEN;
  }
  size_t remaining_bits() const
  {
    return _len - _pos;
  }
  void reserve(size_t n)
  {
    _data.reserve(n);
  }
private:
  std::vector<uint8_t> _data;

  size_t _pos = 0;
  size_t _len = 0;
};
