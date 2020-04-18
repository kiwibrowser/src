// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/fontmgr_default_win.h"

#include "third_party/skia/include/ports/SkFontMgr.h"
#include "third_party/skia/include/ports/SkTypeface_win.h"

namespace {

// This is a leaky bare owning pointer.
SkFontMgr* g_default_fontmgr;

// The ppapi code currently calls SetDefaultSkiaFactory twice on Win8+.
// This tracks when the global escapes and shouldno longer be set.
SkDEBUGCODE(bool g_factory_called;)

}  // namespace

void SetDefaultSkiaFactory(sk_sp<SkFontMgr> fontmgr) {
  SkASSERT(!g_factory_called);

  SkSafeUnref(g_default_fontmgr);
  g_default_fontmgr = fontmgr.release();
}

SK_API sk_sp<SkFontMgr> SkFontMgr::Factory() {
  SkDEBUGCODE(g_factory_called = true;)

  // This will be set when DirectWrite is in use, and an SkFontMgr has been
  // created with the pre-sandbox warmed up one. Otherwise, we fallback to a
  // GDI SkFontMgr which is used in the browser.
  if (g_default_fontmgr)
    return sk_ref_sp(g_default_fontmgr);
  return SkFontMgr_New_GDI();
}
