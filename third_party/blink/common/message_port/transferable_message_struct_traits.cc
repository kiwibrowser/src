// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/common/message_port/transferable_message_struct_traits.h"

#include "base/containers/span.h"
#include "third_party/blink/common/message_port/cloneable_message_struct_traits.h"

namespace mojo {

bool StructTraits<blink::mojom::TransferableMessage::DataView,
                  blink::TransferableMessage>::
    Read(blink::mojom::TransferableMessage::DataView data,
         blink::TransferableMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  if (!data.ReadMessage(static_cast<blink::CloneableMessage*>(out)) ||
      !data.ReadArrayBufferContentsArray(&out->array_buffer_contents_array) ||
      !data.ReadImageBitmapContentsArray(&out->image_bitmap_contents_array) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports = blink::MessagePortChannel::CreateFromHandles(std::move(ports));
  out->has_user_gesture = data.has_user_gesture();
  return true;
}

}  // namespace mojo
