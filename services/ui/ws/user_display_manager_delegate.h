// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_USER_DISPLAY_MANAGER_DELEGATE_H_
#define SERVICES_UI_WS_USER_DISPLAY_MANAGER_DELEGATE_H_

#include <stdint.h>

#include "services/ui/public/interfaces/window_manager_constants.mojom.h"

namespace ui {
namespace ws {

class UserDisplayManagerDelegate {
 public:
  // Gets the frame decorations. Returns true if the decorations have been set,
  // false otherwise. |values| may be null.
  virtual bool GetFrameDecorations(mojom::FrameDecorationValuesPtr* values) = 0;

  virtual int64_t GetInternalDisplayId() = 0;

 protected:
  virtual ~UserDisplayManagerDelegate() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_USER_DISPLAY_MANAGER_DELEGATE_H_
