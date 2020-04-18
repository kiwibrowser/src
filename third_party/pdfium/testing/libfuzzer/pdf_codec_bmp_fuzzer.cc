// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/libfuzzer/xfa_codec_fuzzer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return XFACodecFuzzer::Fuzz(data, size, FXCODEC_IMAGE_BMP);
}
