// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"
#include "third_party/blink/renderer/platform/testing/blink_fuzzer_test_support.h"

#include <stddef.h>
#include <stdint.h>
#include <unicode/ustring.h>

namespace blink {

constexpr size_t kMaxInputLength = 256;

// TODO crbug.com/771901: BlinkFuzzerTestSupport should also initialize the
// custom fontconfig configuration that we use for content_shell.
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static BlinkFuzzerTestSupport fuzzer_support = BlinkFuzzerTestSupport();
  constexpr int32_t kDestinationCapacity = 2 * kMaxInputLength;
  int32_t converted_length = 0;
  UChar converted_input_buffer[kDestinationCapacity] = {0};
  UErrorCode error_code = U_ZERO_ERROR;

  // Discard trailing bytes.
  u_strFromUTF32(converted_input_buffer, kDestinationCapacity,
                 &converted_length, reinterpret_cast<const UChar32*>(data),
                 size / sizeof(UChar32), &error_code);
  if (U_FAILURE(error_code))
    return 0;

  FontCachePurgePreventer font_cache_purge_preventer;
  FontDescription font_description;
  Font font(font_description);
  // Set font size to something other than the default 0 size in
  // FontDescription, 16 matches the default text size in HTML.
  font_description.SetComputedSize(16.0f);
  // Only look for system fonts for now.
  font.Update(nullptr);

  HarfBuzzShaper shaper(converted_input_buffer, converted_length);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  return 0;
}

}  // namespace blink

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return blink::LLVMFuzzerTestOneInput(data, size);
}
