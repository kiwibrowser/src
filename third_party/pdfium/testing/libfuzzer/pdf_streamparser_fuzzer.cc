// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <memory>

#include "core/fpdfapi/page/cpdf_streamparser.h"
#include "core/fpdfapi/parser/cpdf_object.h"
#include "third_party/base/span.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  CPDF_StreamParser parser(pdfium::make_span(data, size));
  while (std::unique_ptr<CPDF_Object> pObj =
             parser.ReadNextObject(true, false, 0))
    continue;

  return 0;
}
