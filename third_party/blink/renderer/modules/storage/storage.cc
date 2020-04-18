/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/storage/storage.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

Storage* Storage::Create(LocalFrame* frame, StorageArea* storage_area) {
  return new Storage(frame, storage_area);
}

Storage::Storage(LocalFrame* frame, StorageArea* storage_area)
    : ContextClient(frame), storage_area_(storage_area) {
  DCHECK(frame);
  DCHECK(storage_area_);
}

void Storage::NamedPropertyEnumerator(Vector<String>& names,
                                      ExceptionState& exception_state) {
  unsigned length = this->length(exception_state);
  if (exception_state.HadException())
    return;
  names.resize(length);
  for (unsigned i = 0; i < length; ++i) {
    String key = this->key(i, exception_state);
    if (exception_state.HadException())
      return;
    DCHECK(!key.IsNull());
    String val = getItem(key, exception_state);
    if (exception_state.HadException())
      return;
    names[i] = key;
  }
}

bool Storage::NamedPropertyQuery(const AtomicString& name,
                                 ExceptionState& exception_state) {
  if (name == "length")
    return false;
  bool found = Contains(name, exception_state);
  if (exception_state.HadException() || !found)
    return false;
  return true;
}

void Storage::Trace(blink::Visitor* visitor) {
  visitor->Trace(storage_area_);
  ScriptWrappable::Trace(visitor);
  ContextClient::Trace(visitor);
}

}  // namespace blink
