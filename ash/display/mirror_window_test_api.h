// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_MIRROR_WINDOW_TEST_API_H_
#define ASH_DISPLAY_MIRROR_WINDOW_TEST_API_H_

#include <vector>

#include "base/macros.h"

namespace aura {
class Window;
class WindowTreeHost;
}  // namespace aura

namespace gfx {
class Point;
}

namespace ui {
enum class CursorType;
}

namespace ash {

class MirrorWindowTestApi {
 public:
  MirrorWindowTestApi() {}
  ~MirrorWindowTestApi() {}

  std::vector<aura::WindowTreeHost*> GetHosts() const;

  ui::CursorType GetCurrentCursorType() const;

  // Returns the position of the hot point within the cursor. This is
  // unaffected by the cursor location.
  const gfx::Point& GetCursorHotPoint() const;

  // Returns the position of the cursor hot point in root window coordinates.
  // This should be the same as the native cursor location.
  gfx::Point GetCursorHotPointLocationInRootWindow() const;

  const aura::Window* GetCursorWindow() const;
  gfx::Point GetCursorLocation() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MirrorWindowTestApi);
};

}  // namespace ash

#endif  // ASH_DISPLAY_MIRROR_WINDOW_TEST_API_H_
