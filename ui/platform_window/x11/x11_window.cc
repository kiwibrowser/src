// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/x11/x11_window.h"

#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/x/x11.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

X11Window::X11Window(PlatformWindowDelegate* delegate, const gfx::Rect& bounds)
    : X11WindowBase(delegate, bounds) {
  DCHECK(PlatformEventSource::GetInstance());
  PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);
}

X11Window::~X11Window() {
  X11Window::PrepareForShutdown();
}

void X11Window::PrepareForShutdown() {
  PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);
}

void X11Window::SetCursor(PlatformCursor cursor) {
  XDefineCursor(xdisplay(), xwindow(), cursor);
}

void X11Window::ProcessXInput2Event(XEvent* xev) {
  if (!TouchFactory::GetInstance()->ShouldProcessXI2Event(xev))
    return;
  EventType event_type = EventTypeFromNative(xev);
  switch (event_type) {
    case ET_KEY_PRESSED:
    case ET_KEY_RELEASED: {
      KeyEvent key_event(xev);
      delegate()->DispatchEvent(&key_event);
      break;
    }
    case ET_MOUSE_PRESSED:
    case ET_MOUSE_MOVED:
    case ET_MOUSE_DRAGGED:
    case ET_MOUSE_RELEASED: {
      MouseEvent mouse_event(xev);
      delegate()->DispatchEvent(&mouse_event);
      break;
    }
    case ET_MOUSEWHEEL: {
      MouseWheelEvent wheel_event(xev);
      delegate()->DispatchEvent(&wheel_event);
      break;
    }
    case ET_SCROLL_FLING_START:
    case ET_SCROLL_FLING_CANCEL:
    case ET_SCROLL: {
      ScrollEvent scroll_event(xev);
      delegate()->DispatchEvent(&scroll_event);
      break;
    }
    case ET_TOUCH_MOVED:
    case ET_TOUCH_PRESSED:
    case ET_TOUCH_CANCELLED:
    case ET_TOUCH_RELEASED: {
      TouchEvent touch_event(xev);
      delegate()->DispatchEvent(&touch_event);
      break;
    }
    default:
      break;
  }
}

bool X11Window::CanDispatchEvent(const PlatformEvent& xev) {
  return IsEventForXWindow(*xev);
}

uint32_t X11Window::DispatchEvent(const PlatformEvent& event) {
  XEvent* xev = event;
  switch (xev->type) {
    case EnterNotify: {
      MouseEvent mouse_event(xev);
      CHECK_EQ(ET_MOUSE_MOVED, mouse_event.type());
      delegate()->DispatchEvent(&mouse_event);
      break;
    }
    case LeaveNotify: {
      MouseEvent mouse_event(xev);
      delegate()->DispatchEvent(&mouse_event);
      break;
    }

    case KeyPress:
    case KeyRelease: {
      KeyEvent key_event(xev);
      delegate()->DispatchEvent(&key_event);
      break;
    }

    case ButtonPress:
    case ButtonRelease: {
      switch (EventTypeFromNative(xev)) {
        case ET_MOUSEWHEEL: {
          MouseWheelEvent mouseev(xev);
          delegate()->DispatchEvent(&mouseev);
          break;
        }
        case ET_MOUSE_PRESSED:
        case ET_MOUSE_RELEASED: {
          MouseEvent mouseev(xev);
          delegate()->DispatchEvent(&mouseev);
          break;
        }
        case ET_UNKNOWN:
          // No event is created for X11-release events for mouse-wheel
          // buttons.
          break;
        default:
          NOTREACHED();
      }
      break;
    }

    case Expose:
    case x11::FocusOut:
    case ConfigureNotify:
    case ClientMessage: {
      ProcessXWindowEvent(xev);
      break;
    }

    case GenericEvent: {
      ProcessXInput2Event(xev);
      break;
    }
  }
  return POST_DISPATCH_STOP_PROPAGATION;
}

}  // namespace ui
