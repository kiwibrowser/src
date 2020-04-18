// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/serialization/v8_script_value_serializer.h"

#include "base/auto_reset.h"
#include "third_party/blink/public/platform/web_blob_info.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_blob.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_matrix.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_matrix_read_only.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_point.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_point_read_only.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_quad.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_rect.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_dom_rect_read_only.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_file.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_file_list.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_image_bitmap.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_image_data.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_message_port.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_offscreen_canvas.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_shared_array_buffer.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_throw_dom_exception.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix_read_only.h"
#include "third_party/blink/renderer/core/geometry/dom_point.h"
#include "third_party/blink/renderer/core/geometry/dom_point_read_only.h"
#include "third_party/blink/renderer/core/geometry/dom_quad.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/geometry/dom_rect_read_only.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_base.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace blink {

// The "Blink-side" serialization version, which defines how Blink will behave
// during the serialization process, is in
// SerializedScriptValue::wireFormatVersion. The serialization format has two
// "envelopes": an outer one controlled by Blink and an inner one by V8.
//
// They are formatted as follows:
// [version tag] [Blink version] [version tag] [v8 version] ...
//
// Before version 16, there was only a single envelope and the version number
// for both parts was always equal.
//
// See also V8ScriptValueDeserializer.cpp.
//
// This version number must be incremented whenever any incompatible changes are
// made to how Blink writes data. Purely V8-side changes do not require an
// adjustment to this value.

V8ScriptValueSerializer::V8ScriptValueSerializer(
    scoped_refptr<ScriptState> script_state,
    const Options& options)
    : script_state_(std::move(script_state)),
      serialized_script_value_(SerializedScriptValue::Create()),
      serializer_(script_state_->GetIsolate(), this),
      transferables_(options.transferables),
      blob_info_array_(options.blob_info),
      wasm_policy_(options.wasm_policy),
      for_storage_(options.for_storage == SerializedScriptValue::kForStorage) {}

