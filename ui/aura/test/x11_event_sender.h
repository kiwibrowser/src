// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_X11_EVENT_SENDER_H_
#define UI_AURA_TEST_X11_EVENT_SENDER_H_

#include "ui/gfx/x/x11_types.h"

using XEvent = union _XEvent;

namespace aura {

class WindowTreeHost;

namespace test {

void PostEventToWindowTreeHost(const XEvent& xevent, WindowTreeHost* host);

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_X11_EVENT_SENDER_H_
