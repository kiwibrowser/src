// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_STRUCT_TRAITS_H_
#define THIRD_PARTY_BLINK_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_STRUCT_TRAITS_H_

#include "skia/public/interfaces/bitmap_skbitmap_struct_traits.h"
#include "third_party/blink/common/message_port/cloneable_message_struct_traits.h"
#include "third_party/blink/public/common/message_port/transferable_message.h"
#include "third_party/blink/public/mojom/message_port/message_port.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace mojo {

template <>
struct BLINK_COMMON_EXPORT
    StructTraits<blink::mojom::TransferableMessage::DataView,
                 blink::TransferableMessage> {
  static blink::CloneableMessage& message(blink::TransferableMessage& input) {
    return input;
  }

  static std::vector<mojo::ScopedMessagePipeHandle> ports(
      blink::TransferableMessage& input) {
    return blink::MessagePortChannel::ReleaseHandles(input.ports);
  }

  static std::vector<blink::mojom::SerializedArrayBufferContentsPtr>
  array_buffer_contents_array(blink::TransferableMessage& input) {
    return std::move(input.array_buffer_contents_array);
  }

  static const std::vector<SkBitmap>& image_bitmap_contents_array(
      blink::TransferableMessage& input) {
    return input.image_bitmap_contents_array;
  }

  static bool has_user_gesture(blink::TransferableMessage& input) {
    return input.has_user_gesture;
  }

  static bool Read(blink::mojom::TransferableMessage::DataView data,
                   blink::TransferableMessage* out);
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_STRUCT_TRAITS_H_
