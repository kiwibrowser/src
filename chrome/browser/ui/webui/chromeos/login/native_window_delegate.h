// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NATIVE_WINDOW_DELEGATE_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NATIVE_WINDOW_DELEGATE_H_

#include "ui/gfx/native_widget_types.h"

namespace chromeos {

// An interface to get gfx::NativeWindow.
class NativeWindowDelegate {
 public:
  NativeWindowDelegate() {}
  virtual ~NativeWindowDelegate() {}

  // Returns corresponding native window.
  virtual gfx::NativeWindow GetNativeWindow() const = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NATIVE_WINDOW_DELEGATE_H_
