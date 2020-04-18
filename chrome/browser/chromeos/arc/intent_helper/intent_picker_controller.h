// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_INTENT_PICKER_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_INTENT_PICKER_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class Browser;
class ListSelectionModel;
class TabStripModel;

namespace arc {

// Controls the visibility of IntentPickerView by watching whenever the current
// tab is switched. An object of this class is owned by IntentPickerView.
class IntentPickerController : public TabStripModelObserver {
 public:
  explicit IntentPickerController(Browser* browser);
  ~IntentPickerController() override;

 private:
  // TabStripModelObserver:
  void TabSelectionChanged(TabStripModel* model,
                           const ui::ListSelectionModel& old_model) override;

  void ResetVisibility();

  Browser* const browser_;

  DISALLOW_COPY_AND_ASSIGN(IntentPickerController);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_INTENT_HELPER_INTENT_PICKER_CONTROLLER_H_
