// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/property_type_converters.h"

#include <stdint.h>

#include "base/strings/utf_string_conversions.h"
#include "skia/public/interfaces/bitmap.mojom.h"
#include "skia/public/interfaces/bitmap_skbitmap_struct_traits.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace mojo {

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, gfx::Rect>::Convert(
    const gfx::Rect& input) {
  std::vector<uint8_t> vec(16);
  vec[0] = (input.x() >> 24) & 0xFF;
  vec[1] = (input.x() >> 16) & 0xFF;
  vec[2] = (input.x() >> 8) & 0xFF;
  vec[3] = input.x() & 0xFF;
  vec[4] = (input.y() >> 24) & 0xFF;
  vec[5] = (input.y() >> 16) & 0xFF;
  vec[6] = (input.y() >> 8) & 0xFF;
  vec[7] = input.y() & 0xFF;
  vec[8] = (input.width() >> 24) & 0xFF;
  vec[9] = (input.width() >> 16) & 0xFF;
  vec[10] = (input.width() >> 8) & 0xFF;
  vec[11] = input.width() & 0xFF;
  vec[12] = (input.height() >> 24) & 0xFF;
  vec[13] = (input.height() >> 16) & 0xFF;
  vec[14] = (input.height() >> 8) & 0xFF;
  vec[15] = input.height() & 0xFF;
  return vec;
}

// static
gfx::Rect TypeConverter<gfx::Rect, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  if (input.size() != 16)
    return gfx::Rect();

  return gfx::Rect(
      input[0] << 24 | input[1] << 16 | input[2] << 8 | input[3],
      input[4] << 24 | input[5] << 16 | input[6] << 8 | input[7],
      input[8] << 24 | input[9] << 16 | input[10] << 8 | input[11],
      input[12] << 24 | input[13] << 16 | input[14] << 8 | input[15]);
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, gfx::Size>::Convert(
    const gfx::Size& input) {
  std::vector<uint8_t> vec(8);
  vec[0] = (input.width() >> 24) & 0xFF;
  vec[1] = (input.width() >> 16) & 0xFF;
  vec[2] = (input.width() >> 8) & 0xFF;
  vec[3] = input.width() & 0xFF;
  vec[4] = (input.height() >> 24) & 0xFF;
  vec[5] = (input.height() >> 16) & 0xFF;
  vec[6] = (input.height() >> 8) & 0xFF;
  vec[7] = input.height() & 0xFF;
  return vec;
}

// static
gfx::Size TypeConverter<gfx::Size, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  if (input.size() != 8)
    return gfx::Size();

  return gfx::Size(input[0] << 24 | input[1] << 16 | input[2] << 8 | input[3],
                   input[4] << 24 | input[5] << 16 | input[6] << 8 | input[7]);
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, int32_t>::Convert(
    const int32_t& input) {
  std::vector<uint8_t> vec(4);
  vec[0] = (input >> 24) & 0xFF;
  vec[1] = (input >> 16) & 0xFF;
  vec[2] = (input >> 8) & 0xFF;
  vec[3] = input & 0xFF;
  return vec;
}

// static
int32_t TypeConverter<int32_t, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  if (input.size() != 4)
    return 0;

  return input[0] << 24 | input[1] << 16 | input[2] << 8 | input[3];
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, int64_t>::Convert(
    const int64_t& input) {
  std::vector<uint8_t> vec(8);
  vec[0] = (input >> 56) & 0xFF;
  vec[1] = (input >> 48) & 0xFF;
  vec[2] = (input >> 40) & 0xFF;
  vec[3] = (input >> 32) & 0xFF;
  vec[4] = (input >> 24) & 0xFF;
  vec[5] = (input >> 16) & 0xFF;
  vec[6] = (input >> 8) & 0xFF;
  vec[7] = input & 0xFF;
  return vec;
}

// static
int64_t TypeConverter<int64_t, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  if (input.size() != 8)
    return 0;

  return static_cast<int64_t>(input[0]) << 56 |
         static_cast<int64_t>(input[1]) << 48 |
         static_cast<int64_t>(input[2]) << 40 |
         static_cast<int64_t>(input[3]) << 32 |
         static_cast<int64_t>(input[4]) << 24 |
         static_cast<int64_t>(input[5]) << 16 |
         static_cast<int64_t>(input[6]) << 8 | static_cast<int64_t>(input[7]);
}

// static
std::vector<uint8_t>
TypeConverter<std::vector<uint8_t>, base::string16>::Convert(
    const base::string16& input) {
  return ConvertTo<std::vector<uint8_t>>(base::UTF16ToUTF8(input));
}

// static
base::string16 TypeConverter<base::string16, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  return base::UTF8ToUTF16(ConvertTo<std::string>(input));
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, std::string>::Convert(
    const std::string& input) {
  return std::vector<uint8_t>(input.begin(), input.end());
}

// static
std::string TypeConverter<std::string, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  return std::string(input.begin(), input.end());
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, SkBitmap>::Convert(
    const SkBitmap& input) {
  return skia::mojom::InlineBitmap::Serialize(&input);
}

// static
SkBitmap TypeConverter<SkBitmap, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  SkBitmap output;
  return skia::mojom::InlineBitmap::Deserialize(input, &output) ? output
                                                                : SkBitmap();
}

// static
std::vector<uint8_t> TypeConverter<std::vector<uint8_t>, bool>::Convert(
    bool input) {
  std::vector<uint8_t> vec(1);
  vec[0] = input ? 1 : 0;
  return vec;
}

// static
bool TypeConverter<bool, std::vector<uint8_t>>::Convert(
    const std::vector<uint8_t>& input) {
  // Empty vectors are interpreted as false.
  return !input.empty() && (input[0] == 1);
}

}  // namespace mojo
