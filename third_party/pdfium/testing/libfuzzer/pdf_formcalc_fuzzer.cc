// Copyright 2017 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_string.h"
#include "xfa/fxfa/fm2js/cxfa_fmparser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  WideString input = WideString::FromUTF8(ByteStringView(data, size));

  CXFA_FMParser parser(input.AsStringView());
  parser.Parse();

  return 0;
}
