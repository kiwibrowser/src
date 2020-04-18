// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_QUEUED_CONTAINER_TYPE_H_
#define UI_KEYBOARD_QUEUED_CONTAINER_TYPE_H_

#include "base/bind.h"
#include "base/optional.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/keyboard/container_type.h"

namespace keyboard {

class KeyboardController;

// Tracks a queued ContainerType change request. Couples a container type with a
// callback to invoke once the necessary animation and container changes are
// complete.
// The callback will be invoked once this object goes out of scope. Success
// is defined as the KeyboardController's current container behavior matching
// the same container type as the queued container type.
class QueuedContainerType {
 public:
  QueuedContainerType(KeyboardController* controller,
                      ContainerType container_type,
                      base::Optional<gfx::Rect> bounds,
                      base::OnceCallback<void(bool success)> callback);
  ~QueuedContainerType();
  ContainerType container_type() { return container_type_; }
  base::Optional<gfx::Rect> target_bounds() { return bounds_; }

 private:
  KeyboardController* controller_;
  ContainerType container_type_;
  base::Optional<gfx::Rect> bounds_;
  base::OnceCallback<void(bool success)> callback_;
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_QUEUED_CONTAINER_TYPE_H_
