// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_base.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer.h"

namespace blink {

class SharedBuffer;

class CORE_EXPORT DOMArrayBuffer final : public DOMArrayBufferBase {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static DOMArrayBuffer* Create(scoped_refptr<WTF::ArrayBuffer> buffer) {
    return new DOMArrayBuffer(std::move(buffer));
  }
  static DOMArrayBuffer* Create(unsigned num_elements,
                                unsigned element_byte_size) {
    return Create(WTF::ArrayBuffer::Create(num_elements, element_byte_size));
  }
  static DOMArrayBuffer* Create(const void* source, unsigned byte_length) {
    return Create(WTF::ArrayBuffer::Create(source, byte_length));
  }
  static DOMArrayBuffer* Create(WTF::ArrayBufferContents& contents) {
    return Create(WTF::ArrayBuffer::Create(contents));
  }
  static DOMArrayBuffer* Create(scoped_refptr<SharedBuffer>);

  // Only for use by XMLHttpRequest::responseArrayBuffer and
  // Internals::serializeObject.
  static DOMArrayBuffer* CreateUninitializedOrNull(unsigned num_elements,
                                                   unsigned element_byte_size);

  DOMArrayBuffer* Slice(int begin, int end) const {
    return Create(Buffer()->Slice(begin, end));
  }
  DOMArrayBuffer* Slice(int begin) const {
    return Create(Buffer()->Slice(begin));
  }

  bool IsNeuterable(v8::Isolate*);

  // Transfer the ArrayBuffer if it is neuterable, otherwise make a copy and
  // transfer that.
  bool Transfer(v8::Isolate*, WTF::ArrayBufferContents& result);

  v8::Local<v8::Object> Wrap(v8::Isolate*,
                             v8::Local<v8::Object> creation_context) override;

 private:
  explicit DOMArrayBuffer(scoped_refptr<WTF::ArrayBuffer> buffer)
      : DOMArrayBufferBase(std::move(buffer)) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_
