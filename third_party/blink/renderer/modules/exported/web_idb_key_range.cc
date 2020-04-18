/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key_range.h"

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key_range.h"

namespace blink {

void WebIDBKeyRange::Assign(const WebIDBKeyRange& other) {
  private_ = other.private_;
}

void WebIDBKeyRange::Assign(WebIDBKey lower,
                            WebIDBKey upper,
                            bool lower_open,
                            bool upper_open) {
  if (!lower.View().IsValid() && !upper.View().IsValid()) {
    private_.Reset();
  } else {
    private_ = IDBKeyRange::Create(lower.ReleaseIdbKey(), upper.ReleaseIdbKey(),
                                   lower_open ? IDBKeyRange::kLowerBoundOpen
                                              : IDBKeyRange::kLowerBoundClosed,
                                   upper_open ? IDBKeyRange::kUpperBoundOpen
                                              : IDBKeyRange::kUpperBoundClosed);
  }
}

void WebIDBKeyRange::Reset() {
  private_.Reset();
}

WebIDBKeyView WebIDBKeyRange::Lower() const {
  if (!private_.Get())
    return WebIDBKeyView(nullptr);
  return WebIDBKeyView(private_->Lower());
}

WebIDBKeyView WebIDBKeyRange::Upper() const {
  if (!private_.Get())
    return WebIDBKeyView(nullptr);
  return WebIDBKeyView(private_->Upper());
}

bool WebIDBKeyRange::LowerOpen() const {
  return private_.Get() && private_->lowerOpen();
}

bool WebIDBKeyRange::UpperOpen() const {
  return private_.Get() && private_->upperOpen();
}

}  // namespace blink
