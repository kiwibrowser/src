// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_INITIALIZER_H_
#define UI_BASE_IME_INPUT_METHOD_INITIALIZER_H_

#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// Initializes thread-local resources for input method. This function should be
// called in the UI thread before input method is used.
UI_BASE_IME_EXPORT void InitializeInputMethod();

// Shutdown thread-local resources for input method. This function should be
// called in the UI thread after input method is used.
UI_BASE_IME_EXPORT void ShutdownInputMethod();

// Initializes thread-local resources for input method. This function is
// intended to be called from Setup function of unit tests.
UI_BASE_IME_EXPORT void InitializeInputMethodForTesting();

// Initializes thread-local resources for input method. This function is
// intended to be called from TearDown function of unit tests.
UI_BASE_IME_EXPORT void ShutdownInputMethodForTesting();

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_INITIALIZER_H_
