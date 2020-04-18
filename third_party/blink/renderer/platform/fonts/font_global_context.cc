// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/font_global_context.h"

#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"

namespace blink {

FontGlobalContext* FontGlobalContext::Get(CreateIfNeeded create_if_needed) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<FontGlobalContext*>,
                                  font_persistent, ());
  if (!*font_persistent && create_if_needed == kCreate) {
    *font_persistent = new FontGlobalContext();
  }
  return *font_persistent;
}

FontGlobalContext::FontGlobalContext()
    : harfbuzz_font_funcs_(nullptr),
      default_locale_(nullptr),
      system_locale_(nullptr),
      default_locale_for_han_(nullptr),
      has_default_locale_for_han_(false) {}

void FontGlobalContext::ClearMemory() {
  if (!Get(kDoNotCreate))
    return;

  GetFontCache().Invalidate();
}

void FontGlobalContext::ClearForTesting() {
  FontGlobalContext* ctx = Get();
  ctx->default_locale_ = nullptr;
  ctx->system_locale_ = nullptr;
  ctx->default_locale_for_han_ = nullptr;
  ctx->has_default_locale_for_han_ = false;
  ctx->layout_locale_map_.clear();
  ctx->font_cache_.Invalidate();
}

}  // namespace blink
