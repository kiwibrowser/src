// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_COCOA_COCOA_EVENT_UTILS_H_
#define UI_EVENTS_COCOA_COCOA_EVENT_UTILS_H_

#import <Cocoa/Cocoa.h>

#include "ui/events/events_export.h"

namespace ui {

// Conversion between wheel delta amounts and number of pixels to scroll.
constexpr double kScrollbarPixelsPerCocoaTick = 40.0;

// Converts the Cocoa |modifiers| bitsum into a ui::EventFlags bitsum.
EVENTS_EXPORT int EventFlagsFromModifiers(NSUInteger modifiers);

// Retrieves a bitsum of ui::EventFlags represented by |event|,
// but instead use the modifier flags given by |modifiers|,
// which is the same format as |-NSEvent modifierFlags|. This allows
// substitution of the modifiers without having to create a new event from
// scratch.
EVENTS_EXPORT int EventFlagsFromNSEventWithModifiers(NSEvent* event,
                                                     NSUInteger modifiers);

// Returns true for |NSKeyUp| and for |NSFlagsChanged| when modifier key was
// released.
EVENTS_EXPORT bool IsKeyUpEvent(NSEvent* event);

}  // namespace ui

#endif  // UI_EVENTS_COCOA_COCOA_EVENT_UTILS_H_
