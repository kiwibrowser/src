// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_values.h"

#include <new>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"

namespace cbor {

CBORValue::CBORValue() noexcept : type_(Type::NONE) {}

CBORValue::CBORValue(CBORValue&& that) noexcept {
  InternalMoveConstructFrom(std::move(that));
}

CBORValue::CBORValue(Type type) : type_(type) {
  // Initialize with the default value.
  switch (type_) {
    case Type::UNSIGNED:
    case Type::NEGATIVE:
      integer_value_ = 0;
      return;
    case Type::BYTE_STRING:
      new (&bytestring_value_) BinaryValue();
      return;
    case Type::STRING:
      new (&string_value_) std::string();
      return;
    case Type::ARRAY:
      new (&array_value_) ArrayValue();
      return;
    case Type::MAP:
      new (&map_value_) MapValue();
      return;
    case Type::SIMPLE_VALUE:
      simple_value_ = CBORValue::SimpleValue::UNDEFINED;
      return;
    case Type::NONE:
      return;
  }
  NOTREACHED();
}

CBORValue::CBORValue(SimpleValue in_simple)
    : type_(Type::SIMPLE_VALUE), simple_value_(in_simple) {
  CHECK(static_cast<int>(in_simple) >= 20 && static_cast<int>(in_simple) <= 23);
}

CBORValue::CBORValue(bool boolean_value) : type_(Type::SIMPLE_VALUE) {
  simple_value_ = boolean_value ? CBORValue::SimpleValue::TRUE_VALUE
                                : CBORValue::SimpleValue::FALSE_VALUE;
}

CBORValue::CBORValue(int integer_value)
    : CBORValue(base::checked_cast<int64_t>(integer_value)) {}

CBORValue::CBORValue(int64_t integer_value) : integer_value_(integer_value) {
  type_ = integer_value >= 0 ? Type::UNSIGNED : Type::NEGATIVE;
}

CBORValue::CBORValue(const BinaryValue& in_bytes)
    : type_(Type::BYTE_STRING), bytestring_value_(in_bytes) {}

CBORValue::CBORValue(BinaryValue&& in_bytes) noexcept
    : type_(Type::BYTE_STRING), bytestring_value_(std::move(in_bytes)) {}

CBORValue::CBORValue(const char* in_string, Type type)
    : CBORValue(base::StringPiece(in_string), type) {}

CBORValue::CBORValue(std::string&& in_string, Type type) noexcept
    : type_(type) {
  switch (type_) {
    case Type::STRING:
      new (&string_value_) std::string();
      string_value_ = std::move(in_string);
      DCHECK(base::IsStringUTF8(string_value_));
      break;
    case Type::BYTE_STRING:
      new (&bytestring_value_) BinaryValue();
      bytestring_value_ = BinaryValue(in_string.begin(), in_string.end());
      break;
    default:
      NOTREACHED();
  }
}

CBORValue::CBORValue(base::StringPiece in_string, Type type) : type_(type) {
  switch (type_) {
    case Type::STRING:
      new (&string_value_) std::string();
      string_value_ = in_string.as_string();
      DCHECK(base::IsStringUTF8(string_value_));
      break;
    case Type::BYTE_STRING:
      new (&bytestring_value_) BinaryValue();
      bytestring_value_ = BinaryValue(in_string.begin(), in_string.end());
      break;
    default:
      NOTREACHED();
  }
}

CBORValue::CBORValue(const ArrayValue& in_array)
    : type_(Type::ARRAY), array_value_() {
  array_value_.reserve(in_array.size());
  for (const auto& val : in_array)
    array_value_.emplace_back(val.Clone());
}

CBORValue::CBORValue(ArrayValue&& in_array) noexcept
    : type_(Type::ARRAY), array_value_(std::move(in_array)) {}

CBORValue::CBORValue(const MapValue& in_map) : type_(Type::MAP), map_value_() {
  map_value_.reserve(in_map.size());
  for (const auto& it : in_map)
    map_value_.emplace_hint(map_value_.end(), it.first.Clone(),
                            it.second.Clone());
}

CBORValue::CBORValue(MapValue&& in_map) noexcept
    : type_(Type::MAP), map_value_(std::move(in_map)) {}

CBORValue& CBORValue::operator=(CBORValue&& that) noexcept {
  InternalCleanup();
  InternalMoveConstructFrom(std::move(that));

  return *this;
}

CBORValue::~CBORValue() {
  InternalCleanup();
}

CBORValue CBORValue::Clone() const {
  switch (type_) {
    case Type::NONE:
      return CBORValue();
    case Type::UNSIGNED:
    case Type::NEGATIVE:
      return CBORValue(integer_value_);
    case Type::BYTE_STRING:
      return CBORValue(bytestring_value_);
    case Type::STRING:
      return CBORValue(string_value_);
    case Type::ARRAY:
      return CBORValue(array_value_);
    case Type::MAP:
      return CBORValue(map_value_);
    case Type::SIMPLE_VALUE:
      return CBORValue(simple_value_);
  }

  NOTREACHED();
  return CBORValue();
}

CBORValue::SimpleValue CBORValue::GetSimpleValue() const {
  CHECK(is_simple());
  return simple_value_;
}

bool CBORValue::GetBool() const {
  CHECK(is_bool());
  return simple_value_ == SimpleValue::TRUE_VALUE;
}

const int64_t& CBORValue::GetInteger() const {
  CHECK(is_integer());
  return integer_value_;
}

const int64_t& CBORValue::GetUnsigned() const {
  CHECK(is_unsigned());
  CHECK_GE(integer_value_, 0);
  return integer_value_;
}

const int64_t& CBORValue::GetNegative() const {
  CHECK(is_negative());
  CHECK_LT(integer_value_, 0);
  return integer_value_;
}

const std::string& CBORValue::GetString() const {
  CHECK(is_string());
  return string_value_;
}

const CBORValue::BinaryValue& CBORValue::GetBytestring() const {
  CHECK(is_bytestring());
  return bytestring_value_;
}

base::StringPiece CBORValue::GetBytestringAsString() const {
  CHECK(is_bytestring());
  const auto& bytestring_value = GetBytestring();
  return base::StringPiece(
      reinterpret_cast<const char*>(bytestring_value.data()),
      bytestring_value.size());
}

const CBORValue::ArrayValue& CBORValue::GetArray() const {
  CHECK(is_array());
  return array_value_;
}

const CBORValue::MapValue& CBORValue::GetMap() const {
  CHECK(is_map());
  return map_value_;
}

void CBORValue::InternalMoveConstructFrom(CBORValue&& that) {
  type_ = that.type_;

  switch (type_) {
    case Type::UNSIGNED:
    case Type::NEGATIVE:
      integer_value_ = that.integer_value_;
      return;
    case Type::BYTE_STRING:
      new (&bytestring_value_) BinaryValue(std::move(that.bytestring_value_));
      return;
    case Type::STRING:
      new (&string_value_) std::string(std::move(that.string_value_));
      return;
    case Type::ARRAY:
      new (&array_value_) ArrayValue(std::move(that.array_value_));
      return;
    case Type::MAP:
      new (&map_value_) MapValue(std::move(that.map_value_));
      return;
    case Type::SIMPLE_VALUE:
      simple_value_ = that.simple_value_;
      return;
    case Type::NONE:
      return;
  }
  NOTREACHED();
}

void CBORValue::InternalCleanup() {
  switch (type_) {
    case Type::BYTE_STRING:
      bytestring_value_.~BinaryValue();
      break;
    case Type::STRING:
      string_value_.~basic_string();
      break;
    case Type::ARRAY:
      array_value_.~ArrayValue();
      break;
    case Type::MAP:
      map_value_.~MapValue();
      break;
    case Type::NONE:
    case Type::UNSIGNED:
    case Type::NEGATIVE:
    case Type::SIMPLE_VALUE:
      break;
  }
  type_ = Type::NONE;
}

}  // namespace cbor
