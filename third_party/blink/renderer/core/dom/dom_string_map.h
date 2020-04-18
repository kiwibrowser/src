/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_STRING_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_STRING_MAP_H_

#include "base/macros.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class DOMStringMap : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual void GetNames(Vector<String>&) = 0;
  virtual String item(const String& name) = 0;
  virtual bool Contains(const String& name) = 0;
  virtual void SetItem(const String& name,
                       const String& value,
                       ExceptionState&) = 0;
  virtual bool DeleteItem(const String& name) = 0;
  bool AnonymousNamedSetter(const String& name,
                            const String& value,
                            ExceptionState& exception_state) {
    SetItem(name, value, exception_state);
    return true;
  }
  DeleteResult AnonymousNamedDeleter(const AtomicString& name) {
    bool known_property = DeleteItem(name);
    return known_property ? kDeleteSuccess : kDeleteUnknownProperty;
  }
  void NamedPropertyEnumerator(Vector<String>& names, ExceptionState&) {
    GetNames(names);
  }
  bool NamedPropertyQuery(const AtomicString&, ExceptionState&);

 protected:
  DOMStringMap() = default;
  DISALLOW_COPY_AND_ASSIGN(DOMStringMap);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_STRING_MAP_H_
