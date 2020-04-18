// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_H_

#include <vector>

#include "base/containers/span.h"
#include "base/macros.h"
#include "third_party/blink/common/common_export.h"
#include "third_party/blink/public/common/message_port/cloneable_message.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"
#include "third_party/blink/public/mojom/array_buffer/array_buffer_contents.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

// This struct represents messages as they are posted over a message port. This
// type can be serialized as a blink::mojom::TransferableMessage struct.
struct BLINK_COMMON_EXPORT TransferableMessage : public CloneableMessage {
  TransferableMessage();
  TransferableMessage(TransferableMessage&&);
  TransferableMessage& operator=(TransferableMessage&&);
  ~TransferableMessage();

  // Any ports being transfered as part of this message.
  std::vector<MessagePortChannel> ports;
  // The contents of any ArrayBuffers being transfered as part of this message.
  std::vector<mojom::SerializedArrayBufferContentsPtr>
      array_buffer_contents_array;
  // The contents of any ImageBitmaps being transfered as part of this message.
  std::vector<SkBitmap> image_bitmap_contents_array;

  // Whether the recipient should have a user gesture when it processes this
  // message.
  bool has_user_gesture = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(TransferableMessage);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_MESSAGE_PORT_TRANSFERABLE_MESSAGE_H_
