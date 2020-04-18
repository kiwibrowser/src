// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_WM_STATE_H_
#define UI_WM_CORE_WM_STATE_H_

#include <memory>

#include "base/macros.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

class CaptureController;
class TransientWindowController;
class TransientWindowStackingClient;

// Installs state needed by the window manager.
class WM_CORE_EXPORT WMState {
 public:
  WMState();
  ~WMState();

  CaptureController* capture_controller() { return capture_controller_.get(); }

 private:
  std::unique_ptr<TransientWindowStackingClient> window_stacking_client_;
  std::unique_ptr<TransientWindowController> transient_window_client_;
  std::unique_ptr<CaptureController> capture_controller_;

  DISALLOW_COPY_AND_ASSIGN(WMState);
};

}  // namespace wm

#endif  // UI_WM_CORE_WM_STATE_H_
