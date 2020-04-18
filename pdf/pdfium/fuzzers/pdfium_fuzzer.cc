// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdfium/fuzzers/pdfium_fuzzer_helper.h"

class PDFiumFuzzer : public PDFiumFuzzerHelper {
 public:
  PDFiumFuzzer() : PDFiumFuzzerHelper() {}
  ~PDFiumFuzzer() override = default;

  int GetFormCallbackVersion() const override { return 1; }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  PDFiumFuzzer fuzzer;
  fuzzer.RenderPdf(reinterpret_cast<const char*>(data), size);
  return 0;
}
