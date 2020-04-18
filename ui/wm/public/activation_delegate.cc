// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/public/activation_delegate.h"

#include "ui/aura/window.h"
#include "ui/base/class_property.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(wm::ActivationDelegate*)

namespace wm {

DEFINE_LOCAL_UI_CLASS_PROPERTY_KEY(
    ActivationDelegate*, kActivationDelegateKey, NULL);

void SetActivationDelegate(aura::Window* window, ActivationDelegate* delegate) {
  window->SetProperty(kActivationDelegateKey, delegate);
}

ActivationDelegate* GetActivationDelegate(aura::Window* window) {
  return window->GetProperty(kActivationDelegateKey);
}

}  // namespace wm
