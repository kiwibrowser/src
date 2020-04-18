// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/metafile_skia_wrapper.h"
#include "third_party/skia/include/core/SkMetaData.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace printing {

namespace {

const char kMetafileKey[] = "CrMetafile";

}  // namespace

// static
void MetafileSkiaWrapper::SetMetafileOnCanvas(cc::PaintCanvas* canvas,
                                              PdfMetafileSkia* metafile) {
  sk_sp<MetafileSkiaWrapper> wrapper;
  // Can't use sk_make_sp<>() because the constructor is private.
  if (metafile)
    wrapper = sk_sp<MetafileSkiaWrapper>(new MetafileSkiaWrapper(metafile));

  SkMetaData& meta = canvas->getMetaData();
  meta.setRefCnt(kMetafileKey, wrapper.get());
}

// static
PdfMetafileSkia* MetafileSkiaWrapper::GetMetafileFromCanvas(
    cc::PaintCanvas* canvas) {
  SkMetaData& meta = canvas->getMetaData();
  SkRefCnt* value;
  if (!meta.findRefCnt(kMetafileKey, &value) || !value)
    return nullptr;

  return static_cast<MetafileSkiaWrapper*>(value)->metafile_;
}

MetafileSkiaWrapper::MetafileSkiaWrapper(PdfMetafileSkia* metafile)
    : metafile_(metafile) {
}

}  // namespace printing
