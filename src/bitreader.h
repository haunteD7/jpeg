#pragma once

#include <vector>
#include <cstdint>
#include <format>
#include <stdexcept>

class BitReader
{
public:
  static constexpr uint8_t BYTE_LEN = 8;

  void set_data(const std::vector<uint8_t>& data, size_t padding = 0)
  {
    _len = BYTE_LEN * data.size();
    if(padding > _len)
      throw std::invalid_argument("padding can't be greater than length of data");
    
    _data = &data;
    _pos = 0;
  }
  void set_data(std::vector<uint8_t>&& data, size_t padding = 0)
  {
    _len = BYTE_LEN * data.size();
    if(padding > _len)
      throw std::invalid_argument("padding can't be greater than length of data");
    
    _owned_data = std::move(data);
    _data = &_owned_data;
    _pos = 0;
  }
  
  bool peek() const
  {
    if(eof())
      return false;

    size_t byte_pos = _pos / BYTE_LEN;
    size_t bit_pos = _pos % BYTE_LEN;
  
    return ((*_data)[byte_pos] >> (BYTE_LEN - bit_pos - 1)) & 1;
  }
  bool get()
  {
    bool bit = peek();
    forward();
    return bit;
  }
  template <typename T>
  T peek_bits(uint8_t n) const
  {
    if(n == 0 || n > sizeof(T) * 8)
      throw std::invalid_argument(std::format("{}: arguments must n must be 1..{}", __func__, sizeof(T) * 8));
    if(n > remaining_bits())
      throw std::out_of_range(std::format("{}: not enough bits in stream", __func__));
  
    T result = 0;
    size_t bits_read = 0;
    while(bits_read < n)
    {
      size_t byte_offset = (_pos + bits_read) / BYTE_LEN;
      uint8_t bit_offset = (_pos + bits_read) % BYTE_LEN;
      uint8_t bits_left_in_byte = BYTE_LEN - bit_offset;
      uint8_t bits_left_to_read = n - bits_read;
      uint8_t bits_to_read = std::min(bits_left_in_byte, bits_left_to_read);      

      T mask = (T(1) << bits_to_read) - 1;
      T bits = ((*_data)[byte_offset] >> (bits_left_in_byte - bits_to_read)) & mask;

      result = (result << bits_to_read) | bits;

      bits_read += bits_to_read;
    }

    return result;
  }
  template <typename T>
  T get_bits(uint8_t n)
  {
    T result = peek_bits<T>(n);
    _pos += n;
    return result;
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
    if(n > _pos)
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
  size_t get_padding() const
  {
    return _len % BYTE_LEN;
  }
  size_t remaining_bits() const
  {
    return _len - _pos;
  }
private:
  const std::vector<uint8_t>* _data = nullptr;
  std::vector<uint8_t> _owned_data;

  size_t _pos = 0;
  size_t _len = 0;
};
