/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SHARED_PERSISTENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SHARED_PERSISTENT_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/bindings/scoped_persistent.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "v8/include/v8.h"

namespace blink {

// A ref counted version of ScopedPersistent. This class is intended for use by
// ScriptValue and not for normal use. Consider using ScopedPersistent directly
// instead.
template <typename T>
class SharedPersistent : public RefCounted<SharedPersistent<T>> {
  WTF_MAKE_NONCOPYABLE(SharedPersistent);

 public:
  static scoped_refptr<SharedPersistent<T>> Create(v8::Local<T> value,
                                                   v8::Isolate* isolate) {
    return base::AdoptRef(new SharedPersistent<T>(value, isolate));
  }

  v8::Local<T> NewLocal(v8::Isolate* isolate) const {
    return value_.NewLocal(isolate);
  }

  bool IsEmpty() { return value_.IsEmpty(); }

  bool operator==(const SharedPersistent<T>& other) {
    return value_ == other.value_;
  }

 private:
  explicit SharedPersistent(v8::Local<T> value, v8::Isolate* isolate)
      : value_(isolate, value) {}
  ScopedPersistent<T> value_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SHARED_PERSISTENT_H_
