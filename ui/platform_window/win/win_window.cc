// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/win/win_window.h"

#include <algorithm>
#include <memory>

#include "base/strings/string16.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/win/msg_util.h"
#include "ui/platform_window/platform_window_delegate.h"

#include <windows.h>

namespace ui {

namespace {

bool use_popup_as_root_window_for_test = false;

gfx::Rect GetWindowBoundsForClientBounds(DWORD style, DWORD ex_style,
                                         const gfx::Rect& bounds) {
  RECT wr;
  wr.left = bounds.x();
  wr.top = bounds.y();
  wr.right = bounds.x() + bounds.width();
  wr.bottom = bounds.y() + bounds.height();
  AdjustWindowRectEx(&wr, style, FALSE, ex_style);

  // Make sure to keep the window onscreen, as AdjustWindowRectEx() may have
  // moved part of it offscreen.
  gfx::Rect window_bounds(wr.left, wr.top,
                          wr.right - wr.left, wr.bottom - wr.top);
  window_bounds.set_x(std::max(0, window_bounds.x()));
  window_bounds.set_y(std::max(0, window_bounds.y()));
  return window_bounds;
}

}  // namespace

WinWindow::WinWindow(PlatformWindowDelegate* delegate, const gfx::Rect& bounds)
    : delegate_(delegate) {
  CHECK(delegate_);
  DWORD window_style = WS_OVERLAPPEDWINDOW;
  if (use_popup_as_root_window_for_test) {
    set_window_style(WS_POPUP);
    window_style = WS_POPUP;
  }
  gfx::Rect window_bounds =
      GetWindowBoundsForClientBounds(window_style, window_ex_style(), bounds);
  gfx::WindowImpl::Init(NULL, window_bounds);
  SetWindowText(hwnd(), L"WinWindow");
}

WinWindow::~WinWindow() {
}

void WinWindow::Destroy() {
  if (IsWindow(hwnd()))
    DestroyWindow(hwnd());
}

void WinWindow::Show() {
  ShowWindow(hwnd(), SW_SHOWNORMAL);
}

void WinWindow::Hide() {
  ShowWindow(hwnd(), SW_HIDE);
}

void WinWindow::Close() {
  Destroy();
}

void WinWindow::PrepareForShutdown() {}

void WinWindow::SetBounds(const gfx::Rect& bounds) {
  gfx::Rect window_bounds = GetWindowBoundsForClientBounds(
      GetWindowLong(hwnd(), GWL_STYLE),
      GetWindowLong(hwnd(), GWL_EXSTYLE),
      bounds);
  unsigned int flags = SWP_NOREPOSITION;
  if (!::IsWindowVisible(hwnd()))
    flags |= SWP_NOACTIVATE;
  SetWindowPos(hwnd(), NULL, window_bounds.x(), window_bounds.y(),
               window_bounds.width(), window_bounds.height(), flags);
}

gfx::Rect WinWindow::GetBounds() {
  RECT cr;
  GetClientRect(hwnd(), &cr);
  return gfx::Rect(cr);
}

void WinWindow::SetTitle(const base::string16& title) {
  SetWindowText(hwnd(), title.c_str());
}

void WinWindow::SetCapture() {
  if (!HasCapture())
    ::SetCapture(hwnd());
}

void WinWindow::ReleaseCapture() {
  if (HasCapture())
    ::ReleaseCapture();
}

bool WinWindow::HasCapture() const {
  return ::GetCapture() == hwnd();
}

void WinWindow::ToggleFullscreen() {}

void WinWindow::Maximize() {}

void WinWindow::Minimize() {}

void WinWindow::Restore() {}

PlatformWindowState WinWindow::GetPlatformWindowState() const {
  return PlatformWindowState::PLATFORM_WINDOW_STATE_UNKNOWN;
}

void WinWindow::SetCursor(PlatformCursor cursor) {
  ::SetCursor(cursor);
}

void WinWindow::MoveCursorTo(const gfx::Point& location) {
  ::SetCursorPos(location.x(), location.y());
}

void WinWindow::ConfineCursorToBounds(const gfx::Rect& bounds) {
}

PlatformImeController* WinWindow::GetPlatformImeController() {
  return nullptr;
}

LRESULT WinWindow::OnMouseRange(UINT message, WPARAM w_param, LPARAM l_param) {
  MSG msg = { hwnd(), message, w_param, l_param,
              static_cast<DWORD>(GetMessageTime()),
              { CR_GET_X_LPARAM(l_param), CR_GET_Y_LPARAM(l_param) } };
  std::unique_ptr<Event> event = EventFromNative(msg);
  if (IsMouseEventFromTouch(message))
    event->set_flags(event->flags() | EF_FROM_TOUCH);
  if (!(event->flags() & ui::EF_IS_NON_CLIENT))
    delegate_->DispatchEvent(event.get());
  SetMsgHandled(event->handled());
  return 0;
}

LRESULT WinWindow::OnCaptureChanged(UINT message,
                                    WPARAM w_param,
                                    LPARAM l_param) {
  delegate_->OnLostCapture();
  return 0;
}

LRESULT WinWindow::OnKeyEvent(UINT message, WPARAM w_param, LPARAM l_param) {
  MSG msg = { hwnd(), message, w_param, l_param };
  KeyEvent event(msg);
  delegate_->DispatchEvent(&event);
  SetMsgHandled(event.handled());
  return 0;
}

LRESULT WinWindow::OnNCActivate(UINT message, WPARAM w_param, LPARAM l_param) {
  delegate_->OnActivationChanged(!!w_param);
  return DefWindowProc(hwnd(), message, w_param, l_param);
}

void WinWindow::OnClose() {
  delegate_->OnCloseRequest();
}

LRESULT WinWindow::OnCreate(CREATESTRUCT* create_struct) {
  // TODO(sky): provide real scale factor.
  delegate_->OnAcceleratedWidgetAvailable(hwnd(), 1.f);
  return 0;
}

void WinWindow::OnDestroy() {
  delegate_->OnClosed();
}

void WinWindow::OnPaint(HDC) {
  gfx::Rect damage_rect;
  RECT update_rect = {0};
  if (GetUpdateRect(hwnd(), &update_rect, FALSE))
    damage_rect = gfx::Rect(update_rect);
  delegate_->OnDamageRect(damage_rect);
  ValidateRect(hwnd(), NULL);
}

void WinWindow::OnWindowPosChanged(WINDOWPOS* window_pos) {
  if (!(window_pos->flags & SWP_NOSIZE) ||
      !(window_pos->flags & SWP_NOMOVE)) {
    RECT cr;
    GetClientRect(hwnd(), &cr);
    delegate_->OnBoundsChanged(
        gfx::Rect(window_pos->x, window_pos->y,
                  cr.right - cr.left, cr.bottom - cr.top));
  }
}

namespace test {

// static
void SetUsePopupAsRootWindowForTest(bool use) {
  use_popup_as_root_window_for_test = use;
}

}  // namespace test
}  // namespace ui
