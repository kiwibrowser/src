// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SERIALIZATION_TRANSFERABLES_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SERIALIZATION_TRANSFERABLES_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class DOMArrayBufferBase;
class ImageBitmap;
class OffscreenCanvas;
class MessagePort;

using ArrayBufferArray = HeapVector<Member<DOMArrayBufferBase>>;
using ImageBitmapArray = HeapVector<Member<ImageBitmap>>;
using OffscreenCanvasArray = HeapVector<Member<OffscreenCanvas>>;
using MessagePortArray = HeapVector<Member<MessagePort>>;

class CORE_EXPORT Transferables final {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(Transferables);

 public:
  Transferables() = default;

  ArrayBufferArray array_buffers;
  ImageBitmapArray image_bitmaps;
  OffscreenCanvasArray offscreen_canvases;
  MessagePortArray message_ports;
};

// Along with extending |Transferables| to hold a new kind of transferable
// objects, serialization handling code changes are required:
//   - extend ScriptValueSerializer::copyTransferables()
//   - alter SerializedScriptValue(Factory) to do the actual transfer.

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SERIALIZATION_TRANSFERABLES_H_