scoped_refptr<SerializedScriptValue> V8ScriptValueSerializer::Serialize(
    v8::Local<v8::Value> value,
    ExceptionState& exception_state) {
#if DCHECK_IS_ON()
  DCHECK(!serialize_invoked_);
  serialize_invoked_ = true;
#endif
  DCHECK(serialized_script_value_);
  base::AutoReset<const ExceptionState*> reset(&exception_state_,
                                               &exception_state);

  // Prepare to transfer the provided transferables.
  PrepareTransfer(exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Write out the file header.
  WriteTag(kVersionTag);
  WriteUint32(SerializedScriptValue::kWireFormatVersion);
  serializer_.WriteHeader();

  // Serialize the value and handle errors.
  v8::TryCatch try_catch(script_state_->GetIsolate());
  bool wrote_value;
  if (!serializer_.WriteValue(script_state_->GetContext(), value)
           .To(&wrote_value)) {
    DCHECK(try_catch.HasCaught());
    exception_state.RethrowV8Exception(try_catch.Exception());
    return nullptr;
  }
  DCHECK(wrote_value);

  // Finalize the transfer (e.g. neutering array buffers).
  FinalizeTransfer(exception_state);
  if (exception_state.HadException())
    return nullptr;

  serialized_script_value_->CloneSharedArrayBuffers(shared_array_buffers_);

  // Finalize the results.
  std::pair<uint8_t*, size_t> buffer = serializer_.Release();
  serialized_script_value_->SetData(
      SerializedScriptValue::DataBufferPtr(buffer.first), buffer.second);
  return std::move(serialized_script_value_);
}

void V8ScriptValueSerializer::PrepareTransfer(ExceptionState& exception_state) {
  if (!transferables_)
    return;

  // Transfer array buffers.
  for (uint32_t i = 0; i < transferables_->array_buffers.size(); i++) {
    DOMArrayBufferBase* array_buffer = transferables_->array_buffers[i].Get();
    if (!array_buffer->IsShared()) {
      v8::Local<v8::Value> wrapper = ToV8(array_buffer, script_state_.get());
      serializer_.TransferArrayBuffer(
          i, v8::Local<v8::ArrayBuffer>::Cast(wrapper));
    } else {
      exception_state.ThrowDOMException(
          kDataCloneError, "SharedArrayBuffer can not be in transfer list.");
      return;
    }
  }
}

void V8ScriptValueSerializer::FinalizeTransfer(
    ExceptionState& exception_state) {
  // TODO(jbroman): Strictly speaking, this is not correct; transfer should
  // occur in the order of the transfer list.
  // https://html.spec.whatwg.org/multipage/infrastructure.html#structuredclonewithtransfer

  v8::Isolate* isolate = script_state_->GetIsolate();

  ArrayBufferArray array_buffers;
  if (transferables_)
    array_buffers.AppendVector(transferables_->array_buffers);

  if (!array_buffers.IsEmpty()) {
    serialized_script_value_->TransferArrayBuffers(isolate, array_buffers,
                                                   exception_state);
    if (exception_state.HadException())
      return;
  }

  if (transferables_) {
    serialized_script_value_->TransferImageBitmaps(
        isolate, transferables_->image_bitmaps, exception_state);
    if (exception_state.HadException())
      return;

    serialized_script_value_->TransferOffscreenCanvas(
        isolate, transferables_->offscreen_canvases, exception_state);
    if (exception_state.HadException())
      return;
  }
}

void V8ScriptValueSerializer::WriteUTF8String(const String& string) {
  // TODO(jbroman): Ideally this method would take a WTF::StringView, but the
  // StringUTF8Adaptor trick doesn't yet work with StringView.
  StringUTF8Adaptor utf8(string);
  DCHECK_LT(utf8.length(), std::numeric_limits<uint32_t>::max());
  WriteUint32(utf8.length());
  WriteRawBytes(utf8.Data(), utf8.length());
}

bool V8ScriptValueSerializer::WriteDOMObject(ScriptWrappable* wrappable,
                                             ExceptionState& exception_state) {
  const WrapperTypeInfo* wrapper_type_info = wrappable->GetWrapperTypeInfo();
  if (wrapper_type_info == &V8Blob::wrapperTypeInfo) {
    Blob* blob = wrappable->ToImpl<Blob>();
    serialized_script_value_->BlobDataHandles().Set(blob->Uuid(),
                                                    blob->GetBlobDataHandle());
    if (blob_info_array_) {
      size_t index = blob_info_array_->size();
      DCHECK_LE(index, std::numeric_limits<uint32_t>::max());
      blob_info_array_->emplace_back(blob->GetBlobDataHandle(), blob->type(),
                                     blob->size());
      WriteTag(kBlobIndexTag);
      WriteUint32(static_cast<uint32_t>(index));
    } else {
      WriteTag(kBlobTag);
      WriteUTF8String(blob->Uuid());
      WriteUTF8String(blob->type());
      WriteUint64(blob->size());
    }
    return true;
  }
  if (wrapper_type_info == &V8File::wrapperTypeInfo) {
    WriteTag(blob_info_array_ ? kFileIndexTag : kFileTag);
    return WriteFile(wrappable->ToImpl<File>(), exception_state);
  }
  if (wrapper_type_info == &V8FileList::wrapperTypeInfo) {
    // This does not presently deduplicate a File object and its entry in a
    // FileList, which is non-standard behavior.
    FileList* file_list = wrappable->ToImpl<FileList>();
    unsigned length = file_list->length();
    WriteTag(blob_info_array_ ? kFileListIndexTag : kFileListTag);
    WriteUint32(length);
    for (unsigned i = 0; i < length; i++) {
      if (!WriteFile(file_list->item(i), exception_state))
        return false;
    }
    return true;
  }
  if (wrapper_type_info == &V8ImageBitmap::wrapperTypeInfo) {
    ImageBitmap* image_bitmap = wrappable->ToImpl<ImageBitmap>();
    if (image_bitmap->IsNeutered()) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "An ImageBitmap is detached and could not be cloned.");
      return false;
    }

    // If this ImageBitmap was transferred, it can be serialized by index.
    size_t index = kNotFound;
    if (transferables_)
      index = transferables_->image_bitmaps.Find(image_bitmap);
    if (index != kNotFound) {
      DCHECK_LE(index, std::numeric_limits<uint32_t>::max());
      WriteTag(kImageBitmapTransferTag);
      WriteUint32(static_cast<uint32_t>(index));
      return true;
    }

    // Otherwise, it must be fully serialized.
    WriteTag(kImageBitmapTag);
    SerializedColorParams color_params(image_bitmap->GetCanvasColorParams());
    WriteUint32Enum(ImageSerializationTag::kCanvasColorSpaceTag);
    WriteUint32Enum(color_params.GetSerializedColorSpace());
    WriteUint32Enum(ImageSerializationTag::kCanvasPixelFormatTag);
    WriteUint32Enum(color_params.GetSerializedPixelFormat());
    WriteUint32Enum(ImageSerializationTag::kCanvasOpacityModeTag);
    WriteUint32Enum(color_params.GetSerializedOpacityMode());
    WriteUint32Enum(ImageSerializationTag::kOriginCleanTag);
    WriteUint32(image_bitmap->OriginClean());
    WriteUint32Enum(ImageSerializationTag::kIsPremultipliedTag);
    WriteUint32(image_bitmap->IsPremultiplied());
    WriteUint32Enum(ImageSerializationTag::kEndTag);
    WriteUint32(image_bitmap->width());
    WriteUint32(image_bitmap->height());
    scoped_refptr<Uint8Array> pixels = image_bitmap->CopyBitmapData();
    WriteUint32(pixels->length());
    WriteRawBytes(pixels->Data(), pixels->length());
    return true;
  }
  if (wrapper_type_info == &V8ImageData::wrapperTypeInfo) {
    ImageData* image_data = wrappable->ToImpl<ImageData>();
    WriteTag(kImageDataTag);
    SerializedColorParams color_params(image_data->GetCanvasColorParams(),
                                       image_data->GetImageDataStorageFormat());
    WriteUint32Enum(ImageSerializationTag::kCanvasColorSpaceTag);
    WriteUint32Enum(color_params.GetSerializedColorSpace());
    WriteUint32Enum(ImageSerializationTag::kImageDataStorageFormatTag);
    WriteUint32Enum(color_params.GetSerializedImageDataStorageFormat());
    WriteUint32Enum(ImageSerializationTag::kEndTag);
    WriteUint32(image_data->width());
    WriteUint32(image_data->height());
    DOMArrayBufferBase* pixel_buffer = image_data->BufferBase();
    WriteUint32(pixel_buffer->ByteLength());
    WriteRawBytes(pixel_buffer->Data(), pixel_buffer->ByteLength());
    return true;
  }
  if (wrapper_type_info == &V8DOMPoint::wrapperTypeInfo) {
    DOMPoint* point = wrappable->ToImpl<DOMPoint>();
    WriteTag(kDOMPointTag);
    WriteDouble(point->x());
    WriteDouble(point->y());
    WriteDouble(point->z());
    WriteDouble(point->w());
    return true;
  }
  if (wrapper_type_info == &V8DOMPointReadOnly::wrapperTypeInfo) {
    DOMPointReadOnly* point = wrappable->ToImpl<DOMPointReadOnly>();
    WriteTag(kDOMPointReadOnlyTag);
    WriteDouble(point->x());
    WriteDouble(point->y());
    WriteDouble(point->z());
    WriteDouble(point->w());
    return true;
  }
  if (wrapper_type_info == &V8DOMRect::wrapperTypeInfo) {
    DOMRect* rect = wrappable->ToImpl<DOMRect>();
    WriteTag(kDOMRectTag);
    WriteDouble(rect->x());
    WriteDouble(rect->y());
    WriteDouble(rect->width());
    WriteDouble(rect->height());
    return true;
  }
  if (wrapper_type_info == &V8DOMRectReadOnly::wrapperTypeInfo) {
    DOMRectReadOnly* rect = wrappable->ToImpl<DOMRectReadOnly>();
    WriteTag(kDOMRectReadOnlyTag);
    WriteDouble(rect->x());
    WriteDouble(rect->y());
    WriteDouble(rect->width());
    WriteDouble(rect->height());
    return true;
  }
  if (wrapper_type_info == &V8DOMQuad::wrapperTypeInfo) {
    DOMQuad* quad = wrappable->ToImpl<DOMQuad>();
    WriteTag(kDOMQuadTag);
    for (const DOMPoint* point :
         {quad->p1(), quad->p2(), quad->p3(), quad->p4()}) {
      WriteDouble(point->x());
      WriteDouble(point->y());
      WriteDouble(point->z());
      WriteDouble(point->w());
    }
    return true;
  }
  if (wrapper_type_info == &V8DOMMatrix::wrapperTypeInfo) {
    DOMMatrix* matrix = wrappable->ToImpl<DOMMatrix>();
    if (matrix->is2D()) {
      WriteTag(kDOMMatrix2DTag);
      WriteDouble(matrix->a());
      WriteDouble(matrix->b());
      WriteDouble(matrix->c());
      WriteDouble(matrix->d());
      WriteDouble(matrix->e());
      WriteDouble(matrix->f());
    } else {
      WriteTag(kDOMMatrixTag);
      WriteDouble(matrix->m11());
      WriteDouble(matrix->m12());
      WriteDouble(matrix->m13());
      WriteDouble(matrix->m14());
      WriteDouble(matrix->m21());
      WriteDouble(matrix->m22());
      WriteDouble(matrix->m23());
      WriteDouble(matrix->m24());
      WriteDouble(matrix->m31());
      WriteDouble(matrix->m32());
      WriteDouble(matrix->m33());
      WriteDouble(matrix->m34());
      WriteDouble(matrix->m41());
      WriteDouble(matrix->m42());
      WriteDouble(matrix->m43());
      WriteDouble(matrix->m44());
    }
    return true;
  }
  if (wrapper_type_info == &V8DOMMatrixReadOnly::wrapperTypeInfo) {
    DOMMatrixReadOnly* matrix = wrappable->ToImpl<DOMMatrixReadOnly>();
    if (matrix->is2D()) {
      WriteTag(kDOMMatrix2DReadOnlyTag);
      WriteDouble(matrix->a());
      WriteDouble(matrix->b());
      WriteDouble(matrix->c());
      WriteDouble(matrix->d());
      WriteDouble(matrix->e());
      WriteDouble(matrix->f());
    } else {
      WriteTag(kDOMMatrixReadOnlyTag);
      WriteDouble(matrix->m11());
      WriteDouble(matrix->m12());
      WriteDouble(matrix->m13());
      WriteDouble(matrix->m14());
      WriteDouble(matrix->m21());
      WriteDouble(matrix->m22());
      WriteDouble(matrix->m23());
      WriteDouble(matrix->m24());
      WriteDouble(matrix->m31());
      WriteDouble(matrix->m32());
      WriteDouble(matrix->m33());
      WriteDouble(matrix->m34());
      WriteDouble(matrix->m41());
      WriteDouble(matrix->m42());
      WriteDouble(matrix->m43());
      WriteDouble(matrix->m44());
    }
    return true;
  }
  if (wrapper_type_info == &V8MessagePort::wrapperTypeInfo) {
    MessagePort* message_port = wrappable->ToImpl<MessagePort>();
    size_t index = kNotFound;
    if (transferables_)
      index = transferables_->message_ports.Find(message_port);
    if (index == kNotFound) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "A MessagePort could not be cloned because it was not transferred.");
      return false;
    }
    DCHECK_LE(index, std::numeric_limits<uint32_t>::max());
    WriteTag(kMessagePortTag);
    WriteUint32(static_cast<uint32_t>(index));
    return true;
  }
  if (wrapper_type_info == &V8OffscreenCanvas::wrapperTypeInfo) {
    OffscreenCanvas* canvas = wrappable->ToImpl<OffscreenCanvas>();
    size_t index = kNotFound;
    if (transferables_)
      index = transferables_->offscreen_canvases.Find(canvas);
    if (index == kNotFound) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "An OffscreenCanvas could not be cloned "
          "because it was not transferred.");
      return false;
    }
    if (canvas->IsNeutered()) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "An OffscreenCanvas could not be cloned because it was detached.");
      return false;
    }
    if (canvas->RenderingContext()) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "An OffscreenCanvas could not be cloned "
          "because it had a rendering context.");
      return false;
    }
    WriteTag(kOffscreenCanvasTransferTag);
    WriteUint32(canvas->width());
    WriteUint32(canvas->height());
    WriteUint32(canvas->PlaceholderCanvasId());
    WriteUint32(canvas->ClientId());
    WriteUint32(canvas->SinkId());
    return true;
  }
  return false;
}

