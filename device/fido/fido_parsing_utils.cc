// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_parsing_utils.h"

#include "base/logging.h"
#include "crypto/sha2.h"

namespace device {
namespace fido_parsing_utils {

namespace {

constexpr bool AreSpansDisjoint(base::span<const uint8_t> lhs,
                                base::span<const uint8_t> rhs) {
  return lhs.data() + lhs.size() <= rhs.data() ||  // [lhs)...[rhs)
         rhs.data() + rhs.size() <= lhs.data();    // [rhs)...[lhs)
}

}  // namespace

const uint32_t kU2fResponseKeyHandleLengthPos = 66u;
const uint32_t kU2fResponseKeyHandleStartPos = 67u;
const char kEs256[] = "ES256";

std::vector<uint8_t> Materialize(base::span<const uint8_t> span) {
  return std::vector<uint8_t>(span.begin(), span.end());
}

base::Optional<std::vector<uint8_t>> MaterializeOrNull(
    base::Optional<base::span<const uint8_t>> span) {
  if (span)
    return Materialize(*span);
  return base::nullopt;
}

void Append(std::vector<uint8_t>* target, base::span<const uint8_t> in_values) {
  CHECK(AreSpansDisjoint(*target, in_values));
  target->insert(target->end(), in_values.begin(), in_values.end());
}

std::vector<uint8_t> Extract(base::span<const uint8_t> span,
                             size_t pos,
                             size_t length) {
  return Materialize(ExtractSpan(span, pos, length));
}

base::span<const uint8_t> ExtractSpan(base::span<const uint8_t> span,
                                      size_t pos,
                                      size_t length) {
  if (!(pos <= span.size() && length <= span.size() - pos))
    return base::span<const uint8_t>();
  return span.subspan(pos, length);
}

std::vector<uint8_t> ExtractSuffix(base::span<const uint8_t> span, size_t pos) {
  return Materialize(ExtractSuffixSpan(span, pos));
}

base::span<const uint8_t> ExtractSuffixSpan(base::span<const uint8_t> span,
                                            size_t pos) {
  if (pos > span.size())
    return std::vector<uint8_t>();
  return span.subspan(pos);
}

std::vector<base::span<const uint8_t>> SplitSpan(base::span<const uint8_t> span,
                                                 size_t max_chunk_size) {
  DCHECK_NE(0u, max_chunk_size);
  std::vector<base::span<const uint8_t>> chunks;
  const size_t num_chunks = (span.size() + max_chunk_size - 1) / max_chunk_size;
  chunks.reserve(num_chunks);
  while (!span.empty()) {
    const size_t chunk_size = std::min(span.size(), max_chunk_size);
    chunks.emplace_back(span.subspan(0, chunk_size));
    span = span.subspan(chunk_size);
  }

  return chunks;
}

std::vector<uint8_t> CreateSHA256Hash(base::StringPiece data) {
  std::vector<uint8_t> hashed_data(crypto::kSHA256Length);
  crypto::SHA256HashString(data, hashed_data.data(), hashed_data.size());
  return hashed_data;
}

}  // namespace fido_parsing_utils
}  // namespace device
