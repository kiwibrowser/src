// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>

#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_string.h"
#include "fxjs/cfxjse_formcalc_context.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FX_SAFE_SIZE_T safe_size = size;
  if (!safe_size.IsValid())
    return 0;

  CFX_WideTextBuf js;
  WideString input =
      WideString::FromUTF8(ByteStringView(data, safe_size.ValueOrDie()));
  CFXJSE_FormCalcContext::Translate(input.AsStringView(), &js);
  return 0;
}
