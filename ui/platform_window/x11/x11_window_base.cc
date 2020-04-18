// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/x11/x11_window_base.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "ui/base/platform_window_defaults.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_window_event_manager.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

namespace {

XID FindXEventTarget(const XEvent& xev) {
  XID target = xev.xany.window;
  if (xev.type == GenericEvent)
    target = static_cast<XIDeviceEvent*>(xev.xcookie.data)->event;
  return target;
}

}  // namespace

X11WindowBase::X11WindowBase(PlatformWindowDelegate* delegate,
                             const gfx::Rect& bounds)
    : delegate_(delegate),
      xdisplay_(gfx::GetXDisplay()),
      xroot_window_(DefaultRootWindow(xdisplay_)),
      bounds_(bounds),
      state_(ui::PlatformWindowState::PLATFORM_WINDOW_STATE_UNKNOWN) {
  DCHECK(delegate_);
  Create();
  pointer_barriers_.fill(x11::None);
}

X11WindowBase::~X11WindowBase() {
  UnConfineCursor();
  Destroy();
}

void X11WindowBase::Destroy() {
  if (xwindow_ == x11::None)
    return;

  // Stop processing events.
  XID xwindow = xwindow_;
  XDisplay* xdisplay = xdisplay_;
  xwindow_ = x11::None;
  delegate_->OnClosed();
  // |this| might be deleted because of the above call.

  XDestroyWindow(xdisplay, xwindow);
}

void X11WindowBase::Create() {
  DCHECK(!bounds_.size().IsEmpty());

  XSetWindowAttributes swa;
  memset(&swa, 0, sizeof(swa));
  swa.background_pixmap = x11::None;
  swa.bit_gravity = NorthWestGravity;
  swa.override_redirect = UseTestConfigForPlatformWindows();
  xwindow_ =
      XCreateWindow(xdisplay_, xroot_window_, bounds_.x(), bounds_.y(),
                    bounds_.width(), bounds_.height(),
                    0,               // border width
                    CopyFromParent,  // depth
                    InputOutput,
                    CopyFromParent,  // visual
                    CWBackPixmap | CWBitGravity | CWOverrideRedirect, &swa);

  // Setup XInput event mask.
  long event_mask = ButtonPressMask | ButtonReleaseMask | FocusChangeMask |
                    KeyPressMask | KeyReleaseMask | EnterWindowMask |
                    LeaveWindowMask | ExposureMask | VisibilityChangeMask |
                    StructureNotifyMask | PropertyChangeMask |
                    PointerMotionMask;
  xwindow_events_.reset(new ui::XScopedEventSelector(xwindow_, event_mask));

  // Setup XInput2 event mask.
  unsigned char mask[XIMaskLen(XI_LASTEVENT)];
  memset(mask, 0, sizeof(mask));

  XISetMask(mask, XI_TouchBegin);
  XISetMask(mask, XI_TouchUpdate);
  XISetMask(mask, XI_TouchEnd);
  XISetMask(mask, XI_ButtonPress);
  XISetMask(mask, XI_ButtonRelease);
  XISetMask(mask, XI_Motion);
  XISetMask(mask, XI_KeyPress);
  XISetMask(mask, XI_KeyRelease);
  XISetMask(mask, XI_HierarchyChanged);

  XIEventMask evmask;
  evmask.deviceid = XIAllDevices;
  evmask.mask_len = sizeof(mask);
  evmask.mask = mask;
  XISelectEvents(xdisplay_, xwindow_, &evmask, 1);
  XFlush(xdisplay_);

  ::Atom protocols[2];
  protocols[0] = gfx::GetAtom("WM_DELETE_WINDOW");
  protocols[1] = gfx::GetAtom("_NET_WM_PING");
  XSetWMProtocols(xdisplay_, xwindow_, protocols, 2);

  // We need a WM_CLIENT_MACHINE and WM_LOCALE_NAME value so we integrate with
  // the desktop environment.
  XSetWMProperties(xdisplay_, xwindow_, NULL, NULL, NULL, 0, NULL, NULL, NULL);

  // Likewise, the X server needs to know this window's pid so it knows which
  // program to kill if the window hangs.
  // XChangeProperty() expects "pid" to be long.
  static_assert(sizeof(long) >= sizeof(pid_t),
                "pid_t should not be larger than long");
  long pid = getpid();
  XChangeProperty(xdisplay_, xwindow_, gfx::GetAtom("_NET_WM_PID"), XA_CARDINAL,
                  32, PropModeReplace, reinterpret_cast<unsigned char*>(&pid),
                  1);
  // Before we map the window, set size hints. Otherwise, some window managers
  // will ignore toplevel XMoveWindow commands.
  XSizeHints size_hints;
  size_hints.flags = PPosition | PWinGravity;
  size_hints.x = bounds_.x();
  size_hints.y = bounds_.y();
  // Set StaticGravity so that the window position is not affected by the
  // frame width when running with window manager.
  size_hints.win_gravity = StaticGravity;
  XSetWMNormalHints(xdisplay_, xwindow_, &size_hints);

  // TODO(sky): provide real scale factor.
  delegate_->OnAcceleratedWidgetAvailable(xwindow_, 1.f);
}

