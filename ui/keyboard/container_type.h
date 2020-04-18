// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_CONTAINER_TYPE_H_
#define UI_KEYBOARD_CONTAINER_TYPE_H_

namespace keyboard {

// Enum corresponding to the various container behaviors.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class ContainerType {

  // Corresponds to a ContainerFullWidthBehavior.
  FULL_WIDTH = 0,

  // Corresponds to a ContainerFloatingBehavior.
  FLOATING = 1,

  // Corresponds to a ContainerFullscreenBehavior.
  FULLSCREEN = 2,

  COUNT,
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_CONTAINER_TYPE_H_
