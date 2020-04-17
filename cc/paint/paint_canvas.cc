// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_canvas.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/paint_recorder.h"
#include "third_party/skia/include/core/SkAnnotation.h"
#include "third_party/skia/include/core/SkMetaData.h"

#if defined(OS_MACOSX)
namespace {
const char kIsPreviewMetafileKey[] = "CrIsPreviewMetafile";
}
#endif

namespace cc {

#if defined(OS_MACOSX)
void SetIsPreviewMetafile(PaintCanvas* canvas, bool is_preview) {
  SkMetaData& meta = canvas->getMetaData();
  meta.setBool(kIsPreviewMetafileKey, is_preview);
}

bool IsPreviewMetafile(PaintCanvas* canvas) {
  bool value;
  SkMetaData& meta = canvas->getMetaData();
  if (!meta.findBool(kIsPreviewMetafileKey, &value))
    value = false;
  return value;
}
#endif

}  // namespace cc
