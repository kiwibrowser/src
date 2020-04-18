// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_
#define UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_

#include <stdint.h>

#include <array>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/x11/x11_window_export.h"

namespace ui {

class XScopedEventSelector;

// Abstract base implementation for a X11 based PlatformWindow. Methods that
// are platform specific are left unimplemented.
class X11_WINDOW_EXPORT X11WindowBase : public PlatformWindow {
 public:
  X11WindowBase(PlatformWindowDelegate* delegate, const gfx::Rect& bounds);
  ~X11WindowBase() override;

  // PlatformWindow:
  void Show() override;
  void Hide() override;
  void Close() override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  PlatformWindowState GetPlatformWindowState() const override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;

 protected:
  // Creates new underlying XWindow. Does not map XWindow.
  void Create();

  void Destroy();

  PlatformWindowDelegate* delegate() { return delegate_; }
  XDisplay* xdisplay() { return xdisplay_; }
  XID xwindow() const { return xwindow_; }

  void UnConfineCursor();

  // Checks if XEvent is for this XWindow.
  bool IsEventForXWindow(const XEvent& xev) const;

  // Processes events for this XWindow.
  void ProcessXWindowEvent(XEvent* xev);

 private:
  // Called when WM_STATE property is changed.
  void OnWMStateUpdated();

  bool IsMaximized() const;
  bool IsFullscreen() const;

  PlatformWindowDelegate* const delegate_;

  XDisplay* xdisplay_;
  XID xwindow_ = x11::None;
  XID xroot_window_;
  std::unique_ptr<ui::XScopedEventSelector> xwindow_events_;

  base::string16 window_title_;

  // The bounds of |xwindow_|.
  gfx::Rect bounds_;

  // The window manager state bits.
  base::flat_set<::Atom> window_properties_;

  // Stores current state of this window.
  ui::PlatformWindowState state_;

  // Keep track of barriers to confine cursor.
  bool has_pointer_barriers_ = false;
  std::array<XID, 4> pointer_barriers_;

  bool window_mapped_ = false;

  DISALLOW_COPY_AND_ASSIGN(X11WindowBase);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_
