// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_INPUT_EVENT_DISPATCH_TYPE_H_
#define CONTENT_COMMON_INPUT_INPUT_EVENT_DISPATCH_TYPE_H_

namespace content {

enum InputEventDispatchType {
  // Dispatch a blocking event. Sender is waiting on an ACK.
  DISPATCH_TYPE_BLOCKING,
  // Dispatch a non-blocking event. Sender will not receive an ACK.
  DISPATCH_TYPE_NON_BLOCKING,
  DISPATCH_TYPE_MAX = DISPATCH_TYPE_NON_BLOCKING
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_INPUT_EVENT_DISPATCH_TYPE_H_