bool V8ScriptValueSerializer::WriteFile(File* file,
                                        ExceptionState& exception_state) {
  serialized_script_value_->BlobDataHandles().Set(file->Uuid(),
                                                  file->GetBlobDataHandle());
  if (blob_info_array_) {
    size_t index = blob_info_array_->size();
    DCHECK_LE(index, std::numeric_limits<uint32_t>::max());
    long long size = -1;
    double last_modified_ms = InvalidFileTime();
    file->CaptureSnapshot(size, last_modified_ms);
    // FIXME: transition WebBlobInfo.lastModified to be milliseconds-based also.
    double last_modified = last_modified_ms / kMsPerSecond;
    blob_info_array_->emplace_back(file->GetBlobDataHandle(), file->GetPath(),
                                   file->name(), file->type(), last_modified,
                                   size);
    WriteUint32(static_cast<uint32_t>(index));
  } else {
    WriteUTF8String(file->HasBackingFile() ? file->GetPath() : g_empty_string);
    WriteUTF8String(file->name());
    WriteUTF8String(file->webkitRelativePath());
    WriteUTF8String(file->Uuid());
    WriteUTF8String(file->type());
    // TODO(jsbell): metadata is unconditionally captured in the index case.
    // Why this inconsistency?
    if (file->HasValidSnapshotMetadata()) {
      WriteUint32(1);
      long long size;
      double last_modified_ms;
      file->CaptureSnapshot(size, last_modified_ms);
      DCHECK_GE(size, 0);
      WriteUint64(static_cast<uint64_t>(size));
      WriteDouble(last_modified_ms);
    } else {
      WriteUint32(0);
    }
    WriteUint32(file->GetUserVisibility() == File::kIsUserVisible ? 1 : 0);
  }
  return true;
}

