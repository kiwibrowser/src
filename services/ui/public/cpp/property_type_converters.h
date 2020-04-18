// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_PROPERTY_TYPE_CONVERTERS_H_
#define SERVICES_UI_PUBLIC_CPP_PROPERTY_TYPE_CONVERTERS_H_

#include <stdint.h>
#include <vector>

#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/type_converter.h"

class SkBitmap;

namespace gfx {
class Rect;
class Size;
}

namespace mojo {

// TODO(beng): these methods serialize types used for standard properties
//             to vectors of bytes used by Window::SetSharedProperty().
//             replace this with serialization code generated @ bindings.

template <>
struct TypeConverter<std::vector<uint8_t>, gfx::Rect> {
  static std::vector<uint8_t> Convert(const gfx::Rect& input);
};
template <>
struct TypeConverter<gfx::Rect, std::vector<uint8_t>> {
  static gfx::Rect Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, gfx::Size> {
  static std::vector<uint8_t> Convert(const gfx::Size& input);
};
template <>
struct TypeConverter<gfx::Size, std::vector<uint8_t>> {
  static gfx::Size Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, int32_t> {
  static std::vector<uint8_t> Convert(const int32_t& input);
};
template <>
struct TypeConverter<int32_t, std::vector<uint8_t>> {
  static int32_t Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, int64_t> {
  static std::vector<uint8_t> Convert(const int64_t& input);
};
template <>
struct TypeConverter<int64_t, std::vector<uint8_t>> {
  static int64_t Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, base::string16> {
  static std::vector<uint8_t> Convert(const base::string16& input);
};
template <>
struct TypeConverter<base::string16, std::vector<uint8_t>> {
  static base::string16 Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, std::string> {
  static std::vector<uint8_t> Convert(const std::string& input);
};
template <>
struct TypeConverter<std::string, std::vector<uint8_t>> {
  static std::string Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, SkBitmap> {
  static std::vector<uint8_t> Convert(const SkBitmap& input);
};
template <>
struct TypeConverter<SkBitmap, std::vector<uint8_t>> {
  static SkBitmap Convert(const std::vector<uint8_t>& input);
};

template <>
struct TypeConverter<std::vector<uint8_t>, bool> {
  static std::vector<uint8_t> Convert(bool input);
};
template <>
struct TypeConverter<bool, std::vector<uint8_t>> {
  static bool Convert(const std::vector<uint8_t>& input);
};

}  // namespace mojo

#endif  // SERVICES_UI_PUBLIC_CPP_PROPERTY_TYPE_CONVERTERS_H_
