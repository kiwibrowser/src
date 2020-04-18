// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CBOR_CBOR_READER_H_
#define COMPONENTS_CBOR_CBOR_READER_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/optional.h"
#include "components/cbor/cbor_export.h"
#include "components/cbor/cbor_values.h"

// Concise Binary Object Representation (CBOR) decoder as defined by
// https://tools.ietf.org/html/rfc7049. This decoder only accepts canonical
// CBOR as defined by section 3.9.
// Supported:
//  * Major types:
//     * 0: Unsigned integers, up to 64-bit.
//     * 2: Byte strings.
//     * 3: UTF-8 strings.
//     * 4: Definite-length arrays.
//     * 5: Definite-length maps.
//     * 7: Simple values.
//
// Requirements for canonical CBOR representation:
//  - Duplicate keys for map are not allowed.
//  - Keys for map must be sorted first by length and then by byte-wise
//    lexical order.
//
// Known limitations and interpretations of the RFC:
//  - Does not support negative integers, indefinite data streams and tagging.
//  - Floating point representations and BREAK stop code in major
//    type 7 are not supported.
//  - Non-character codepoint are not supported for Major type 3.
//  - Incomplete CBOR data items are treated as syntax errors.
//  - Trailing data bytes are treated as errors.
//  - Unknown additional information formats are treated as syntax errors.
//  - Callers can decode CBOR values with at most 16 nested depth layer. More
//    strict restrictions on nesting layer size of CBOR values can be enforced
//    by setting |max_nesting_level|.
//  - Only CBOR maps with integer or string type keys are supported due to the
//    cost of serialization when sorting map keys.
//  - Simple values that are unassigned/reserved as per RFC 7049 are not
//    supported and treated as errors.

namespace cbor {

class CBOR_EXPORT CBORReader {
 public:
  enum class DecoderError {
    CBOR_NO_ERROR = 0,
    UNSUPPORTED_MAJOR_TYPE,
    UNKNOWN_ADDITIONAL_INFO,
    INCOMPLETE_CBOR_DATA,
    INCORRECT_MAP_KEY_TYPE,
    TOO_MUCH_NESTING,
    INVALID_UTF8,
    EXTRANEOUS_DATA,
    OUT_OF_ORDER_KEY,
    NON_MINIMAL_CBOR_ENCODING,
    UNSUPPORTED_SIMPLE_VALUE,
    UNSUPPORTED_FLOATING_POINT_VALUE,
    OUT_OF_RANGE_INTEGER_VALUE,
    UNKNOWN_ERROR,
  };

  // CBOR nested depth sufficient for most use cases.
  static const int kCBORMaxDepth = 16;

  ~CBORReader();

  // Reads and parses |input_data| into a CBORValue. If any one of the syntax
  // formats is violated -including unknown additional info and incomplete
  // CBOR data- then an empty optional is returned. Optional |error_code_out|
  // can be provided by the caller to obtain additional information about
  // decoding failures, which is always available if an empty value is returned.
  //
  // Fails if not all the data was consumed and sets |error_code_out| to
  // EXTRANEOUS_DATA in this case.
  static base::Optional<CBORValue> Read(base::span<const uint8_t> input_data,
                                        DecoderError* error_code_out = nullptr,
                                        int max_nesting_level = kCBORMaxDepth);

  // Never fails with EXTRANEOUS_DATA, but informs the caller of how many bytes
  // were consumed through |num_bytes_consumed|.
  static base::Optional<CBORValue> Read(base::span<const uint8_t> input_data,
                                        size_t* num_bytes_consumed,
                                        DecoderError* error_code_out = nullptr,
                                        int max_nesting_level = kCBORMaxDepth);

  // Translates errors to human-readable error messages.
  static const char* ErrorCodeToString(DecoderError error_code);

 private:
  CBORReader(base::span<const uint8_t>::const_iterator it,
             const base::span<const uint8_t>::const_iterator end);

  // Encapsulates information extracted from the header of a CBOR data item,
  // which consists of the initial byte, and a variable-length-encoded integer
  // (if any).
  struct DataItemHeader {
    // The major type decoded from the initial byte.
    CBORValue::Type type;

    // The raw 5-bit additional information from the initial byte.
    uint8_t additional_info;

    // The integer |value| decoded from the |additional_info| and the
    // variable-length-encoded integer, if any.
    uint64_t value;
  };

  base::Optional<DataItemHeader> DecodeDataItemHeader();
  base::Optional<CBORValue> DecodeCompleteDataItem(int max_nesting_level);
  base::Optional<CBORValue> DecodeValueToNegative(uint64_t value);
  base::Optional<CBORValue> DecodeValueToUnsigned(uint64_t value);
  base::Optional<CBORValue> DecodeToSimpleValue(const DataItemHeader& header);
  bool ReadVariadicLengthInteger(uint8_t additional_info, uint64_t* value);
  base::Optional<CBORValue> ReadByteStringContent(const DataItemHeader& header);
  base::Optional<CBORValue> ReadStringContent(const DataItemHeader& header);
  base::Optional<CBORValue> ReadArrayContent(const DataItemHeader& header,
                                             int max_nesting_level);
  base::Optional<CBORValue> ReadMapContent(const DataItemHeader& header,
                                           int max_nesting_level);
  bool CanConsume(uint64_t bytes);
  bool HasValidUTF8Format(const std::string& string_data);
  bool CheckOutOfOrderKey(const CBORValue& new_key, CBORValue::MapValue* map);
  bool CheckMinimalEncoding(uint8_t additional_bytes, uint64_t uint_data);

  DecoderError GetErrorCode();

  size_t num_bytes_consumed() const { return it_ - begin_; }

  const base::span<const uint8_t>::const_iterator begin_;
  base::span<const uint8_t>::const_iterator it_;
  const base::span<const uint8_t>::const_iterator end_;
  DecoderError error_code_;

  DISALLOW_COPY_AND_ASSIGN(CBORReader);
};

}  // namespace cbor

#endif  // COMPONENTS_CBOR_CBOR_READER_H_
