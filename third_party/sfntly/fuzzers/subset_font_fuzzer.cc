// Copyright 2016 The Chromimum Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <string>

#include "base/test/fuzzed_data_provider.h"
#include "third_party/sfntly/src/cpp/src/sample/chromium/font_subsetter.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  constexpr int kMaxFontNameSize = 128;
  constexpr int kMaxFontSize = 50 * 1024 * 1024;
  base::FuzzedDataProvider fuzzed_data(data, size);

  size_t font_name_size = fuzzed_data.ConsumeUint32InRange(0, kMaxFontNameSize);
  std::string font_name = fuzzed_data.ConsumeBytes(font_name_size);

  size_t font_str_size = fuzzed_data.ConsumeUint32InRange(0, kMaxFontSize);
  std::string font_str = fuzzed_data.ConsumeBytes(font_str_size);
  const unsigned char* font_data =
      reinterpret_cast<const unsigned char*>(font_str.data());

  std::string glyph_ids_str = fuzzed_data.ConsumeRemainingBytes();
  const unsigned int* glyph_ids =
      reinterpret_cast<const unsigned int*>(glyph_ids_str.data());
  size_t glyph_ids_size =
      glyph_ids_str.size() * sizeof(char) / sizeof(unsigned int);

  unsigned char* output = nullptr;
  SfntlyWrapper::SubsetFont(font_name.data(), font_data, font_str_size,
                            glyph_ids, glyph_ids_size, &output);
  delete[] output;
  return 0;
}
