// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_SWITCHES_H_
#define UI_KEYBOARD_KEYBOARD_SWITCHES_H_

#include "ui/keyboard/keyboard_export.h"

namespace keyboard {
namespace switches {

// Disables IME extension APIs from overriding the URL for specifying the
// contents of the virtual keyboard container.
KEYBOARD_EXPORT extern const char kDisableInputView[];

// Disables voice input.
KEYBOARD_EXPORT extern const char kDisableVoiceInput[];

// Enables experimental features for IME extensions.
KEYBOARD_EXPORT extern const char kEnableExperimentalInputViewFeatures[];

// Flag which disables gesture typing for the virtual keyboard.
KEYBOARD_EXPORT extern const char kDisableGestureTyping[];

// Controls the appearance of the settings option to enable gesture editing
// for the virtual keyboard.
KEYBOARD_EXPORT extern const char kDisableGestureEditing[];

// Enables the virtual keyboard.
KEYBOARD_EXPORT extern const char kEnableVirtualKeyboard[];

// Floating virtual keyboard flag.
KEYBOARD_EXPORT extern const char kEnableFloatingVirtualKeyboard[];

// Disabled overscrolling of web content when the virtual keyboard is displayed.
// If disabled, the work area is resized to restrict windows from overlapping
// with the keybaord area.
KEYBOARD_EXPORT extern const char kDisableVirtualKeyboardOverscroll[];

}  // namespace switches
}  // namespace keyboard

#endif  //  UI_KEYBOARD_KEYBOARD_SWITCHES_H_
