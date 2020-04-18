// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_SOURCE_H_
#define CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_SOURCE_H_

namespace content {

// Describes the source of where the input event ACK was
// generated from inside the renderer.
enum class InputEventAckSource {
  UNKNOWN,
  COMPOSITOR_THREAD,
  MAIN_THREAD,
  MAX_FROM_RENDERER = MAIN_THREAD,
  // All values less than or equal to |MAX_FROM_RENDERER| are
  // permitted to be sent from the renderer on the IPC channel.
  BROWSER,
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_INPUT_EVENT_ACK_SOURCE_H_
