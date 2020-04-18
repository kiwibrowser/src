// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_IME_BRIDGE_OBSERVER_H_
#define UI_BASE_IME_IME_BRIDGE_OBSERVER_H_

#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// A interface to .
class UI_BASE_IME_EXPORT IMEBridgeObserver {
 public:
  virtual ~IMEBridgeObserver() {}

  // Called when requesting to switch the engine handler from ui::InputMethod.
  virtual void OnRequestSwitchEngine() = 0;
};

}  // namespace ui

#endif  // UI_BASE_IME_IME_BRIDGE_OBSERVER_H_
