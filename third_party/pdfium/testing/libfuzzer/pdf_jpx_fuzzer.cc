// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <memory>
#include <vector>

#include "core/fxcodec/codec/ccodec_jpxmodule.h"
#include "core/fxcodec/codec/cjpx_decoder.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/fx_dib.h"

CCodec_JpxModule g_module;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::unique_ptr<CJPX_Decoder> decoder =
      g_module.CreateDecoder(data, size, nullptr);
  if (!decoder)
    return 0;

  uint32_t width;
  uint32_t height;
  uint32_t components;
  g_module.GetImageInfo(decoder.get(), &width, &height, &components);

  static constexpr uint32_t kMemLimit = 1024 * 1024 * 1024;  // 1 GB.
  FX_SAFE_UINT32 mem = width;
  mem *= height;
  mem *= components;
  if (!mem.IsValid() || mem.ValueOrDie() > kMemLimit)
    return 0;

  FXDIB_Format format;
  if (components == 1) {
    format = FXDIB_8bppRgb;
  } else if (components <= 3) {
    format = FXDIB_Rgb;
  } else if (components == 4) {
    format = FXDIB_Rgb32;
  } else {
    width = (width * components + 2) / 3;
    format = FXDIB_Rgb;
  }
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!bitmap->Create(width, height, format))
    return 0;

  std::vector<uint8_t> output_offsets(components);
  for (uint32_t i = 0; i < components; ++i)
    output_offsets[i] = i;

  g_module.Decode(decoder.get(), bitmap->GetBuffer(), bitmap->GetPitch(),
                  output_offsets);
  return 0;
}
