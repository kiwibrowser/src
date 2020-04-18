// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/audio/volume_view.h"

#include <algorithm>

#include "ash/metrics/user_metrics_action.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/actionable_view.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/fill_layout.h"

using chromeos::CrasAudioHandler;

namespace ash {
namespace {

const gfx::VectorIcon* const kVolumeLevelIcons[] = {
    &kSystemMenuVolumeMuteIcon,    // Muted.
    &kSystemMenuVolumeLowIcon,     // Low volume.
    &kSystemMenuVolumeMediumIcon,  // Medium volume.
    &kSystemMenuVolumeHighIcon,    // High volume.
    &kSystemMenuVolumeHighIcon,    // Full volume.
};

const gfx::VectorIcon& GetActiveOutputDeviceVectorIcon() {
  chromeos::AudioDevice device;
  if (CrasAudioHandler::Get()->GetPrimaryActiveOutputDevice(&device)) {
    if (device.type == chromeos::AUDIO_TYPE_HEADPHONE)
      return kSystemMenuHeadsetIcon;
    if (device.type == chromeos::AUDIO_TYPE_USB)
      return kSystemMenuUsbIcon;
    if (device.type == chromeos::AUDIO_TYPE_BLUETOOTH)
      return kSystemMenuBluetoothIcon;
    if (device.type == chromeos::AUDIO_TYPE_HDMI)
      return kSystemMenuHdmiIcon;
  }
  return gfx::kNoneIcon;
}

}  // namespace

namespace tray {

class VolumeButton : public ButtonListenerActionableView {
 public:
  VolumeButton(SystemTrayItem* owner, views::ButtonListener* listener)
      : ButtonListenerActionableView(owner,
                                     TrayPopupInkDropStyle::HOST_CENTERED,
                                     listener),
        image_(TrayPopupUtils::CreateMainImageView()),
        image_index_(-1) {
    TrayPopupUtils::ConfigureContainer(TriView::Container::START, this);
    AddChildView(image_);
    SetInkDropMode(InkDropMode::ON);
    Update();

    set_notify_enter_exit_on_child(true);
  }

  ~VolumeButton() override = default;

  void Update() {
    CrasAudioHandler* audio_handler = CrasAudioHandler::Get();
    float level = audio_handler->GetOutputVolumePercent() / 100.0f;
    int volume_levels = arraysize(kVolumeLevelIcons) - 1;
    int image_index =
        audio_handler->IsOutputMuted()
            ? 0
            : (level == 1.0 ? volume_levels
                            : std::max(1, static_cast<int>(std::ceil(
                                              level * (volume_levels - 1)))));
    gfx::ImageSkia image_skia = gfx::CreateVectorIcon(
        *kVolumeLevelIcons[image_index], features::IsSystemTrayUnifiedEnabled()
                                             ? kUnifiedMenuIconColor
                                             : kMenuIconColor);
    image_->SetImage(&image_skia);
    image_index_ = image_index;
  }

 private:
  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->SetName(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_VOLUME_MUTE));
    node_data->role = ax::mojom::Role::kToggleButton;
    const bool is_pressed = CrasAudioHandler::Get()->IsOutputMuted();
    node_data->SetCheckedState(is_pressed ? ax::mojom::CheckedState::kTrue
                                          : ax::mojom::CheckedState::kFalse);
  }

  views::ImageView* image_;
  int image_index_;

  DISALLOW_COPY_AND_ASSIGN(VolumeButton);
};

VolumeView::VolumeView(SystemTrayItem* owner,
                       bool is_default_view)
    : owner_(owner),
      tri_view_(TrayPopupUtils::CreateMultiTargetRowView()),
      more_button_(nullptr),
      icon_(nullptr),
      slider_(nullptr),
      device_type_(nullptr),
      is_default_view_(is_default_view) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(tri_view_);

  icon_ = new VolumeButton(owner, this);
  tri_view_->AddView(TriView::Container::START, icon_);

  slider_ = TrayPopupUtils::CreateSlider(this);
  slider_->SetValue(CrasAudioHandler::Get()->GetOutputVolumePercent() / 100.0f);
  slider_->GetViewAccessibility().OverrideName(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_VOLUME));
  tri_view_->AddView(TriView::Container::CENTER, slider_);

  SetBackground(features::IsSystemTrayUnifiedEnabled()
                    ? views::CreateSolidBackground(kUnifiedMenuBackgroundColor)
                    : views::CreateThemedSolidBackground(
                          this, ui::NativeTheme::kColorId_BubbleBackground));

  Update();
}

VolumeView::~VolumeView() = default;

void VolumeView::Update() {
  icon_->Update();
  slider_->UpdateState(!CrasAudioHandler::Get()->IsOutputMuted());
  UpdateDeviceTypeAndMore();
  Layout();
}

void VolumeView::SetVolumeLevel(float percent) {
  // Update volume level to the current audio level.
  Update();

  // Slider's value is in finer granularity than audio volume level(0.01),
  // there will be a small discrepancy between slider's value and volume level
  // on audio side. To avoid the jittering in slider UI, do not set change
  // slider value if the change is less than 1%.
  if (std::abs(percent - slider_->value()) < 0.01)
    return;
  slider_->SetValue(percent);
  // It is possible that the volume was (un)muted, but the actual volume level
  // did not change. In that case, setting the value of the slider won't
  // trigger an update. So explicitly trigger an update.
  Update();
  slider_->set_enable_accessibility_events(true);
}

