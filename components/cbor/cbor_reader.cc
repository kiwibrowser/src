// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_reader.h"

#include <math.h>

#include <utility>

#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "components/cbor/cbor_binary.h"

namespace cbor {

namespace {

CBORValue::Type GetMajorType(uint8_t initial_data_byte) {
  return static_cast<CBORValue::Type>(
      (initial_data_byte & constants::kMajorTypeMask) >>
      constants::kMajorTypeBitShift);
}

uint8_t GetAdditionalInfo(uint8_t initial_data_byte) {
  return initial_data_byte & constants::kAdditionalInformationMask;
}

// Error messages that correspond to each of the error codes.
const char kNoError[] = "Successfully deserialized to a CBOR value.";
const char kUnsupportedMajorType[] = "Unsupported major type.";
const char kUnknownAdditionalInfo[] =
    "Unknown additional info format in the first byte.";
const char kIncompleteCBORData[] =
    "Prematurely terminated CBOR data byte array.";
const char kIncorrectMapKeyType[] =
    "Specified map key type is not supported by the current implementation.";
const char kTooMuchNesting[] = "Too much nesting.";
const char kInvalidUTF8[] = "String encoding other than utf8 are not allowed.";
const char kExtraneousData[] = "Trailing data bytes are not allowed.";
const char kMapKeyOutOfOrder[] =
    "Map keys must be strictly monotonically increasing based on byte length "
    "and then by byte-wise lexical order.";
const char kNonMinimalCBOREncoding[] =
    "Unsigned integers must be encoded with minimum number of bytes.";
const char kUnsupportedSimpleValue[] =
    "Unsupported or unassigned simple value.";
const char kUnsupportedFloatingPointValue[] =
    "Floating point numbers are not supported.";
const char kOutOfRangeIntegerValue[] =
    "Integer values must be between INT64_MIN and INT64_MAX.";
const char kUnknownError[] = "An unknown error occured.";

}  // namespace

CBORReader::CBORReader(base::span<const uint8_t>::const_iterator begin,
                       const base::span<const uint8_t>::const_iterator end)
    : begin_(begin),
      it_(begin),
      end_(end),
      error_code_(DecoderError::CBOR_NO_ERROR) {}
CBORReader::~CBORReader() {}

// static
base::Optional<CBORValue> CBORReader::Read(base::span<uint8_t const> data,
                                           DecoderError* error_code_out,
                                           int max_nesting_level) {
  size_t num_bytes_consumed;
  auto decoded_cbor =
      Read(data, &num_bytes_consumed, error_code_out, max_nesting_level);

  if (decoded_cbor && num_bytes_consumed != data.size()) {
    if (error_code_out)
      *error_code_out = DecoderError::EXTRANEOUS_DATA;
    return base::nullopt;
  }

  return decoded_cbor;
}

// static
base::Optional<CBORValue> CBORReader::Read(base::span<uint8_t const> data,
                                           size_t* num_bytes_consumed,
                                           DecoderError* error_code_out,
                                           int max_nesting_level) {
  CBORReader reader(data.cbegin(), data.cend());
  base::Optional<CBORValue> decoded_cbor =
      reader.DecodeCompleteDataItem(max_nesting_level);

  auto error_code = reader.GetErrorCode();
  const bool failed = !decoded_cbor.has_value();

  // An error code must be set iff parsing failed.
  DCHECK_EQ(failed, error_code != DecoderError::CBOR_NO_ERROR);

  if (error_code_out)
    *error_code_out = error_code;

  *num_bytes_consumed = failed ? 0 : reader.num_bytes_consumed();
  return decoded_cbor;
}

base::Optional<CBORValue> CBORReader::DecodeCompleteDataItem(
    int max_nesting_level) {
  if (max_nesting_level < 0 || max_nesting_level > kCBORMaxDepth) {
    error_code_ = DecoderError::TOO_MUCH_NESTING;
    return base::nullopt;
  }

  base::Optional<DataItemHeader> header = DecodeDataItemHeader();
  if (!header.has_value())
    return base::nullopt;

  switch (header->type) {
    case CBORValue::Type::UNSIGNED:
      return DecodeValueToUnsigned(header->value);
    case CBORValue::Type::NEGATIVE:
      return DecodeValueToNegative(header->value);
    case CBORValue::Type::BYTE_STRING:
      return ReadByteStringContent(*header);
    case CBORValue::Type::STRING:
      return ReadStringContent(*header);
    case CBORValue::Type::ARRAY:
      return ReadArrayContent(*header, max_nesting_level);
    case CBORValue::Type::MAP:
      return ReadMapContent(*header, max_nesting_level);
    case CBORValue::Type::SIMPLE_VALUE:
      return DecodeToSimpleValue(*header);
    case CBORValue::Type::NONE:
      break;
  }

  error_code_ = DecoderError::UNSUPPORTED_MAJOR_TYPE;
  return base::nullopt;
}

base::Optional<CBORReader::DataItemHeader> CBORReader::DecodeDataItemHeader() {
  if (!CanConsume(1)) {
    return base::nullopt;
  }

  const uint8_t initial_byte = *it_++;
  const auto major_type = GetMajorType(initial_byte);
  const uint8_t additional_info = GetAdditionalInfo(initial_byte);

  uint64_t value;
  if (!ReadVariadicLengthInteger(additional_info, &value))
    return base::nullopt;

  return DataItemHeader{major_type, additional_info, value};
}

bool CBORReader::ReadVariadicLengthInteger(uint8_t additional_info,
                                           uint64_t* value) {
  uint8_t additional_bytes = 0;
  if (additional_info < 24) {
    *value = additional_info;
    return true;
  } else if (additional_info == 24) {
    additional_bytes = 1;
  } else if (additional_info == 25) {
    additional_bytes = 2;
  } else if (additional_info == 26) {
    additional_bytes = 4;
  } else if (additional_info == 27) {
    additional_bytes = 8;
  } else {
    error_code_ = DecoderError::UNKNOWN_ADDITIONAL_INFO;
    return false;
  }

  if (!CanConsume(additional_bytes)) {
    return false;
  }

  uint64_t int_data = 0;
  for (uint8_t i = 0; i < additional_bytes; ++i) {
    int_data <<= 8;
    int_data |= *it_++;
  }

  *value = int_data;
  return CheckMinimalEncoding(additional_bytes, int_data);
}

base::Optional<CBORValue> CBORReader::DecodeValueToNegative(uint64_t value) {
  auto negative_value = -base::CheckedNumeric<int64_t>(value) - 1;
  if (!negative_value.IsValid()) {
    error_code_ = DecoderError::OUT_OF_RANGE_INTEGER_VALUE;
    return base::nullopt;
  }
  return CBORValue(negative_value.ValueOrDie());
}

base::Optional<CBORValue> CBORReader::DecodeValueToUnsigned(uint64_t value) {
  auto unsigned_value = base::CheckedNumeric<int64_t>(value);
  if (!unsigned_value.IsValid()) {
    error_code_ = DecoderError::OUT_OF_RANGE_INTEGER_VALUE;
    return base::nullopt;
  }
  return CBORValue(unsigned_value.ValueOrDie());
}

base::Optional<CBORValue> CBORReader::DecodeToSimpleValue(
    const DataItemHeader& header) {
  // ReadVariadicLengthInteger provides this bound.
  CHECK_LE(header.additional_info, 27);
  // Floating point numbers are not supported.
  if (header.additional_info > 24) {
    error_code_ = DecoderError::UNSUPPORTED_FLOATING_POINT_VALUE;
    return base::nullopt;
  }

  // Since |header.additional_info| <= 24, ReadVariadicLengthInteger also
  // provides this bound for |header.value|.
  CHECK_LE(header.value, 255u);
  // |SimpleValue| is an enum class and so the underlying type is specified to
  // be |int|. So this cast is safe.
  CBORValue::SimpleValue possibly_unsupported_simple_value =
      static_cast<CBORValue::SimpleValue>(static_cast<int>(header.value));
  switch (possibly_unsupported_simple_value) {
    case CBORValue::SimpleValue::FALSE_VALUE:
    case CBORValue::SimpleValue::TRUE_VALUE:
    case CBORValue::SimpleValue::NULL_VALUE:
    case CBORValue::SimpleValue::UNDEFINED:
      return CBORValue(possibly_unsupported_simple_value);
  }

  error_code_ = DecoderError::UNSUPPORTED_SIMPLE_VALUE;
  return base::nullopt;
}

base::Optional<CBORValue> CBORReader::ReadStringContent(
    const CBORReader::DataItemHeader& header) {
  uint64_t num_bytes = header.value;
  if (!CanConsume(num_bytes)) {
    return base::nullopt;
  }

  std::string cbor_string(it_, it_ + num_bytes);
  it_ += num_bytes;

  return HasValidUTF8Format(cbor_string)
             ? base::make_optional<CBORValue>(CBORValue(std::move(cbor_string)))
             : base::nullopt;
}

base::Optional<CBORValue> CBORReader::ReadByteStringContent(
    const CBORReader::DataItemHeader& header) {
  uint64_t num_bytes = header.value;
  if (!CanConsume(num_bytes)) {
    return base::nullopt;
  }

  std::vector<uint8_t> cbor_byte_string(it_, it_ + num_bytes);
  it_ += num_bytes;

  return CBORValue(std::move(cbor_byte_string));
}

base::Optional<CBORValue> CBORReader::ReadArrayContent(
    const CBORReader::DataItemHeader& header,
    int max_nesting_level) {
  uint64_t length = header.value;

  CBORValue::ArrayValue cbor_array;
  for (uint64_t i = 0; i < length; ++i) {
    base::Optional<CBORValue> cbor_element =
        DecodeCompleteDataItem(max_nesting_level - 1);
    if (!cbor_element.has_value())
      return base::nullopt;
    cbor_array.push_back(std::move(cbor_element.value()));
  }
  return CBORValue(std::move(cbor_array));
}

base::Optional<CBORValue> CBORReader::ReadMapContent(
    const CBORReader::DataItemHeader& header,
    int max_nesting_level) {
  uint64_t length = header.value;

  CBORValue::MapValue cbor_map;
  for (uint64_t i = 0; i < length; ++i) {
    base::Optional<CBORValue> key =
        DecodeCompleteDataItem(max_nesting_level - 1);
    base::Optional<CBORValue> value =
        DecodeCompleteDataItem(max_nesting_level - 1);
    if (!key.has_value() || !value.has_value())
      return base::nullopt;

    switch (key.value().type()) {
      case CBORValue::Type::UNSIGNED:
      case CBORValue::Type::NEGATIVE:
      case CBORValue::Type::STRING:
      case CBORValue::Type::BYTE_STRING:
        break;
      default:
        error_code_ = DecoderError::INCORRECT_MAP_KEY_TYPE;
        return base::nullopt;
    }
    if (!CheckOutOfOrderKey(key.value(), &cbor_map)) {
      return base::nullopt;
    }

    cbor_map.insert_or_assign(std::move(key.value()), std::move(value.value()));
  }
  return CBORValue(std::move(cbor_map));
}

bool CBORReader::CanConsume(uint64_t bytes) {
  if (base::checked_cast<uint64_t>(std::distance(it_, end_)) >= bytes) {
    return true;
  }
  error_code_ = DecoderError::INCOMPLETE_CBOR_DATA;
  return false;
}

bool CBORReader::CheckMinimalEncoding(uint8_t additional_bytes,
                                      uint64_t uint_data) {
  if ((additional_bytes == 1 && uint_data < 24) ||
      uint_data <= (1ULL << 8 * (additional_bytes >> 1)) - 1) {
    error_code_ = DecoderError::NON_MINIMAL_CBOR_ENCODING;
    return false;
  }
  return true;
}

bool CBORReader::HasValidUTF8Format(const std::string& string_data) {
  if (!base::IsStringUTF8(string_data)) {
    error_code_ = DecoderError::INVALID_UTF8;
    return false;
  }
  return true;
}

bool CBORReader::CheckOutOfOrderKey(const CBORValue& new_key,
                                    CBORValue::MapValue* map) {
  if (map->empty()) {
    return true;
  }

  const auto& max_current_key = map->rbegin()->first;
  const auto less = map->key_comp();
  if (!less(max_current_key, new_key)) {
    error_code_ = DecoderError::OUT_OF_ORDER_KEY;
    return false;
  }
  return true;
}

CBORReader::DecoderError CBORReader::GetErrorCode() {
  return error_code_;
}

// static
const char* CBORReader::ErrorCodeToString(DecoderError error) {
  switch (error) {
    case DecoderError::CBOR_NO_ERROR:
      return kNoError;
    case DecoderError::UNSUPPORTED_MAJOR_TYPE:
      return kUnsupportedMajorType;
    case DecoderError::UNKNOWN_ADDITIONAL_INFO:
      return kUnknownAdditionalInfo;
    case DecoderError::INCOMPLETE_CBOR_DATA:
      return kIncompleteCBORData;
    case DecoderError::INCORRECT_MAP_KEY_TYPE:
      return kIncorrectMapKeyType;
    case DecoderError::TOO_MUCH_NESTING:
      return kTooMuchNesting;
    case DecoderError::INVALID_UTF8:
      return kInvalidUTF8;
    case DecoderError::EXTRANEOUS_DATA:
      return kExtraneousData;
    case DecoderError::OUT_OF_ORDER_KEY:
      return kMapKeyOutOfOrder;
    case DecoderError::NON_MINIMAL_CBOR_ENCODING:
      return kNonMinimalCBOREncoding;
    case DecoderError::UNSUPPORTED_SIMPLE_VALUE:
      return kUnsupportedSimpleValue;
    case DecoderError::UNSUPPORTED_FLOATING_POINT_VALUE:
      return kUnsupportedFloatingPointValue;
    case DecoderError::OUT_OF_RANGE_INTEGER_VALUE:
      return kOutOfRangeIntegerValue;
    case DecoderError::UNKNOWN_ERROR:
      return kUnknownError;
    default:
      NOTREACHED();
      return "Unknown error code.";
  }
}

}  // namespace cbor
