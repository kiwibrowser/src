// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/first_run/steps/help_step.h"

#include "ash/public/interfaces/first_run_helper.mojom.h"
#include "base/bind.h"
#include "chrome/browser/chromeos/first_run/first_run_controller.h"
#include "chrome/browser/chromeos/first_run/step_names.h"
#include "chrome/browser/ui/webui/chromeos/first_run/first_run_actor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace {

const int kCircleRadius = 19;

}  // namespace

namespace chromeos {
namespace first_run {

HelpStep::HelpStep(FirstRunController* controller, FirstRunActor* actor)
    : Step(kHelpStep, controller, actor) {}

void HelpStep::DoShow() {
  const ash::mojom::FirstRunHelperPtr& helper_ptr =
      first_run_controller()->first_run_helper_ptr();
  helper_ptr->OpenTrayBubble(base::DoNothing());
  // FirstRunController owns |this|, so use Unretained.
  helper_ptr->GetHelpButtonBounds(base::BindOnce(
      &HelpStep::ShowWithHelpButtonBounds, base::Unretained(this)));
}

void HelpStep::DoOnAfterHide() {
  first_run_controller()->first_run_helper_ptr()->CloseTrayBubble();
}

void HelpStep::ShowWithHelpButtonBounds(const gfx::Rect& screen_bounds) {
  gfx::Point center = screen_bounds.CenterPoint();
  actor()->AddRoundHole(center.x(), center.y(), kCircleRadius);
  actor()->ShowStepPointingTo(name(), center.x(), center.y(), kCircleRadius);
}

}  // namespace first_run
}  // namespace chromeos

