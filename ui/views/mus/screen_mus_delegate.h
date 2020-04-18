// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_SCREEN_MUS_DELEGATE_H_
#define UI_VIEWS_MUS_SCREEN_MUS_DELEGATE_H_

#include "ui/views/mus/mus_export.h"

namespace aura {
class Window;
}

namespace gfx {
class Point;
}

namespace views {

// Delegate for screen implementation backed by ui::mojom::ScreenProvider.
class VIEWS_MUS_EXPORT ScreenMusDelegate {
 public:
  virtual void OnWindowManagerFrameValuesChanged() = 0;

  virtual aura::Window* GetWindowAtScreenPoint(const gfx::Point& point) = 0;

 protected:
  virtual ~ScreenMusDelegate() {}
};

}  // namespace views

#endif  // UI_VIEWS_MUS_SCREEN_MUS_DELEGATE_H_
