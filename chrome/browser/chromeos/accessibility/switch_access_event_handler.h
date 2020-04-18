// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SWITCH_ACCESS_EVENT_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SWITCH_ACCESS_EVENT_HANDLER_H_

#include <set>

#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace chromeos {

// When Switch Access is enabled, intercepts a few keys before the rest of the
// OS has a chance to handle them, as a method for controlling the entire OS
// with a small number of switches. The key events do not propagate to the rest
// of the system.
//
// This class just intercepts the keys and forwards them to the component
// extension.
class SwitchAccessEventHandler : public ui::EventHandler {
 public:
  SwitchAccessEventHandler();
  ~SwitchAccessEventHandler() override;

  void SetKeysToCapture(const std::set<int>& key_codes);

 private:
  // EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

  void CancelEvent(ui::Event* event);
  void DispatchKeyEventToSwitchAccess(const ui::KeyEvent& event);

  std::set<int> captured_keys_;

  DISALLOW_COPY_AND_ASSIGN(SwitchAccessEventHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_SWITCH_ACCESS_EVENT_HANDLER_H_
