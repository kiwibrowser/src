// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_text_blob.h"

#include <vector>

#include "cc/paint/paint_typeface.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace cc {

PaintTextBlob::PaintTextBlob() = default;
PaintTextBlob::PaintTextBlob(sk_sp<SkTextBlob> blob,
                             std::vector<PaintTypeface> typefaces)
    : sk_blob_(std::move(blob)), typefaces_(std::move(typefaces)) {}
PaintTextBlob::~PaintTextBlob() = default;

}  // namespace cc
