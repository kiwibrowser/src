// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/messaging/blink_transferable_message.h"

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"

namespace blink {

BlinkTransferableMessage::BlinkTransferableMessage() = default;
BlinkTransferableMessage::~BlinkTransferableMessage() = default;

BlinkTransferableMessage::BlinkTransferableMessage(BlinkTransferableMessage&&) =
    default;
BlinkTransferableMessage& BlinkTransferableMessage::operator=(
    BlinkTransferableMessage&&) = default;

BlinkTransferableMessage ToBlinkTransferableMessage(
    TransferableMessage message) {
  BlinkTransferableMessage result;
  result.message = SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message.encoded_message.data()),
      message.encoded_message.size());
  for (auto& blob : message.blobs) {
    result.message->BlobDataHandles().Set(
        WebString::FromUTF8(blob->uuid),
        BlobDataHandle::Create(
            WebString::FromUTF8(blob->uuid),
            WebString::FromUTF8(blob->content_type), blob->size,
            mojom::blink::BlobPtrInfo(blob->blob.PassHandle(),
                                      mojom::Blob::Version_)));
  }
  result.sender_stack_trace_id = v8_inspector::V8StackTraceId(
      message.stack_trace_id,
      std::make_pair(message.stack_trace_debugger_id_first,
                     message.stack_trace_debugger_id_second));
  result.ports.AppendRange(message.ports.begin(), message.ports.end());
  result.has_user_gesture = message.has_user_gesture;
  return result;
}

TransferableMessage ToTransferableMessage(BlinkTransferableMessage message) {
  TransferableMessage result;
  result.encoded_message = message.message->GetWireData();
  result.blobs.reserve(message.message->BlobDataHandles().size());
  for (const auto& blob : message.message->BlobDataHandles()) {
    result.blobs.push_back(mojom::SerializedBlob::New(
        WebString(blob.value->Uuid()).Utf8(),
        WebString(blob.value->GetType()).Utf8(), blob.value->size(),
        mojom::BlobPtrInfo(
            blob.value->CloneBlobPtr().PassInterface().PassHandle(),
            mojom::Blob::Version_)));
  }
  result.stack_trace_id = message.sender_stack_trace_id.id;
  result.stack_trace_debugger_id_first =
      message.sender_stack_trace_id.debugger_id.first;
  result.stack_trace_debugger_id_second =
      message.sender_stack_trace_id.debugger_id.second;
  result.ports.assign(message.ports.begin(), message.ports.end());
  result.has_user_gesture = message.has_user_gesture;
  return result;
}

}  // namespace blink
