// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/message_port/cloneable_message.h"

#include "third_party/blink/public/mojom/blob/blob.mojom.h"
#include "third_party/blink/public/mojom/message_port/message_port.mojom.h"

namespace blink {

CloneableMessage::CloneableMessage() = default;
CloneableMessage::CloneableMessage(CloneableMessage&&) = default;
CloneableMessage& CloneableMessage::operator=(CloneableMessage&&) = default;
CloneableMessage::~CloneableMessage() = default;

CloneableMessage CloneableMessage::ShallowClone() const {
  CloneableMessage clone;
  clone.encoded_message = encoded_message;
  for (const auto& blob : blobs) {
    // NOTE: We dubiously exercise dual ownership of the blob's pipe handle here
    // so that we can temporarily bind and send a message over the pipe without
    // mutating the state of this CloneableMessage.
    mojo::ScopedMessagePipeHandle handle(blob->blob.handle().get());
    mojom::BlobPtr blob_proxy(
        mojom::BlobPtrInfo(std::move(handle), blob->blob.version()));
    mojom::BlobPtrInfo blob_clone_info;
    blob_proxy->Clone(MakeRequest(&blob_clone_info));
    clone.blobs.push_back(
        mojom::SerializedBlob::New(blob->uuid, blob->content_type, blob->size,
                                   std::move(blob_clone_info)));

    // Not leaked - still owned by |blob->blob|.
    ignore_result(blob_proxy.PassInterface().PassHandle().release());
  }
  return clone;
}

void CloneableMessage::EnsureDataIsOwned() {
  if (encoded_message.data() == owned_encoded_message.data())
    return;
  owned_encoded_message.assign(encoded_message.begin(), encoded_message.end());
  encoded_message = owned_encoded_message;
}

}  // namespace blink