void X11WindowBase::Show() {
  if (window_mapped_)
    return;

  XMapWindow(xdisplay_, xwindow_);
  // TODO(thomasanderson): Find out why this flush is necessary.
  XFlush(xdisplay_);
  window_mapped_ = true;
}

void X11WindowBase::Hide() {
  if (!window_mapped_)
    return;

  XWithdrawWindow(xdisplay_, xwindow_, 0);
  window_mapped_ = false;
}

void X11WindowBase::Close() {
  Destroy();
}

void X11WindowBase::SetBounds(const gfx::Rect& bounds) {
  if (xwindow_ != x11::None) {
    XWindowChanges changes = {0};
    unsigned value_mask = 0;

    if (bounds_.size() != bounds.size()) {
      changes.width = bounds.width();
      changes.height = bounds.height();
      value_mask |= CWHeight | CWWidth;
    }

    if (bounds_.origin() != bounds.origin()) {
      changes.x = bounds.x();
      changes.y = bounds.y();
      value_mask |= CWX | CWY;
    }

    if (value_mask)
      XConfigureWindow(xdisplay_, xwindow_, value_mask, &changes);
  }

  // Assume that the resize will go through as requested, which should be the
  // case if we're running without a window manager.  If there's a window
  // manager, it can modify or ignore the request, but (per ICCCM) we'll get a
  // (possibly synthetic) ConfigureNotify about the actual size and correct
  // |bounds_| later.
  bounds_ = bounds;

  // Even if the pixel bounds didn't change this call to the delegate should
  // still happen. The device scale factor may have changed which effectively
  // changes the bounds.
  delegate_->OnBoundsChanged(bounds_);
}

gfx::Rect X11WindowBase::GetBounds() {
  return bounds_;
}

void X11WindowBase::SetTitle(const base::string16& title) {
  if (window_title_ == title)
    return;
  window_title_ = title;
  std::string utf8str = base::UTF16ToUTF8(title);
  XChangeProperty(xdisplay_, xwindow_, gfx::GetAtom("_NET_WM_NAME"),
                  gfx::GetAtom("UTF8_STRING"), 8, PropModeReplace,
                  reinterpret_cast<const unsigned char*>(utf8str.c_str()),
                  utf8str.size());
  XTextProperty xtp;
  char* c_utf8_str = const_cast<char*>(utf8str.c_str());
  if (Xutf8TextListToTextProperty(xdisplay_, &c_utf8_str, 1, XUTF8StringStyle,
                                  &xtp) == x11::Success) {
    XSetWMName(xdisplay_, xwindow_, &xtp);
    XFree(xtp.value);
  }
}

void X11WindowBase::SetCapture() {}

void X11WindowBase::ReleaseCapture() {}

bool X11WindowBase::HasCapture() const {
  return false;
}

void X11WindowBase::ToggleFullscreen() {
  ui::SetWMSpecState(xwindow_, !IsFullscreen(),
                     gfx::GetAtom("_NET_WM_STATE_FULLSCREEN"), x11::None);
}

