// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_PARSING_UTILS_H_
#define DEVICE_FIDO_FIDO_PARSING_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "base/optional.h"
#include "base/strings/string_piece.h"

namespace device {
namespace fido_parsing_utils {

// Comparator object that calls base::make_span on its arguments before
// comparing them with operator<. Useful when comparing sequence containers that
// are of different types, but have similar semantics.
struct SpanLess {
  template <typename T, typename U>
  constexpr bool operator()(T&& lhs, U&& rhs) const {
    return base::make_span(std::forward<T>(lhs)) <
           base::make_span(std::forward<U>(rhs));
  }

  using is_transparent = void;
};

// U2FResponse offsets. The format of a U2F response is defined in
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-raw-message-formats-v1.2-ps-20170411.html#registration-response-message-success
COMPONENT_EXPORT(DEVICE_FIDO)
extern const uint32_t kU2fResponseKeyHandleLengthPos;
COMPONENT_EXPORT(DEVICE_FIDO)
extern const uint32_t kU2fResponseKeyHandleStartPos;
COMPONENT_EXPORT(DEVICE_FIDO) extern const char kEs256[];

// Returns a materialized copy of |span|, that is, a vector with the same
// elements.
COMPONENT_EXPORT(DEVICE_FIDO)
std::vector<uint8_t> Materialize(base::span<const uint8_t> span);
COMPONENT_EXPORT(DEVICE_FIDO)
base::Optional<std::vector<uint8_t>> MaterializeOrNull(
    base::Optional<base::span<const uint8_t>> span);

// Appends |in_values| to the end of |target|. The underlying container for
// |in_values| should *not* be |target|.
COMPONENT_EXPORT(DEVICE_FIDO)
void Append(std::vector<uint8_t>* target, base::span<const uint8_t> in_values);

// Safely extracts, with bound checking, a contiguous subsequence of |span| of
// the given |length| and starting at |pos|. Returns an empty vector/span if the
// requested range is out-of-bound.
COMPONENT_EXPORT(DEVICE_FIDO)
std::vector<uint8_t> Extract(base::span<const uint8_t> span,
                             size_t pos,
                             size_t length);
COMPONENT_EXPORT(DEVICE_FIDO)
base::span<const uint8_t> ExtractSpan(base::span<const uint8_t> span,
                                      size_t pos,
                                      size_t length);

// Safely extracts, with bound checking, the suffix of the given |span| starting
// at the given position |pos|. Returns an empty vector/span if the requested
// starting position is out-of-bound.
COMPONENT_EXPORT(DEVICE_FIDO)
std::vector<uint8_t> ExtractSuffix(base::span<const uint8_t> span, size_t pos);

COMPONENT_EXPORT(DEVICE_FIDO)
base::span<const uint8_t> ExtractSuffixSpan(base::span<const uint8_t> span,
                                            size_t pos);

template <size_t N>
bool ExtractArray(base::span<const uint8_t> span,
                  size_t pos,
                  std::array<uint8_t, N>* array) {
  const auto extracted_span = ExtractSpan(span, pos, N);
  if (extracted_span.size() != N)
    return false;

  std::copy(extracted_span.begin(), extracted_span.end(), array->begin());
  return true;
}

// Partitions |span| into N = ⌈span.size() / max_chunk_size⌉ consecutive chunks.
// The first N-1 chunks are of size |max_chunk_size|, and the Nth chunk is of
// size span.size() % max_chunk_size. |max_chunk_size| must be greater than 0.
// Returns an empty vector in case |span| is empty.
COMPONENT_EXPORT(DEVICE_FIDO)
std::vector<base::span<const uint8_t>> SplitSpan(base::span<const uint8_t> span,
                                                 size_t max_chunk_size);

COMPONENT_EXPORT(DEVICE_FIDO)
std::vector<uint8_t> CreateSHA256Hash(base::StringPiece data);

}  // namespace fido_parsing_utils
}  // namespace device

#endif  // DEVICE_FIDO_FIDO_PARSING_UTILS_H_
