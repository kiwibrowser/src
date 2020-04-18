// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/fontmgr_default_android.h"

#include "third_party/skia/include/ports/SkFontMgr.h"
#include "third_party/skia/include/ports/SkFontMgr_android.h"

namespace {
// An owning leaky bare pointer.
SkFontMgr* g_default_fontmgr;
}  // namespace

SK_API void SetDefaultSkiaFactory(sk_sp<SkFontMgr> fontmgr) {
  SkASSERT(g_default_fontmgr == nullptr);
  g_default_fontmgr = fontmgr.release();
}

SK_API sk_sp<SkFontMgr> SkFontMgr::Factory() {
  return g_default_fontmgr ? sk_ref_sp(g_default_fontmgr)
                           : SkFontMgr_New_Android(nullptr);
}
