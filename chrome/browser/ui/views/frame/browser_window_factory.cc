// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/native_browser_frame_factory.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "chrome/grit/chromium_strings.h"
#if defined(USE_AURA)
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#endif
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/widget/widget.h"

// static
BrowserWindow* BrowserWindow::CreateBrowserWindow(Browser* browser,
                                                  bool user_gesture) {
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 1";
  // Create the view and the frame. The frame will attach itself via the view
  // so we don't need to do anything with the pointer.
  BrowserView* view = new BrowserView();
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 2";
  view->Init(browser);
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 3";
  (new BrowserFrame(view))->InitBrowserFrame();
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 4: " << view;
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 4a: " << view->GetWidget();
/*
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 4b: " << view->GetWidget()->non_client_view();
  view->GetWidget()->non_client_view()->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
*/
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 5";

#if defined(USE_AURA)
  // For now, all browser windows are true. This only works when USE_AURA
  // because it requires gfx::NativeWindow to be an aura::Window*.
  view->GetWidget()->GetNativeWindow()->SetProperty(
      aura::client::kCreatedByUserGesture, user_gesture);
#endif
  LOG(INFO) << "[EXTENSIONS] BrowserWindow::CreateBrowserWindow - Step 6: " << view;
  return view;
}
