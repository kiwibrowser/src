// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/intent_helper/intent_picker_controller.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace arc {

IntentPickerController::IntentPickerController(Browser* browser)
    : browser_(browser) {
  browser_->tab_strip_model()->AddObserver(this);
}

IntentPickerController::~IntentPickerController() {
  browser_->tab_strip_model()->RemoveObserver(this);
}

void IntentPickerController::TabSelectionChanged(
    TabStripModel* model,
    const ui::ListSelectionModel& old_model) {
  ResetVisibility();
}

void IntentPickerController::ResetVisibility() {
  browser_->window()->SetIntentPickerViewVisibility(/*visible=*/false);
}

}  // namespace arc
