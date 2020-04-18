// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/corewm/tooltip_win.h"

#include <winuser.h>

#include "base/debug/stack_trace.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/corewm/cursor_height_provider_win.h"

namespace views {
namespace corewm {

TooltipWin::TooltipWin(HWND parent)
    : parent_hwnd_(parent),
      tooltip_hwnd_(NULL),
      showing_(false) {
  memset(&toolinfo_, 0, sizeof(toolinfo_));
  toolinfo_.cbSize = sizeof(toolinfo_);
  toolinfo_.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
  toolinfo_.uId = reinterpret_cast<UINT_PTR>(parent_hwnd_);
  toolinfo_.hwnd = parent_hwnd_;
  toolinfo_.lpszText = NULL;
  toolinfo_.lpReserved = NULL;
  SetRectEmpty(&toolinfo_.rect);
}

TooltipWin::~TooltipWin() {
  if (tooltip_hwnd_)
    DestroyWindow(tooltip_hwnd_);
}

bool TooltipWin::HandleNotify(int w_param, NMHDR* l_param, LRESULT* l_result) {
  if (tooltip_hwnd_ == NULL)
    return false;

  switch (l_param->code) {
    case TTN_POP:
      showing_ = false;
      return true;
    case TTN_SHOW:
      *l_result = TRUE;
      PositionTooltip();
      showing_ = true;
      return true;
    default:
      break;
  }
  return false;
}

bool TooltipWin::EnsureTooltipWindow() {
  if (tooltip_hwnd_)
    return true;

  tooltip_hwnd_ = CreateWindowEx(
      WS_EX_TRANSPARENT | l10n_util::GetExtendedTooltipStyles(),
      TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | WS_POPUP, 0, 0, 0, 0,
      parent_hwnd_, NULL, NULL, NULL);
  if (!tooltip_hwnd_) {
    PLOG(WARNING) << "tooltip creation failed, disabling tooltips";
    return false;
  }

  l10n_util::AdjustUIFontForWindow(tooltip_hwnd_);

  SendMessage(tooltip_hwnd_, TTM_ADDTOOL, 0,
              reinterpret_cast<LPARAM>(&toolinfo_));
  return true;
}

void TooltipWin::PositionTooltip() {
  gfx::Point screen_point =
      display::win::ScreenWin::DIPToScreenPoint(location_);
  const int cursoroffset = GetCurrentCursorVisibleHeight();
  screen_point.Offset(0, cursoroffset);

  DWORD tooltip_size = SendMessage(tooltip_hwnd_, TTM_GETBUBBLESIZE, 0,
                                   reinterpret_cast<LPARAM>(&toolinfo_));
  const gfx::Size size(LOWORD(tooltip_size), HIWORD(tooltip_size));

  const display::Display display(
      display::Screen::GetScreen()->GetDisplayNearestPoint(location_));

  gfx::Rect tooltip_bounds(screen_point, size);
  tooltip_bounds.AdjustToFit(
      display::win::ScreenWin::DIPToScreenRect(parent_hwnd_,
                                               display.work_area()));
  SetWindowPos(tooltip_hwnd_, NULL, tooltip_bounds.x(), tooltip_bounds.y(), 0,
               0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

int TooltipWin::GetMaxWidth(const gfx::Point& location) const {
  const gfx::Point screen_point =
      display::win::ScreenWin::DIPToScreenPoint(location);
  display::Display display(
      display::Screen::GetScreen()->GetDisplayNearestPoint(screen_point));
  const gfx::Rect monitor_bounds = display.bounds();
  return (monitor_bounds.width() + 1) / 2;
}

void TooltipWin::SetText(aura::Window* window,
                         const base::string16& tooltip_text,
                         const gfx::Point& location) {
  if (!EnsureTooltipWindow())
    return;

  // See comment in header for details on why |location_| is needed.
  location_ = location;

  // Without this we get a flicker of the tooltip appearing at 0x0. Not sure
  // why.
  SetWindowPos(tooltip_hwnd_, NULL, 0, 0, 0, 0,
               SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE |
               SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER);

  base::string16 adjusted_text(tooltip_text);
  base::i18n::AdjustStringForLocaleDirection(&adjusted_text);
  toolinfo_.lpszText = const_cast<WCHAR*>(adjusted_text.c_str());
  SendMessage(tooltip_hwnd_, TTM_SETTOOLINFO, 0,
              reinterpret_cast<LPARAM>(&toolinfo_));

  int max_width = GetMaxWidth(location_);
  SendMessage(tooltip_hwnd_, TTM_SETMAXTIPWIDTH, 0, max_width);
}

void TooltipWin::Show() {
  if (!EnsureTooltipWindow())
    return;

  SendMessage(tooltip_hwnd_, TTM_TRACKACTIVATE,
              TRUE, reinterpret_cast<LPARAM>(&toolinfo_));
  SetWindowPos(tooltip_hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

void TooltipWin::Hide() {
  if (!tooltip_hwnd_)
    return;

  SendMessage(tooltip_hwnd_, TTM_TRACKACTIVATE, FALSE,
              reinterpret_cast<LPARAM>(&toolinfo_));
}

bool TooltipWin::IsVisible() {
  return showing_;
}

}  // namespace corewm
}  // namespace views
