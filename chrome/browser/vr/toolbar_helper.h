// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TOOLBAR_HELPER_H_
#define CHROME_BROWSER_VR_TOOLBAR_HELPER_H_

#include "chrome/browser/vr/browser_ui_interface.h"
#include "chrome/browser/vr/model/toolbar_state.h"

class ToolbarModel;
class ToolbarModelDelegate;

namespace vr {

class BrowserUiInterface;

// This class houses an instance of ToolbarModel, and queries it when requested,
// passing a snapshot of the toolbar state to the UI when necessary.
class ToolbarHelper {
 public:
  ToolbarHelper(BrowserUiInterface* ui, ToolbarModelDelegate* delegate);
  virtual ~ToolbarHelper();

  // Poll ToolbarModel and post an update to the UI if state has changed.
  void Update();

 private:
  BrowserUiInterface* ui_;
  std::unique_ptr<ToolbarModel> toolbar_model_;
  ToolbarState current_state_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TOOLBAR_HELPER_H_
