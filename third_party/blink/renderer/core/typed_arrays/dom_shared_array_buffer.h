// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_SHARED_ARRAY_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_SHARED_ARRAY_BUFFER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_base.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer.h"

namespace blink {

class CORE_EXPORT DOMSharedArrayBuffer final : public DOMArrayBufferBase {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static DOMSharedArrayBuffer* Create(scoped_refptr<WTF::ArrayBuffer> buffer) {
    DCHECK(buffer->IsShared());
    return new DOMSharedArrayBuffer(std::move(buffer));
  }
  static DOMSharedArrayBuffer* Create(unsigned num_elements,
                                      unsigned element_byte_size) {
    return Create(
        WTF::ArrayBuffer::CreateShared(num_elements, element_byte_size));
  }
  static DOMSharedArrayBuffer* Create(const void* source,
                                      unsigned byte_length) {
    return Create(WTF::ArrayBuffer::CreateShared(source, byte_length));
  }
  static DOMSharedArrayBuffer* Create(WTF::ArrayBufferContents& contents) {
    DCHECK(contents.IsShared());
    return Create(WTF::ArrayBuffer::Create(contents));
  }

  bool ShareContentsWith(WTF::ArrayBufferContents& result) {
    return Buffer()->ShareContentsWith(result);
  }

  v8::Local<v8::Object> Wrap(v8::Isolate*,
                             v8::Local<v8::Object> creation_context) override;

 private:
  explicit DOMSharedArrayBuffer(scoped_refptr<WTF::ArrayBuffer> buffer)
      : DOMArrayBufferBase(std::move(buffer)) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_SHARED_ARRAY_BUFFER_H_
