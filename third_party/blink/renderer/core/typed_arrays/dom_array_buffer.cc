// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/bindings/dom_data_store.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

static void AccumulateArrayBuffersForAllWorlds(
    v8::Isolate* isolate,
    DOMArrayBuffer* object,
    Vector<v8::Local<v8::ArrayBuffer>, 4>& buffers) {
  Vector<scoped_refptr<DOMWrapperWorld>> worlds;
  DOMWrapperWorld::AllWorldsInCurrentThread(worlds);
  for (const auto& world : worlds) {
    v8::Local<v8::Object> wrapper = world->DomDataStore().Get(object, isolate);
    if (!wrapper.IsEmpty())
      buffers.push_back(v8::Local<v8::ArrayBuffer>::Cast(wrapper));
  }
}

bool DOMArrayBuffer::IsNeuterable(v8::Isolate* isolate) {
  Vector<v8::Local<v8::ArrayBuffer>, 4> buffer_handles;
  v8::HandleScope handle_scope(isolate);
  AccumulateArrayBuffersForAllWorlds(isolate, this, buffer_handles);

  bool is_neuterable = true;
  for (const auto& buffer_handle : buffer_handles)
    is_neuterable &= buffer_handle->IsNeuterable();

  return is_neuterable;
}

bool DOMArrayBuffer::Transfer(v8::Isolate* isolate,
                              WTF::ArrayBufferContents& result) {
  DOMArrayBuffer* to_transfer = this;
  if (!IsNeuterable(isolate)) {
    to_transfer =
        DOMArrayBuffer::Create(Buffer()->Data(), Buffer()->ByteLength());
  }

  if (!to_transfer->Buffer()->Transfer(result))
    return false;

  Vector<v8::Local<v8::ArrayBuffer>, 4> buffer_handles;
  v8::HandleScope handle_scope(isolate);
  AccumulateArrayBuffersForAllWorlds(isolate, to_transfer, buffer_handles);

  for (const auto& buffer_handle : buffer_handles)
    buffer_handle->Neuter();

  return true;
}

DOMArrayBuffer* DOMArrayBuffer::CreateUninitializedOrNull(
    unsigned num_elements,
    unsigned element_byte_size) {
  scoped_refptr<ArrayBuffer> buffer =
      WTF::ArrayBuffer::CreateUninitializedOrNull(num_elements,
                                                  element_byte_size);
  if (!buffer)
    return nullptr;
  return Create(std::move(buffer));
}

v8::Local<v8::Object> DOMArrayBuffer::Wrap(
    v8::Isolate* isolate,
    v8::Local<v8::Object> creation_context) {
  DCHECK(!DOMDataStore::ContainsWrapper(this, isolate));

  const WrapperTypeInfo* wrapper_type_info = this->GetWrapperTypeInfo();
  v8::Local<v8::Object> wrapper =
      v8::ArrayBuffer::New(isolate, Data(), ByteLength());

  return AssociateWithWrapper(isolate, wrapper_type_info, wrapper);
}

DOMArrayBuffer* DOMArrayBuffer::Create(
    scoped_refptr<SharedBuffer> shared_buffer) {
  WTF::ArrayBufferContents contents(shared_buffer->size(), 1,
                                    WTF::ArrayBufferContents::kNotShared,
                                    WTF::ArrayBufferContents::kDontInitialize);
  uint8_t* data = static_cast<uint8_t*>(contents.Data());
  if (UNLIKELY(!data))
    OOM_CRASH();

  shared_buffer->ForEachSegment([&data](const char* segment,
                                        size_t segment_size,
                                        size_t segment_offset) -> bool {
    memcpy(data + segment_offset, segment, segment_size);
    return true;
  });

  return Create(ArrayBuffer::Create(contents));
}

}  // namespace blink
