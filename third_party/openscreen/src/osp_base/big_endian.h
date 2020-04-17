// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_BIG_ENDIAN_H_
#define OSP_BASE_BIG_ENDIAN_H_

#include <stdint.h>

#include <cstring>
#include <type_traits>

namespace openscreen {

////////////////////////////////////////////////////////////////////////////////
// Note: All of the functions here are defined inline, as any half-decent
// compiler will optimize them to a single integer constant or single
// instruction on most architectures.
////////////////////////////////////////////////////////////////////////////////

// Returns true if this code is running on a big-endian architecture.
inline bool IsBigEndianArchitecture() {
  const uint16_t kTestWord = 0x0100;
  uint8_t bytes[sizeof(kTestWord)];
  memcpy(bytes, &kTestWord, sizeof(bytes));
  return !!bytes[0];
}

// Returns the bytes of |x| in reverse order. This is only defined for 16-, 32-,
// and 64-bit unsigned integers.
template <typename Integer>
Integer ByteSwap(Integer x);

template <>
inline uint8_t ByteSwap(uint8_t x) {
  return x;
}

#if defined(__clang__) || defined(__GNUC__)

template <>
inline uint64_t ByteSwap(uint64_t x) {
  return __builtin_bswap64(x);
}
template <>
inline uint32_t ByteSwap(uint32_t x) {
  return __builtin_bswap32(x);
}
template <>
inline uint16_t ByteSwap(uint16_t x) {
  return __builtin_bswap16(x);
}

#elif defined(_MSC_VER)

template <>
inline uint64_t ByteSwap(uint64_t x) {
  return _byteswap_uint64(x);
}
template <>
inline uint32_t ByteSwap(uint32_t x) {
  return _byteswap_ulong(x);
}
template <>
inline uint16_t ByteSwap(uint16_t x) {
  return _byteswap_ushort(x);
}

#else

#include <byteswap.h>

template <>
inline uint64_t ByteSwap(uint64_t x) {
  return bswap_64(x);
}
template <>
inline uint32_t ByteSwap(uint32_t x) {
  return bswap_32(x);
}
template <>
inline uint16_t ByteSwap(uint16_t x) {
  return bswap_16(x);
}

#endif

// Read a POD integer from |src| in big-endian byte order, returning the integer
// in native byte order.
template <typename Integer>
inline Integer ReadBigEndian(const void* src) {
  Integer result;
  memcpy(&result, src, sizeof(result));
  if (!IsBigEndianArchitecture()) {
    result = ByteSwap<typename std::make_unsigned<Integer>::type>(result);
  }
  return result;
}

// Write a POD integer |val| to |dest| in big-endian byte order.
template <typename Integer>
inline void WriteBigEndian(Integer val, void* dest) {
  if (!IsBigEndianArchitecture()) {
    val = ByteSwap<typename std::make_unsigned<Integer>::type>(val);
  }
  memcpy(dest, &val, sizeof(val));
}

class BigEndianReader {
 public:
  BigEndianReader(const uint8_t* buffer, size_t length);
  BigEndianReader(const BigEndianReader&) = delete;
  BigEndianReader& operator=(const BigEndianReader&) = delete;

  template <typename T>
  bool Read(T* out) {
    if (current_ + sizeof(T) > end_) {
      return false;
    }
    *out = ReadBigEndian<T>(current_);
    current_ += sizeof(T);
    return true;
  }

  bool ReadBytes(size_t length, void* out);
  bool Skip(size_t length);

  const uint8_t* begin() const { return begin_; }
  const uint8_t* current() const { return current_; }
  const uint8_t* end() const { return end_; }
  size_t length() const { return end_ - begin_; }
  size_t remaining() const { return end_ - current_; }
  size_t offset() const { return current_ - begin_; }

 private:
  const uint8_t* begin_;
  const uint8_t* current_;
  const uint8_t* end_;
};

class BigEndianWriter {
 public:
  BigEndianWriter(uint8_t* buffer, size_t length);
  BigEndianWriter(const BigEndianWriter&) = delete;
  BigEndianWriter& operator=(const BigEndianWriter&) = delete;

  template <typename T>
  bool Write(T value) {
    if (current_ + sizeof(T) > end_) {
      return false;
    }
    WriteBigEndian<T>(value, current_);
    current_ += sizeof(T);
    return true;
  }

  bool WriteBytes(const void* buffer, size_t length);
  bool Skip(size_t length);

  uint8_t* begin() const { return begin_; }
  uint8_t* current() const { return current_; }
  uint8_t* end() const { return end_; }
  size_t length() const { return end_ - begin_; }
  size_t remaining() const { return end_ - current_; }
  size_t offset() const { return current_ - begin_; }

 private:
  uint8_t* begin_;
  uint8_t* current_;
  uint8_t* end_;
};

}  // namespace openscreen

#endif  // OSP_BASE_BIG_ENDIAN_H_
