// Copyright 2017 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "core/fxcodec/gif/cfx_lzwdecompressor.h"
#include "third_party/base/numerics/safe_conversions.h"

// Between 2x and 5x is a standard range for LZW according to a quick
// search of papers. Running up to 10x to catch any niche cases.
constexpr uint32_t kMinCompressionRatio = 2;
constexpr uint32_t kMaxCompressionRatio = 10;

void LZWFuzz(const uint8_t* src_buf,
             size_t src_size,
             uint8_t color_exp,
             uint8_t code_exp) {
  std::unique_ptr<CFX_LZWDecompressor> decompressor =
      CFX_LZWDecompressor::Create(color_exp, code_exp);
  if (!decompressor)
    return;

  for (uint32_t compressions_ratio = kMinCompressionRatio;
       compressions_ratio <= kMaxCompressionRatio; compressions_ratio++) {
    std::vector<uint8_t> dest_buf(compressions_ratio * src_size);
    // This cast should be safe since the caller is checking for overflow on
    // the initial data.
    uint32_t dest_size = static_cast<uint32_t>(dest_buf.size());
    if (CFX_GifDecodeStatus::InsufficientDestSize !=
        decompressor->Decode(const_cast<uint8_t*>(src_buf), src_size,
                             dest_buf.data(), &dest_size))
      return;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Need at least 3 bytes to do anything.
  if (size < 3)
    return 0;

  // Normally the GIF would provide the code and color sizes, instead, going
  // to assume they are the first two bytes of data provided.
  uint8_t color_exp = data[0];
  uint8_t code_exp = data[1];
  const uint8_t* lzw_data = data + 2;
  uint32_t lzw_data_size = static_cast<uint32_t>(size - 2);
  // Check that there isn't going to be an overflow in the destination buffer
  // size.
  if (lzw_data_size >
      std::numeric_limits<uint32_t>::max() / kMaxCompressionRatio) {
    return 0;
  }

  LZWFuzz(lzw_data, lzw_data_size, color_exp, code_exp);

  return 0;
}