void V8ScriptValueSerializer::ThrowDataCloneError(
    v8::Local<v8::String> v8_message) {
  DCHECK(exception_state_);
  String message = exception_state_->AddExceptionContext(
      V8StringToWebCoreString<String>(v8_message, kDoNotExternalize));
  V8ThrowDOMException::ThrowDOMException(script_state_->GetIsolate(),
                                         kDataCloneError, message);
}

v8::Maybe<bool> V8ScriptValueSerializer::WriteHostObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object) {
  DCHECK(exception_state_);
  DCHECK_EQ(isolate, script_state_->GetIsolate());
  ExceptionState exception_state(isolate, exception_state_->Context(),
                                 exception_state_->InterfaceName(),
                                 exception_state_->PropertyName());

  if (!V8DOMWrapper::IsWrapper(isolate, object)) {
    exception_state.ThrowDOMException(kDataCloneError,
                                      "An object could not be cloned.");
    return v8::Nothing<bool>();
  }
  ScriptWrappable* wrappable = ToScriptWrappable(object);
  bool wrote_dom_object = WriteDOMObject(wrappable, exception_state);
  if (wrote_dom_object) {
    DCHECK(!exception_state.HadException());
    return v8::Just(true);
  }
  if (!exception_state.HadException()) {
    StringView interface = wrappable->GetWrapperTypeInfo()->interface_name;
    exception_state.ThrowDOMException(
        kDataCloneError, interface + " object could not be cloned.");
  }
  return v8::Nothing<bool>();
}

