// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_RESOURCE_UTIL_H_
#define UI_KEYBOARD_KEYBOARD_RESOURCE_UTIL_H_

#include <stddef.h>

#include "ui/keyboard/keyboard_export.h"

struct GritResourceMap;

namespace keyboard {

// The URL of the keyboard extension.
KEYBOARD_EXPORT extern const char kKeyboardURL[];

// The host of the keyboard extension URL.
KEYBOARD_EXPORT extern const char kKeyboardHost[];

// Get the list of keyboard resources. |size| is populated with the number of
// resources in the returned array.
KEYBOARD_EXPORT const GritResourceMap* GetKeyboardExtensionResources(
    size_t* size);

// Initializes the keyboard module. This includes adding the necessary pak files
// for loading resources used in for the virtual keyboard. This becomes a no-op
// after the first call.
KEYBOARD_EXPORT void InitializeKeyboardResources();

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_RESOURCE_UTIL_H_
