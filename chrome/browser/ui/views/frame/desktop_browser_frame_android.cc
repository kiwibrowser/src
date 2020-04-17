// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/desktop_browser_frame_android.h"

#include <memory>

// This file is only instantiated in classic ash/mus. It is never used in mash.
// See native_browser_frame_factory_chromeos.cc switches on
// features::IsUsingWindowService().
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/base/ui_base_features.h"
#include "ui/views/view.h"

BrowserFrameAndroid::BrowserFrameAndroid(BrowserFrame* browser_frame,
                                 BrowserView* browser_view) {
}

BrowserFrameAndroid::~BrowserFrameAndroid() {}

////////////////////////////////////////////////////////////////////////////////
// BrowserFrameAndroid, NativeBrowserFrame implementation:

bool BrowserFrameAndroid::ShouldSaveWindowPlacement() const {
  return false;
}

void BrowserFrameAndroid::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
}

bool BrowserFrameAndroid::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  return false;
}

views::Widget::InitParams BrowserFrameAndroid::GetWidgetParams() {
  views::Widget::InitParams params;
  return params;
}

bool BrowserFrameAndroid::UseCustomFrame() const {
  return true;
}

bool BrowserFrameAndroid::UsesNativeSystemMenu() const {
  return true;
}

int BrowserFrameAndroid::GetMinimizeButtonOffset() const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserFrameAndroid, private:

void BrowserFrameAndroid::SetWindowAutoManaged() {
}
