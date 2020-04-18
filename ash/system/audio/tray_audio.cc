// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/audio/tray_audio.h"

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/audio/audio_detailed_view.h"
#include "ash/system/audio/volume_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/display/screen.h"
#include "ui/views/view.h"

namespace ash {

using chromeos::CrasAudioHandler;
using chromeos::DBusThreadManager;

TrayAudio::TrayAudio(SystemTray* system_tray)
    : TrayImageItem(system_tray, kSystemTrayVolumeMuteIcon, UMA_AUDIO),
      volume_view_(nullptr),
      pop_up_volume_view_(false),
      audio_detail_view_(nullptr),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {
  if (CrasAudioHandler::IsInitialized())
    CrasAudioHandler::Get()->AddAudioObserver(this);
  display::Screen::GetScreen()->AddObserver(this);
  DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(this);
}

TrayAudio::~TrayAudio() {
  DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);
  if (CrasAudioHandler::IsInitialized())
    CrasAudioHandler::Get()->RemoveAudioObserver(this);
}

// static
void TrayAudio::ShowPopUpVolumeView() {
  // Show the popup on all monitors with a system tray.
  for (RootWindowController* root : Shell::GetAllRootWindowControllers()) {
    SystemTray* system_tray = root->GetSystemTray();
    if (!system_tray)
      continue;
    // Show the popup by simulating a volume change. The provided node id and
    // volume value are ignored.
    system_tray->GetTrayAudio()->OnOutputNodeVolumeChanged(0, 0);
  }
}

bool TrayAudio::GetInitialVisibility() {
  return CrasAudioHandler::Get()->IsOutputMuted();
}

views::View* TrayAudio::CreateDefaultView(LoginStatus status) {
  volume_view_ = new tray::VolumeView(this, true);
  return volume_view_;
}

views::View* TrayAudio::CreateDetailedView(LoginStatus status) {
  if (pop_up_volume_view_) {
    volume_view_ = new tray::VolumeView(this, false);
    return volume_view_;
  } else {
    Shell::Get()->metrics()->RecordUserMetricsAction(
        UMA_STATUS_AREA_DETAILED_AUDIO_VIEW);
    audio_detail_view_ =
        new tray::AudioDetailedView(detailed_view_delegate_.get());
    return audio_detail_view_;
  }
}

void TrayAudio::OnDefaultViewDestroyed() {
  volume_view_ = nullptr;
}

void TrayAudio::OnDetailedViewDestroyed() {
  if (audio_detail_view_) {
    audio_detail_view_ = nullptr;
  } else if (volume_view_) {
    volume_view_ = nullptr;
    pop_up_volume_view_ = false;
  }
}

bool TrayAudio::ShouldShowShelf() const {
  return !pop_up_volume_view_;
}

views::View* TrayAudio::GetItemToRestoreFocusTo() {
  if (!volume_view_)
    return nullptr;

  // The more button on |volume_view_| is the view that triggered the detail
  // view, so it should grab focus when going back to the default view.
  return volume_view_->more_button();
}

void TrayAudio::OnOutputNodeVolumeChanged(uint64_t /* node_id */,
                                          int /* volume */) {
  float percent = CrasAudioHandler::Get()->GetOutputVolumePercent() / 100.0f;
  if (tray_view())
    tray_view()->SetVisible(GetInitialVisibility());

  if (volume_view_) {
    volume_view_->SetVolumeLevel(percent);
    SetDetailedViewCloseDelay(kTrayPopupAutoCloseDelayInSeconds);
    return;
  }

  // Show popup only when UnifiedSystemTray bubble is not shown.
  if (IsUnifiedBubbleShown())
    return;

  pop_up_volume_view_ = true;
  ShowDetailedView(kTrayPopupAutoCloseDelayInSeconds);
}

void TrayAudio::OnOutputMuteChanged(bool /* mute_on */, bool system_adjust) {
  if (tray_view())
    tray_view()->SetVisible(GetInitialVisibility());

  if (volume_view_) {
    volume_view_->Update();
    SetDetailedViewCloseDelay(kTrayPopupAutoCloseDelayInSeconds);
  } else if (!system_adjust) {
    // Show popup only when UnifiedSystemTray bubble is not shown.
    if (IsUnifiedBubbleShown())
      return;

    pop_up_volume_view_ = true;
    ShowDetailedView(kTrayPopupAutoCloseDelayInSeconds);
  }
}

void TrayAudio::OnAudioNodesChanged() {
  Update();
}

void TrayAudio::OnActiveOutputNodeChanged() {
  Update();
}

void TrayAudio::OnActiveInputNodeChanged() {
  Update();
}

void TrayAudio::ChangeInternalSpeakerChannelMode() {
  // Swap left/right channel only if it is in Yoga mode.
  bool swap = false;
  if (display::Display::HasInternalDisplay()) {
    const display::ManagedDisplayInfo& display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(
            display::Display::InternalDisplayId());
    if (display_info.GetActiveRotation() == display::Display::ROTATE_180)
      swap = true;
  }
  CrasAudioHandler::Get()->SwapInternalSpeakerLeftRightChannel(swap);
}

void TrayAudio::OnDisplayAdded(const display::Display& new_display) {
  if (!new_display.IsInternal())
    return;
  ChangeInternalSpeakerChannelMode();

  // This event will be triggered when the lid of the device is opened to exit
  // the docked mode, we should always start or re-start HDMI re-discovering
  // grace period right after this event.
  CrasAudioHandler::Get()->SetActiveHDMIOutoutRediscoveringIfNecessary(true);
}

void TrayAudio::OnDisplayRemoved(const display::Display& old_display) {
  if (!old_display.IsInternal())
    return;
  ChangeInternalSpeakerChannelMode();

  // This event will be triggered when the lid of the device is closed to enter
  // the docked mode, we should always start or re-start HDMI re-discovering
  // grace period right after this event.
  CrasAudioHandler::Get()->SetActiveHDMIOutoutRediscoveringIfNecessary(true);
}

void TrayAudio::OnDisplayMetricsChanged(const display::Display& display,
                                        uint32_t changed_metrics) {
  if (!display.IsInternal())
    return;

  if (changed_metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION)
    ChangeInternalSpeakerChannelMode();

  // The event could be triggered multiple times during the HDMI display
  // transition, we don't need to restart HDMI re-discovering grace period
  // it is already started earlier.
  CrasAudioHandler::Get()->SetActiveHDMIOutoutRediscoveringIfNecessary(false);
}

void TrayAudio::SuspendDone(const base::TimeDelta& sleep_duration) {
  // This event is triggered when the device resumes after earlier suspension,
  // we should always start or re-start HDMI re-discovering
  // grace period right after this event.
  CrasAudioHandler::Get()->SetActiveHDMIOutoutRediscoveringIfNecessary(true);
}

void TrayAudio::Update() {
  if (tray_view())
    tray_view()->SetVisible(GetInitialVisibility());
  if (volume_view_) {
    volume_view_->SetVolumeLevel(
        CrasAudioHandler::Get()->GetOutputVolumePercent() / 100.0f);
    volume_view_->Update();
  }

  if (audio_detail_view_)
    audio_detail_view_->Update();
}

}  // namespace ash
