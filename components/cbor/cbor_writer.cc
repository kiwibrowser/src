// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_writer.h"

#include <string>

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_piece.h"
#include "components/cbor/cbor_binary.h"

namespace cbor {

CBORWriter::~CBORWriter() {}

// static
base::Optional<std::vector<uint8_t>> CBORWriter::Write(
    const CBORValue& node,
    size_t max_nesting_level) {
  std::vector<uint8_t> cbor;
  CBORWriter writer(&cbor);
  if (writer.EncodeCBOR(node, base::checked_cast<int>(max_nesting_level)))
    return cbor;
  return base::nullopt;
}

CBORWriter::CBORWriter(std::vector<uint8_t>* cbor) : encoded_cbor_(cbor) {}

bool CBORWriter::EncodeCBOR(const CBORValue& node, int max_nesting_level) {
  if (max_nesting_level < 0)
    return false;

  switch (node.type()) {
    case CBORValue::Type::NONE: {
      StartItem(CBORValue::Type::BYTE_STRING, 0);
      return true;
    }

    // Represents unsigned integers.
    case CBORValue::Type::UNSIGNED: {
      int64_t value = node.GetUnsigned();
      StartItem(CBORValue::Type::UNSIGNED, static_cast<uint64_t>(value));
      return true;
    }

    // Represents negative integers.
    case CBORValue::Type::NEGATIVE: {
      int64_t value = node.GetNegative();
      StartItem(CBORValue::Type::NEGATIVE, static_cast<uint64_t>(-(value + 1)));
      return true;
    }

    // Represents a byte string.
    case CBORValue::Type::BYTE_STRING: {
      const CBORValue::BinaryValue& bytes = node.GetBytestring();
      StartItem(CBORValue::Type::BYTE_STRING,
                base::strict_cast<uint64_t>(bytes.size()));
      // Add the bytes.
      encoded_cbor_->insert(encoded_cbor_->end(), bytes.begin(), bytes.end());
      return true;
    }

    case CBORValue::Type::STRING: {
      base::StringPiece string = node.GetString();
      StartItem(CBORValue::Type::STRING,
                base::strict_cast<uint64_t>(string.size()));

      // Add the characters.
      encoded_cbor_->insert(encoded_cbor_->end(), string.begin(), string.end());
      return true;
    }

    // Represents an array.
    case CBORValue::Type::ARRAY: {
      const CBORValue::ArrayValue& array = node.GetArray();
      StartItem(CBORValue::Type::ARRAY, array.size());
      for (const auto& value : array) {
        if (!EncodeCBOR(value, max_nesting_level - 1))
          return false;
      }
      return true;
    }

    // Represents a map.
    case CBORValue::Type::MAP: {
      const CBORValue::MapValue& map = node.GetMap();
      StartItem(CBORValue::Type::MAP, map.size());

      for (const auto& value : map) {
        if (!EncodeCBOR(value.first, max_nesting_level - 1))
          return false;
        if (!EncodeCBOR(value.second, max_nesting_level - 1))
          return false;
      }
      return true;
    }

    // Represents a simple value.
    case CBORValue::Type::SIMPLE_VALUE: {
      const CBORValue::SimpleValue simple_value = node.GetSimpleValue();
      StartItem(CBORValue::Type::SIMPLE_VALUE,
                base::checked_cast<uint64_t>(simple_value));
      return true;
    }
  }

  // This is needed because, otherwise, MSVC complains that not all paths return
  // a value. We should be able to remove it once MSVC builders are gone.
  NOTREACHED();
  return false;
}

void CBORWriter::StartItem(CBORValue::Type type, uint64_t size) {
  encoded_cbor_->push_back(base::checked_cast<uint8_t>(
      static_cast<unsigned>(type) << constants::kMajorTypeBitShift));
  SetUint(size);
}

void CBORWriter::SetAdditionalInformation(uint8_t additional_information) {
  DCHECK(!encoded_cbor_->empty());
  DCHECK_EQ(additional_information & constants::kAdditionalInformationMask,
            additional_information);
  encoded_cbor_->back() |=
      (additional_information & constants::kAdditionalInformationMask);
}

void CBORWriter::SetUint(uint64_t value) {
  size_t count = GetNumUintBytes(value);
  int shift = -1;
  // Values under 24 are encoded directly in the initial byte.
  // Otherwise, the last 5 bits of the initial byte contains the length
  // of unsigned integer, which is encoded in following bytes.
  switch (count) {
    case 0:
      SetAdditionalInformation(base::checked_cast<uint8_t>(value));
      break;
    case 1:
      SetAdditionalInformation(constants::kAdditionalInformation1Byte);
      shift = 0;
      break;
    case 2:
      SetAdditionalInformation(constants::kAdditionalInformation2Bytes);
      shift = 1;
      break;
    case 4:
      SetAdditionalInformation(constants::kAdditionalInformation4Bytes);
      shift = 3;
      break;
    case 8:
      SetAdditionalInformation(constants::kAdditionalInformation8Bytes);
      shift = 7;
      break;
    default:
      NOTREACHED();
      break;
  }
  for (; shift >= 0; shift--) {
    encoded_cbor_->push_back(0xFF & (value >> (shift * 8)));
  }
}

size_t CBORWriter::GetNumUintBytes(uint64_t value) {
  if (value < 24) {
    return 0;
  } else if (value <= 0xFF) {
    return 1;
  } else if (value <= 0xFFFF) {
    return 2;
  } else if (value <= 0xFFFFFFFF) {
    return 4;
  }
  return 8;
}

}  // namespace cbor
