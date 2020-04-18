// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "core/fpdfapi/page/cpdf_psengine.h"
#include "third_party/base/span.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  CPDF_PSEngine engine;
  if (engine.Parse(pdfium::make_span(data, size)))
    engine.Execute();
  return 0;
}
