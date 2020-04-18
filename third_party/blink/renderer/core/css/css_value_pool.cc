/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/css_value_pool.h"

#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

using namespace cssvalue;

CSSValuePool& CssValuePool() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<Persistent<CSSValuePool>>,
                                  thread_specific_pool, ());
  Persistent<CSSValuePool>& pool_handle = *thread_specific_pool;
  if (!pool_handle) {
    pool_handle = new CSSValuePool;
    pool_handle.RegisterAsStaticReference();
  }
  return *pool_handle;
}

CSSValuePool::CSSValuePool()
    : inherited_value_(new CSSInheritedValue),
      initial_value_(new CSSInitialValue()),
      unset_value_(new CSSUnsetValue),
      color_transparent_(new CSSColorValue(Color::kTransparent)),
      color_white_(new CSSColorValue(Color::kWhite)),
      color_black_(new CSSColorValue(Color::kBlack)) {
  identifier_value_cache_.resize(numCSSValueKeywords);
  pixel_value_cache_.resize(kMaximumCacheableIntegerValue + 1);
  percent_value_cache_.resize(kMaximumCacheableIntegerValue + 1);
  number_value_cache_.resize(kMaximumCacheableIntegerValue + 1);
}

void CSSValuePool::Trace(blink::Visitor* visitor) {
  visitor->Trace(inherited_value_);
  visitor->Trace(initial_value_);
  visitor->Trace(unset_value_);
  visitor->Trace(color_transparent_);
  visitor->Trace(color_white_);
  visitor->Trace(color_black_);
  visitor->Trace(identifier_value_cache_);
  visitor->Trace(pixel_value_cache_);
  visitor->Trace(percent_value_cache_);
  visitor->Trace(number_value_cache_);
  visitor->Trace(color_value_cache_);
  visitor->Trace(font_face_value_cache_);
  visitor->Trace(font_family_value_cache_);
}

}  // namespace blink
