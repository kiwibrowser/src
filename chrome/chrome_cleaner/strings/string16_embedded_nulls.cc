// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_cleaner/strings/string16_embedded_nulls.h"

namespace chrome_cleaner {

String16EmbeddedNulls::String16EmbeddedNulls() = default;

String16EmbeddedNulls::String16EmbeddedNulls(nullptr_t)
    : String16EmbeddedNulls() {}

String16EmbeddedNulls::String16EmbeddedNulls(const String16EmbeddedNulls& str) =
    default;

String16EmbeddedNulls::String16EmbeddedNulls(const wchar_t* const array,
                                             size_t size) {
  // Empty strings should always be represented as an empty array.
  if (array != nullptr && size > 0)
    data_ = std::vector<wchar_t>(array, array + size);
}

String16EmbeddedNulls::String16EmbeddedNulls(const std::vector<wchar_t>& str)
    : String16EmbeddedNulls(str.data(), str.size()) {}

String16EmbeddedNulls::String16EmbeddedNulls(const base::string16& str)
    : String16EmbeddedNulls(str.data(), str.size()) {}

String16EmbeddedNulls::String16EmbeddedNulls(base::StringPiece16 str)
    : String16EmbeddedNulls(str.data(), str.size()) {}

String16EmbeddedNulls::~String16EmbeddedNulls() = default;

String16EmbeddedNulls& String16EmbeddedNulls::operator=(
    const String16EmbeddedNulls& str) = default;

bool String16EmbeddedNulls::operator==(const String16EmbeddedNulls& str) const {
  return CastAsStringPiece16() == str.CastAsStringPiece16();
}

size_t String16EmbeddedNulls::size() const {
  return data_.size();
}

const base::StringPiece16 String16EmbeddedNulls::CastAsStringPiece16() const {
  return base::StringPiece16(data_.data(), data_.size());
}

const wchar_t* String16EmbeddedNulls::CastAsWCharArray() const {
  return data_.data();
}

const uint16_t* String16EmbeddedNulls::CastAsUInt16Array() const {
  return reinterpret_cast<const uint16_t* const>(data_.data());
}

}  // namespace chrome_cleaner
