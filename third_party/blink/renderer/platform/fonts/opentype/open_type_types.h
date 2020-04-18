/*
 * Copyright (C) 2012 Koji Ishii <kojiishi@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_OPENTYPE_OPEN_TYPE_TYPES_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_OPENTYPE_OPEN_TYPE_TYPES_H_

#include "third_party/blink/renderer/platform/wtf/byte_order.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace OpenType {

struct Int16 {
  DISALLOW_NEW();
  Int16(int16_t u) : v(htons(static_cast<uint16_t>(u))) {}
  operator int16_t() const { return static_cast<int16_t>(ntohs(v)); }
  uint16_t v;  // in BigEndian
};

struct UInt16 {
  DISALLOW_NEW();
  UInt16(uint16_t u) : v(htons(u)) {}
  operator uint16_t() const { return ntohs(v); }
  uint16_t v;  // in BigEndian
};

struct Int32 {
  DISALLOW_NEW();
  Int32(int32_t u) : v(htonl(static_cast<uint32_t>(u))) {}
  operator int32_t() const { return static_cast<int32_t>(ntohl(v)); }
  uint32_t v;  // in BigEndian
};

struct UInt32 {
  DISALLOW_NEW();
  UInt32(uint32_t u) : v(htonl(u)) {}
  operator uint32_t() const { return ntohl(v); }
  uint32_t v;  // in BigEndian
};

typedef UInt32 Fixed;
typedef UInt16 Offset;
typedef UInt16 GlyphID;

template <typename T>
static const T* ValidateTable(const Vector<char>& buffer, size_t count = 1) {
  if (buffer.size() < sizeof(T) * count)
    return nullptr;
  return reinterpret_cast<const T*>(buffer.data());
}

struct TableBase {
  DISALLOW_NEW();

 protected:
  static bool IsValidEnd(const Vector<char>& buffer, const void* position) {
    if (position < buffer.data())
      return false;
    size_t offset = reinterpret_cast<const char*>(position) - buffer.data();
    return offset <= buffer.size();  // "<=" because end is included as valid
  }

  template <typename T>
  static const T* ValidatePtr(const Vector<char>& buffer,
                              const void* position) {
    const T* casted = reinterpret_cast<const T*>(position);
    if (!IsValidEnd(buffer, &casted[1]))
      return nullptr;
    return casted;
  }

  template <typename T>
  const T* ValidateOffset(const Vector<char>& buffer, uint16_t offset) const {
    return ValidatePtr<T>(buffer,
                          reinterpret_cast<const int8_t*>(this) + offset);
  }
};

}  // namespace OpenType
}  // namespace blink
#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_OPENTYPE_OPEN_TYPE_TYPES_H_