v8::Maybe<uint32_t> V8ScriptValueSerializer::GetSharedArrayBufferId(
    v8::Isolate* isolate,
    v8::Local<v8::SharedArrayBuffer> v8_shared_array_buffer) {
  if (for_storage_) {
    DCHECK(exception_state_);
    DCHECK_EQ(isolate, script_state_->GetIsolate());
    ExceptionState exception_state(isolate, exception_state_->Context(),
                                   exception_state_->InterfaceName(),
                                   exception_state_->PropertyName());
    exception_state.ThrowDOMException(
        kDataCloneError,
        "A SharedArrayBuffer can not be serialized for storage.");
    return v8::Nothing<uint32_t>();
  }

  DOMSharedArrayBuffer* shared_array_buffer =
      V8SharedArrayBuffer::ToImpl(v8_shared_array_buffer);

  // The index returned from this function will be serialized into the data
  // stream. When deserializing, this will be used to index into the
  // sharedArrayBufferContents array of the SerializedScriptValue.
  size_t index = shared_array_buffers_.Find(shared_array_buffer);
  if (index == kNotFound) {
    shared_array_buffers_.push_back(shared_array_buffer);
    index = shared_array_buffers_.size() - 1;
  }
  return v8::Just<uint32_t>(index);
}

v8::Maybe<uint32_t> V8ScriptValueSerializer::GetWasmModuleTransferId(
    v8::Isolate* isolate,
    v8::Local<v8::WasmCompiledModule> module) {
  switch (wasm_policy_) {
    case Options::kSerialize:
      return v8::Nothing<uint32_t>();

    case Options::kBlockedInNonSecureContext: {
      // This happens, currently, when we try to serialize to IndexedDB
      // in an non-secure context.
      ExceptionState exception_state(isolate, exception_state_->Context(),
                                     exception_state_->InterfaceName(),
                                     exception_state_->PropertyName());
      exception_state.ThrowDOMException(kDataCloneError,
                                        "Serializing WebAssembly modules in "
                                        "non-secure contexts is not allowed.");
      return v8::Nothing<uint32_t>();
    }

    case Options::kTransfer: {
      // We don't expect scenarios with numerous wasm modules being transferred
      // around. Most likely, we'll have one module. The vector approach is
      // simple and should perform sufficiently well under these expectations.
      serialized_script_value_->WasmModules().push_back(
          module->GetTransferrableModule());
      uint32_t size =
          static_cast<uint32_t>(serialized_script_value_->WasmModules().size());
      DCHECK_GE(size, 1u);
      return v8::Just(size - 1);
    }

    case Options::kUnspecified:
      NOTREACHED();
  }
  return v8::Nothing<uint32_t>();
}

void* V8ScriptValueSerializer::ReallocateBufferMemory(void* old_buffer,
                                                      size_t size,
                                                      size_t* actual_size) {
  *actual_size = WTF::Partitions::BufferActualSize(size);
  return WTF::Partitions::BufferRealloc(old_buffer, *actual_size,
                                        "SerializedScriptValue buffer");
}

void V8ScriptValueSerializer::FreeBufferMemory(void* buffer) {
  return WTF::Partitions::BufferFree(buffer);
}

}  // namespace blink
