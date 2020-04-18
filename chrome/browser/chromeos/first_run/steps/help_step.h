// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_HELP_STEP_H_
#define CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_HELP_STEP_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/first_run/step.h"

namespace gfx {
class Rect;
}

namespace chromeos {
namespace first_run {

class HelpStep : public Step {
 public:
  HelpStep(FirstRunController* controller, FirstRunActor* actor);

 private:
  // Step:
  void DoShow() override;
  void DoOnAfterHide() override;

  void ShowWithHelpButtonBounds(const gfx::Rect& screen_bounds);

  DISALLOW_COPY_AND_ASSIGN(HelpStep);
};

}  // namespace first_run
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_HELP_STEP_H_

