// Copyright 2017 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "core/fxcrt/fx_string.h"
#include "xfa/fwl/cfx_barcode.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 2 * sizeof(wchar_t))
    return 0;

  BC_TYPE type = static_cast<BC_TYPE>(data[0] % (BC_DATAMATRIX + 1));

  // Only used one byte, but align with wchar_t for string below.
  data += sizeof(wchar_t);
  size -= sizeof(wchar_t);

  auto barcode = CFX_Barcode::Create(type);
  if (!barcode)
    return 0;

  // TODO(tsepez): Setup more options from |data|.
  barcode->SetModuleHeight(300);
  barcode->SetModuleWidth(420);
  barcode->SetHeight(298);
  barcode->SetWidth(418);

  WideStringView content(reinterpret_cast<const wchar_t*>(data),
                         size / sizeof(wchar_t));

  if (!barcode->Encode(content))
    return 0;

  // TODO(tsepez): Output to device.
  return 0;
}
