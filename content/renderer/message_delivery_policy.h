// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MESSAGE_DELIVERY_POLICY_H_
#define CONTENT_RENDERER_MESSAGE_DELIVERY_POLICY_H_

namespace content {

enum MessageDeliveryPolicy {
  // If a commit was requested before the message was enqueued the message
  // will be delivered with the swap corresponding to that commit. Otherwise
  // the message will be sent immediately using regular IPC.
  // If the commit is aborted or optimized out the message is sent using regular
  // IPC.
  MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE,
  // The message will be delivered with the next swap.
  // If the swap is optimized out, the message is sent using regular IPC.
  // If the swap fails the message remains enqueued.
  MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP,
};

}  // namespace content

#endif  // CONTENT_RENDERER_MESSAGE_DELIVERY_POLICY_H_
