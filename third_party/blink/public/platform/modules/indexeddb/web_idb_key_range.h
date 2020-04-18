/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_RANGE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_RANGE_H_

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"

namespace blink {

class IDBKeyRange;

class WebIDBKeyRange {
 public:
  ~WebIDBKeyRange() { Reset(); }

  WebIDBKeyRange() = default;
  WebIDBKeyRange(const WebIDBKeyRange& key_range) { Assign(key_range); }
  WebIDBKeyRange(WebIDBKey lower,
                 WebIDBKey upper,
                 bool lower_open,
                 bool upper_open) {
    Assign(std::move(lower), std::move(upper), lower_open, upper_open);
  }

  BLINK_EXPORT WebIDBKeyView Lower() const;
  BLINK_EXPORT WebIDBKeyView Upper() const;
  BLINK_EXPORT bool LowerOpen() const;
  BLINK_EXPORT bool UpperOpen() const;

  BLINK_EXPORT void Assign(const WebIDBKeyRange&);
  BLINK_EXPORT void Assign(WebIDBKey lower,
                           WebIDBKey upper,
                           bool lower_open,
                           bool upper_open);

  WebIDBKeyRange& operator=(const WebIDBKeyRange& e) {
    Assign(e);
    return *this;
  }

  BLINK_EXPORT void Reset();

#if INSIDE_BLINK
  WebIDBKeyRange(IDBKeyRange* value) : private_(value) {}
  WebIDBKeyRange& operator=(IDBKeyRange* value) {
    private_ = value;
    return *this;
  }
  operator IDBKeyRange*() const { return private_.Get(); }
#endif

 private:
  WebPrivatePtr<IDBKeyRange> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_KEY_RANGE_H_
