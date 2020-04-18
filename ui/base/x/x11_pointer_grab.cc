// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/x/x11_pointer_grab.h"

#include "base/logging.h"
#include "ui/base/x/x11_util.h"
#include "ui/events/devices/x11/device_data_manager_x11.h"
#include "ui/gfx/x/x11.h"

namespace ui {

namespace {

// The grab window. None if there are no active pointer grabs.
XID g_grab_window = x11::None;

// The "owner events" parameter used to grab the pointer.
bool g_owner_events = false;

}  // namespace

int GrabPointer(XID window, bool owner_events, ::Cursor cursor) {
  int result = GrabInvalidTime;
  if (ui::IsXInput2Available()) {
    // Do an XInput2 pointer grab. If there is an active XInput2 pointer grab
    // as a result of normal button press, XGrabPointer() will fail.
    unsigned char mask[XIMaskLen(XI_LASTEVENT)];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XISetMask(mask, XI_TouchBegin);
    XISetMask(mask, XI_TouchUpdate);
    XISetMask(mask, XI_TouchEnd);
    XIEventMask evmask;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;

    const std::vector<int>& master_pointers =
        ui::DeviceDataManagerX11::GetInstance()->master_pointers();
    for (int master_pointer : master_pointers) {
      evmask.deviceid = master_pointer;
      result = XIGrabDevice(gfx::GetXDisplay(), master_pointer, window,
                            x11::CurrentTime, cursor, GrabModeAsync,
                            GrabModeAsync, owner_events, &evmask);
      // Assume that the grab will succeed on either all or none of the master
      // pointers.
      if (result != GrabSuccess) {
        // Try core pointer grab.
        break;
      }
    }
  }

  if (result != GrabSuccess) {
    int event_mask = PointerMotionMask | ButtonReleaseMask | ButtonPressMask;
    result = XGrabPointer(gfx::GetXDisplay(), window, owner_events, event_mask,
                          GrabModeAsync, GrabModeAsync, x11::None, cursor,
                          x11::CurrentTime);
  }

  if (result == GrabSuccess) {
    g_grab_window = window;
    g_owner_events = owner_events;
  }
  return result;
}

void ChangeActivePointerGrabCursor(::Cursor cursor) {
  DCHECK(g_grab_window != x11::None);
  GrabPointer(g_grab_window, g_owner_events, cursor);
}

void UngrabPointer() {
  g_grab_window = x11::None;
  if (ui::IsXInput2Available()) {
    const std::vector<int>& master_pointers =
        ui::DeviceDataManagerX11::GetInstance()->master_pointers();
    for (int master_pointer : master_pointers)
      XIUngrabDevice(gfx::GetXDisplay(), master_pointer, x11::CurrentTime);
  }
  // Try core pointer ungrab in case the XInput2 pointer ungrab failed.
  XUngrabPointer(gfx::GetXDisplay(), x11::CurrentTime);
}

}  // namespace ui
