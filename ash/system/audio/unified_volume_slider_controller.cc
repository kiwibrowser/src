// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/audio/unified_volume_slider_controller.h"

#include "ash/metrics/user_metrics_action.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/shell.h"
#include "ash/system/audio/unified_volume_view.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"

using chromeos::CrasAudioHandler;

namespace ash {

UnifiedVolumeSliderController::UnifiedVolumeSliderController() = default;
UnifiedVolumeSliderController::~UnifiedVolumeSliderController() = default;

views::View* UnifiedVolumeSliderController::CreateView() {
  DCHECK(!slider_);
  slider_ = new UnifiedVolumeView(this);
  return slider_;
}

void UnifiedVolumeSliderController::ButtonPressed(views::Button* sender,
                                                  const ui::Event& event) {
  bool mute_on = !CrasAudioHandler::Get()->IsOutputMuted();
  if (mute_on)
    base::RecordAction(base::UserMetricsAction("StatusArea_Audio_Muted"));
  else
    base::RecordAction(base::UserMetricsAction("StatusArea_Audio_Unmuted"));
  CrasAudioHandler::Get()->SetOutputMute(mute_on);
}

void UnifiedVolumeSliderController::SliderValueChanged(
    views::Slider* sender,
    float value,
    float old_value,
    views::SliderChangeReason reason) {
  if (reason != views::VALUE_CHANGED_BY_USER)
    return;

  const int level = value * 100;

  if (level != CrasAudioHandler::Get()->GetOutputVolumePercent()) {
    Shell::Get()->metrics()->RecordUserMetricsAction(
        UMA_STATUS_AREA_CHANGED_VOLUME_MENU);
  }

  CrasAudioHandler::Get()->SetOutputVolumePercent(level);

  // If the volume is above certain level and it's muted, it should be unmuted.
  // If the volume is below certain level and it's unmuted, it should be muted.
  if (CrasAudioHandler::Get()->IsOutputMuted() ==
      level > CrasAudioHandler::Get()->GetOutputDefaultVolumeMuteThreshold()) {
    CrasAudioHandler::Get()->SetOutputMute(
        !CrasAudioHandler::Get()->IsOutputMuted());
  }
}

}  // namespace ash
