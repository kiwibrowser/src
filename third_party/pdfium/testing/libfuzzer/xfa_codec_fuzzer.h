// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_LIBFUZZER_XFA_CODEC_FUZZER_H_
#define TESTING_LIBFUZZER_XFA_CODEC_FUZZER_H_

#include <memory>

#include "core/fxcodec/codec/ccodec_progressivedecoder.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "testing/fx_string_testhelpers.h"
#include "third_party/base/ptr_util.h"

#ifdef PDF_ENABLE_XFA_BMP
#include "core/fxcodec/codec/ccodec_bmpmodule.h"
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
#include "core/fxcodec/codec/ccodec_gifmodule.h"
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_PNG
#include "core/fxcodec/codec/ccodec_pngmodule.h"
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_TIFF
#include "core/fxcodec/codec/ccodec_tiffmodule.h"
#endif  // PDF_ENABLE_XFA_TIFF

// Support up to 64 MB. This prevents trivial OOM when MSAN is on and
// time outs.
const int kXFACodecFuzzerPixelLimit = 64000000;

class XFACodecFuzzer {
 public:
  static int Fuzz(const uint8_t* data, size_t size, FXCODEC_IMAGE_TYPE type) {
    auto mgr = pdfium::MakeUnique<CCodec_ModuleMgr>();
#ifdef PDF_ENABLE_XFA_BMP
    mgr->SetBmpModule(pdfium::MakeUnique<CCodec_BmpModule>());
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
    mgr->SetGifModule(pdfium::MakeUnique<CCodec_GifModule>());
#endif  // PDF_ENABLE_XFA_GIF
#ifdef PDF_ENABLE_XFA_PNG
    mgr->SetPngModule(pdfium::MakeUnique<CCodec_PngModule>());
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
    mgr->SetTiffModule(pdfium::MakeUnique<CCodec_TiffModule>());
#endif  // PDF_ENABLE_XFA_TIFF

    std::unique_ptr<CCodec_ProgressiveDecoder> decoder =
        mgr->CreateProgressiveDecoder();
    auto source = pdfium::MakeRetain<CFX_BufferSeekableReadStream>(data, size);
    FXCODEC_STATUS status = decoder->LoadImageInfo(source, type, nullptr, true);
    if (status != FXCODEC_STATUS_FRAME_READY)
      return 0;

    // Skipping very large images, since they will take a long time and may lead
    // to OOM.
    FX_SAFE_UINT32 bitmap_size = decoder->GetHeight();
    bitmap_size *= decoder->GetWidth();
    bitmap_size *= 4;  // From CFX_DIBitmap impl.
    if (!bitmap_size.IsValid() ||
        bitmap_size.ValueOrDie() > kXFACodecFuzzerPixelLimit) {
      return 0;
    }

    auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    bitmap->Create(decoder->GetWidth(), decoder->GetHeight(), FXDIB_Argb);

    size_t frames;
    std::tie(status, frames) = decoder->GetFrames();
    if (status != FXCODEC_STATUS_DECODE_READY || frames == 0)
      return 0;

    status = decoder->StartDecode(bitmap, 0, 0, bitmap->GetWidth(),
                                  bitmap->GetHeight());
    while (status == FXCODEC_STATUS_DECODE_TOBECONTINUE)
      status = decoder->ContinueDecode();

    return 0;
  }
};

#endif  // TESTING_LIBFUZZER_XFA_CODEC_FUZZER_H_
