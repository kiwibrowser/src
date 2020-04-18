// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_STUB_STUB_WINDOW_H_
#define UI_PLATFORM_WINDOW_STUB_STUB_WINDOW_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/stub/stub_window_export.h"

namespace ui {

class PlatformWindowDelegate;

// StubWindow is useful for tests, as well as implementations that only care
// about bounds.
class STUB_WINDOW_EXPORT StubWindow : public PlatformWindow {
 public:
  explicit StubWindow(PlatformWindowDelegate* delegate,
                      bool use_default_accelerated_widget = true,
                      const gfx::Rect& bounds = gfx::Rect());
  ~StubWindow() override;

 protected:
  PlatformWindowDelegate* delegate() { return delegate_; }

 private:
  // PlatformWindow:
  void Show() override;
  void Hide() override;
  void Close() override;
  void PrepareForShutdown() override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void ToggleFullscreen() override;
  bool HasCapture() const override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  PlatformWindowState GetPlatformWindowState() const override;
  void SetCursor(PlatformCursor cursor) override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;

  PlatformWindowDelegate* delegate_;
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(StubWindow);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_STUB_STUB_WINDOW_H_