void VolumeView::UpdateDeviceTypeAndMore() {
  CrasAudioHandler* audio_handler = CrasAudioHandler::Get();
  // There is no point in letting the user open the audio detailed submenu if
  // there are no alternative output or input devices present, so do not show
  // the 'more' button in the default audio row.
  if (is_default_view_ && !audio_handler->has_alternative_output() &&
      !audio_handler->has_alternative_input()) {
    tri_view_->SetContainerVisible(TriView::Container::END, false);
    return;
  }

  const gfx::VectorIcon& device_icon = GetActiveOutputDeviceVectorIcon();
  const bool device_icon_visibility = !device_icon.is_empty();

  if (is_default_view_) {
    if (!more_button_) {
      more_button_ = new ButtonListenerActionableView(
          owner_, TrayPopupInkDropStyle::INSET_BOUNDS, this);
      TrayPopupUtils::ConfigureContainer(TriView::Container::END, more_button_);

      more_button_->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
      more_button_->SetBorder(
          views::CreateEmptyBorder(gfx::Insets(0, kTrayPopupButtonEndMargin)));
      tri_view_->AddView(TriView::Container::END, more_button_);

      device_type_ = TrayPopupUtils::CreateMoreImageView();
      device_type_->SetVisible(false);
      more_button_->AddChildView(device_type_);

      more_button_->AddChildView(TrayPopupUtils::CreateMoreImageView());
      more_button_->SetAccessibleName(
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_AUDIO));
    }
  } else {
    if (!device_icon_visibility) {
      tri_view_->SetContainerVisible(TriView::Container::END, false);
      tri_view_->InvalidateLayout();
      return;
    }
    if (!device_type_) {
      device_type_ = TrayPopupUtils::CreateMoreImageView();
      device_type_->SetVisible(false);
      tri_view_->AddView(TriView::Container::END, device_type_);
    }
  }

  tri_view_->SetContainerVisible(TriView::Container::END, true);
  tri_view_->InvalidateLayout();
  if (device_icon_visibility)
    device_type_->SetImage(gfx::CreateVectorIcon(
        device_icon, features::IsSystemTrayUnifiedEnabled()
                         ? kUnifiedMenuIconColor
                         : kMenuIconColor));
  if (device_type_->visible() != device_icon_visibility) {
    device_type_->SetVisible(device_icon_visibility);
    device_type_->InvalidateLayout();
  }
}

void VolumeView::HandleVolumeUp(int level) {
  CrasAudioHandler* audio_handler = CrasAudioHandler::Get();
  audio_handler->SetOutputVolumePercent(level);
  if (audio_handler->IsOutputMuted() &&
      level > audio_handler->GetOutputDefaultVolumeMuteThreshold()) {
    audio_handler->SetOutputMute(false);
  }
}

void VolumeView::HandleVolumeDown(int level) {
  CrasAudioHandler* audio_handler = CrasAudioHandler::Get();
  audio_handler->SetOutputVolumePercent(level);
  if (!audio_handler->IsOutputMuted() &&
      level <= audio_handler->GetOutputDefaultVolumeMuteThreshold()) {
    audio_handler->SetOutputMute(true);
  } else if (audio_handler->IsOutputMuted() &&
             level > audio_handler->GetOutputDefaultVolumeMuteThreshold()) {
    audio_handler->SetOutputMute(false);
  }
}

void VolumeView::ButtonPressed(views::Button* sender, const ui::Event& event) {
  if (sender == icon_) {
    CrasAudioHandler* audio_handler = CrasAudioHandler::Get();
    bool mute_on = !audio_handler->IsOutputMuted();

    if (mute_on)
      base::RecordAction(base::UserMetricsAction("StatusArea_Audio_Muted"));
    else
      base::RecordAction(base::UserMetricsAction("StatusArea_Audio_Unmuted"));

    audio_handler->SetOutputMute(mute_on);
    if (!mute_on)
      audio_handler->AdjustOutputVolumeToAudibleLevel();
    icon_->Update();
  } else if (sender == more_button_) {
    owner_->TransitionDetailedView();
  } else {
    NOTREACHED() << "Unexpected sender=" << sender->GetClassName() << ".";
  }
}

void VolumeView::SliderValueChanged(views::Slider* sender,
                                    float value,
                                    float old_value,
                                    views::SliderChangeReason reason) {
  if (reason == views::VALUE_CHANGED_BY_USER) {
    int new_volume = static_cast<int>(value * 100);
    int current_volume = CrasAudioHandler::Get()->GetOutputVolumePercent();
    if (new_volume == current_volume)
      return;
    Shell::Get()->metrics()->RecordUserMetricsAction(
        is_default_view_ ? UMA_STATUS_AREA_CHANGED_VOLUME_MENU
                         : UMA_STATUS_AREA_CHANGED_VOLUME_POPUP);
    if (new_volume > current_volume)
      HandleVolumeUp(new_volume);
    else
      HandleVolumeDown(new_volume);
  }
  icon_->Update();
}

}  // namespace tray
}  // namespace ash
