// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_auto_opener.h"

#include "base/command_line.h"
#include "chrome/browser/devtools/devtools_window.h"

DevToolsAutoOpener::DevToolsAutoOpener()
    : browser_tab_strip_tracker_(this, nullptr, nullptr) {
  browser_tab_strip_tracker_.Init();
}

DevToolsAutoOpener::~DevToolsAutoOpener() {
}

void DevToolsAutoOpener::TabInsertedAt(TabStripModel* tab_strip_model,
                                       content::WebContents* contents,
                                       int index,
                                       bool foreground) {
  if (!DevToolsWindow::IsDevToolsWindow(contents))
    DevToolsWindow::OpenDevToolsWindow(contents);
}
