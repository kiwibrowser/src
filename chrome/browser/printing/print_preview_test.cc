// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_test.h"

#include "chrome/test/base/dialog_test_browser_window.h"
#include "content/public/browser/plugin_service.h"

PrintPreviewTest::PrintPreviewTest() {}
PrintPreviewTest::~PrintPreviewTest() {}

void PrintPreviewTest::SetUp() {
  BrowserWithTestWindowTest::SetUp();

  // The PluginService will be destroyed at the end of the test (due to the
  // ShadowingAtExitManager in our base class).
  content::PluginService::GetInstance()->Init();
}

BrowserWindow* PrintPreviewTest::CreateBrowserWindow() {
  return new DialogTestBrowserWindow;
}
