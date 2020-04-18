// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_EVENT_H_
#define UI_EVENTS_PLATFORM_EVENT_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(USE_X11)
typedef union _XEvent XEvent;
#elif defined(OS_MACOSX)
#if defined(__OBJC__)
@class NSEvent;
#else   // __OBJC__
class NSEvent;
#endif  // __OBJC__
#endif

namespace ui {
class Event;
}

namespace ui {

// Cross platform typedefs for native event types.
#if defined(USE_OZONE)
using PlatformEvent = ui::Event*;
#elif defined(OS_WIN)
using PlatformEvent = MSG;
#elif defined(USE_X11)
using PlatformEvent = XEvent*;
#elif defined(OS_MACOSX)
using PlatformEvent = NSEvent*;
#else
using PlatformEvent = void*;
#endif

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_EVENT_H_
