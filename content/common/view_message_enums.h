// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_VIEW_MESSAGE_ENUMS_H_
#define CONTENT_COMMON_VIEW_MESSAGE_ENUMS_H_

#include "ipc/ipc_message_macros.h"

// Values that may be OR'd together to form the 'flags' parameter of a
// ViewHostMsg_ResizeOrRepaint_ACK_Params structure.
struct ViewHostMsg_ResizeOrRepaint_ACK_Flags {
  enum {
    IS_RESIZE_ACK = 1 << 0,
    IS_REPAINT_ACK = 1 << 2,
  };
  static bool is_resize_ack(int flags) {
    return (flags & IS_RESIZE_ACK) != 0;
  }
  static bool is_repaint_ack(int flags) {
    return (flags & IS_REPAINT_ACK) != 0;
  }
};

#endif  // CONTENT_COMMON_VIEW_MESSAGE_ENUMS_H_
