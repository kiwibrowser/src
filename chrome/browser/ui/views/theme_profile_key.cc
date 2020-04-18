// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/theme_profile_key.h"

#include "ui/aura/window.h"
#include "ui/base/class_property.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(Profile*);

namespace {

DEFINE_UI_CLASS_PROPERTY_KEY(Profile*, kThemeProfileKey, nullptr);

}  // anonymous namespace

void SetThemeProfileForWindow(aura::Window* window, Profile* profile) {
  window->SetProperty(kThemeProfileKey, profile);
}

Profile* GetThemeProfileForWindow(aura::Window* window) {
  return window->GetProperty(kThemeProfileKey);
}