void X11WindowBase::Maximize() {
  if (IsFullscreen())
    ToggleFullscreen();

  ui::SetWMSpecState(xwindow_, true,
                     gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT"),
                     gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ"));
}

void X11WindowBase::Minimize() {
  XIconifyWindow(xdisplay_, xwindow_, 0);
}

void X11WindowBase::Restore() {
  if (IsFullscreen())
    ToggleFullscreen();

  if (IsMaximized()) {
    ui::SetWMSpecState(xwindow_, false,
                       gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT"),
                       gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ"));
  }
}

PlatformWindowState X11WindowBase::GetPlatformWindowState() const {
  return state_;
}

void X11WindowBase::MoveCursorTo(const gfx::Point& location) {
  XWarpPointer(xdisplay_, x11::None, xroot_window_, 0, 0, 0, 0,
               bounds_.x() + location.x(), bounds_.y() + location.y());
}

void X11WindowBase::ConfineCursorToBounds(const gfx::Rect& bounds) {
  UnConfineCursor();

  if (bounds.IsEmpty())
    return;

  gfx::Rect barrier = bounds + bounds_.OffsetFromOrigin();

  // Top horizontal barrier.
  pointer_barriers_[0] = XFixesCreatePointerBarrier(
      xdisplay_, xroot_window_, barrier.x(), barrier.y(), barrier.right(),
      barrier.y(), BarrierPositiveY, 0, XIAllDevices);
  // Bottom horizontal barrier.
  pointer_barriers_[1] = XFixesCreatePointerBarrier(
      xdisplay_, xroot_window_, barrier.x(), barrier.bottom(), barrier.right(),
      barrier.bottom(), BarrierNegativeY, 0, XIAllDevices);
  // Left vertical barrier.
  pointer_barriers_[2] = XFixesCreatePointerBarrier(
      xdisplay_, xroot_window_, barrier.x(), barrier.y(), barrier.x(),
      barrier.bottom(), BarrierPositiveX, 0, XIAllDevices);
  // Right vertical barrier.
  pointer_barriers_[3] = XFixesCreatePointerBarrier(
      xdisplay_, xroot_window_, barrier.right(), barrier.y(), barrier.right(),
      barrier.bottom(), BarrierNegativeX, 0, XIAllDevices);

  has_pointer_barriers_ = true;
}

PlatformImeController* X11WindowBase::GetPlatformImeController() {
  return nullptr;
}

void X11WindowBase::UnConfineCursor() {
  if (!has_pointer_barriers_)
    return;

  for (XID pointer_barrier : pointer_barriers_)
    XFixesDestroyPointerBarrier(xdisplay_, pointer_barrier);
  pointer_barriers_.fill(x11::None);

  has_pointer_barriers_ = false;
}

bool X11WindowBase::IsEventForXWindow(const XEvent& xev) const {
  return xwindow_ != x11::None && FindXEventTarget(xev) == xwindow_;
}

void X11WindowBase::ProcessXWindowEvent(XEvent* xev) {
  switch (xev->type) {
    case Expose: {
      gfx::Rect damage_rect(xev->xexpose.x, xev->xexpose.y, xev->xexpose.width,
                            xev->xexpose.height);
      delegate_->OnDamageRect(damage_rect);
      break;
    }

    case x11::FocusOut:
      if (xev->xfocus.mode != NotifyGrab)
        delegate_->OnLostCapture();
      break;

    case ConfigureNotify: {
      DCHECK_EQ(xwindow_, xev->xconfigure.event);
      DCHECK_EQ(xwindow_, xev->xconfigure.window);
      // It's possible that the X window may be resized by some other means than
      // from within aura (e.g. the X window manager can change the size). Make
      // sure the root window size is maintained properly.
      int translated_x_in_pixels = xev->xconfigure.x;
      int translated_y_in_pixels = xev->xconfigure.y;
      if (!xev->xconfigure.send_event && !xev->xconfigure.override_redirect) {
        Window unused;
        XTranslateCoordinates(xdisplay_, xwindow_, xroot_window_, 0, 0,
                              &translated_x_in_pixels, &translated_y_in_pixels,
                              &unused);
      }
      gfx::Rect bounds(translated_x_in_pixels, translated_y_in_pixels,
                       xev->xconfigure.width, xev->xconfigure.height);
      if (bounds_ != bounds) {
        bounds_ = bounds;
        delegate_->OnBoundsChanged(bounds_);
      }
      break;
    }

    case ClientMessage: {
      Atom message = static_cast<Atom>(xev->xclient.data.l[0]);
      if (message == gfx::GetAtom("WM_DELETE_WINDOW")) {
        delegate_->OnCloseRequest();
      } else if (message == gfx::GetAtom("_NET_WM_PING")) {
        XEvent reply_event = *xev;
        reply_event.xclient.window = xroot_window_;

        XSendEvent(xdisplay_, reply_event.xclient.window, x11::False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &reply_event);
        XFlush(xdisplay_);
      }
      break;
    }
    case PropertyNotify: {
      ::Atom changed_atom = xev->xproperty.atom;
      if (changed_atom == gfx::GetAtom("_NET_WM_STATE"))
        OnWMStateUpdated();
      break;
    }
  }
}

void X11WindowBase::OnWMStateUpdated() {
  std::vector<::Atom> atom_list;
  // Ignore the return value of ui::GetAtomArrayProperty(). Fluxbox removes the
  // _NET_WM_STATE property when no _NET_WM_STATE atoms are set.
  ui::GetAtomArrayProperty(xwindow_, "_NET_WM_STATE", &atom_list);

  window_properties_.clear();
  std::copy(atom_list.begin(), atom_list.end(),
            inserter(window_properties_, window_properties_.begin()));

  // Propagate the window state information to the client.
  // Note that the order of checks is important here, because window can have
  // several proprties at the same time.
  ui::PlatformWindowState old_state = state_;
  if (ui::HasWMSpecProperty(window_properties_,
                            gfx::GetAtom("_NET_WM_STATE_HIDDEN"))) {
    state_ = ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MINIMIZED;
  } else if (ui::HasWMSpecProperty(window_properties_,
                                   gfx::GetAtom("_NET_WM_STATE_FULLSCREEN"))) {
    state_ = ui::PlatformWindowState::PLATFORM_WINDOW_STATE_FULLSCREEN;
  } else if (ui::HasWMSpecProperty(
                 window_properties_,
                 gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT")) &&
             ui::HasWMSpecProperty(
                 window_properties_,
                 gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ"))) {
    state_ = ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MAXIMIZED;
  } else {
    state_ = ui::PlatformWindowState::PLATFORM_WINDOW_STATE_NORMAL;
  }

  if (old_state != state_)
    delegate_->OnWindowStateChanged(state_);
}

bool X11WindowBase::IsMaximized() const {
  return state_ == ui::PlatformWindowState::PLATFORM_WINDOW_STATE_MAXIMIZED;
}

bool X11WindowBase::IsFullscreen() const {
  return state_ == ui::PlatformWindowState::PLATFORM_WINDOW_STATE_FULLSCREEN;
}

}  // namespace ui
