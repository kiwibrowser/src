// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_FACTORY_H_
#define UI_BASE_IME_INPUT_METHOD_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {
namespace internal {
class InputMethodDelegate;
}  // namespace internal

class InputMethod;

// Creates a new instance of InputMethod and returns it.
UI_BASE_IME_EXPORT std::unique_ptr<InputMethod> CreateInputMethod(
    internal::InputMethodDelegate* delegate,
    gfx::AcceleratedWidget widget);

// Makes CreateInputMethod return a MockInputMethod.
UI_BASE_IME_EXPORT void SetUpInputMethodFactoryForTesting();
UI_BASE_IME_EXPORT void SetUpInputMethodForTesting(InputMethod* input_method);

}  // namespace ui;

#endif  // UI_BASE_IME_INPUT_METHOD_FACTORY_H_
