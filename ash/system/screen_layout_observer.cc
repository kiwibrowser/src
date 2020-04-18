// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_layout_observer.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/display/screen_orientation_controller.h"
#include "ash/metrics/user_metrics_action.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/bind.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/devicetype_utils.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/types/display_constants.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/strings/grit/ui_strings.h"

using message_center::Notification;

namespace ash {
namespace {

const char kNotifierDisplay[] = "ash.display";

display::DisplayManager* GetDisplayManager() {
  return Shell::Get()->display_manager();
}

base::string16 GetDisplayName(int64_t display_id) {
  return base::UTF8ToUTF16(
      GetDisplayManager()->GetDisplayNameForId(display_id));
}

base::string16 GetDisplaySize(int64_t display_id) {
  display::DisplayManager* display_manager = GetDisplayManager();

  // We don't show display size for mirrored display. Fallback
  // to empty string if this happens on release build.
  const display::DisplayIdList id_list =
      display_manager->GetMirroringDestinationDisplayIdList();
  const bool mirroring = display_manager->IsInMirrorMode() &&
                         base::ContainsValue(id_list, display_id);
  DCHECK(!mirroring);
  if (mirroring)
    return base::string16();

  const display::Display& display =
      display_manager->GetDisplayForId(display_id);
  DCHECK(display.is_valid());
  return base::UTF8ToUTF16(display.size().ToString());
}

// Callback to handle a user selecting the notification view.
void OnNotificationClicked(base::Optional<int> button_index) {
  DCHECK(!button_index);

  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DISPLAY_NOTIFICATION_SELECTED);
  // Settings may be blocked, e.g. at the lock screen.
  if (Shell::Get()->session_controller()->ShouldEnableSettings()) {
    Shell::Get()->system_tray_controller()->ShowDisplaySettings();
    Shell::Get()->metrics()->RecordUserMetricsAction(
        UMA_STATUS_AREA_DISPLAY_NOTIFICATION_SHOW_SETTINGS);
  }
  message_center::MessageCenter::Get()->RemoveNotification(
      ScreenLayoutObserver::kNotificationId, true /* by_user */);
}

// Returns the name of the currently connected external display whose ID is
// |external_display_id|. This should not be used when the external display is
// used for mirroring.
base::string16 GetExternalDisplayName(int64_t external_display_id) {
  DCHECK(!display::Display::IsInternalDisplayId(external_display_id));

  display::DisplayManager* display_manager = GetDisplayManager();
  DCHECK(!display_manager->IsInMirrorMode());

  if (external_display_id == display::kInvalidDisplayId)
    return l10n_util::GetStringUTF16(IDS_DISPLAY_NAME_UNKNOWN);

  // The external display name may have an annotation of "(width x height)" in
  // case that the display is rotated or its resolution is changed.
  base::string16 name = GetDisplayName(external_display_id);
  const display::ManagedDisplayInfo& display_info =
      display_manager->GetDisplayInfo(external_display_id);
  if (display_info.GetActiveRotation() != display::Display::ROTATE_0 ||
      display_info.configured_ui_scale() != 1.0f ||
      !display_info.overscan_insets_in_dip().IsEmpty()) {
    name =
        l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_ANNOTATED_NAME,
                                   name, GetDisplaySize(external_display_id));
  } else if (display_info.overscan_insets_in_dip().IsEmpty() &&
             display_info.has_overscan()) {
    name = l10n_util::GetStringFUTF16(
        IDS_ASH_STATUS_TRAY_DISPLAY_ANNOTATED_NAME, name,
        l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_DISPLAY_ANNOTATION_OVERSCAN));
  }

  return name;
}

// Returns true if docked mode is currently enabled.
bool IsDockedModeEnabled() {
  display::DisplayManager* display_manager = GetDisplayManager();
  if (!display::Display::HasInternalDisplay())
    return false;

  for (size_t i = 0; i < display_manager->GetNumDisplays(); ++i) {
    if (display::Display::IsInternalDisplayId(
            display_manager->GetDisplayAt(i).id())) {
      return false;
    }
  }

  // We have an internal display but it's not one of the active displays.
  return true;
}

// Returns the notification message that should be shown when mirror display
// mode is entered.
base::string16 GetEnterMirrorModeMessage() {
  DCHECK(GetDisplayManager()->IsInMirrorMode());
  if (display::Display::HasInternalDisplay()) {
    base::string16 display_names;
    for (auto& id :
         GetDisplayManager()->GetMirroringDestinationDisplayIdList()) {
      if (!display_names.empty())
        display_names.append(base::UTF8ToUTF16(","));
      display_names.append(GetDisplayName(id));
    }
    return l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING,
                                      display_names);
  }

  return l10n_util::GetStringUTF16(
      IDS_ASH_STATUS_TRAY_DISPLAY_MIRRORING_NO_INTERNAL);
}

// Returns the notification message that should be shown when unified desktop
// mode is entered.
base::string16 GetEnterUnifiedModeMessage() {
  return l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED);
}

// Returns the notification message that should be shown when unified desktop
// mode is exited.
base::string16 GetExitUnifiedModeMessage() {
  return l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_UNIFIED_EXITING);
}

base::string16 GetDisplayRemovedMessage(
    const display::ManagedDisplayInfo& removed_display_info,
    base::string16* out_additional_message) {
  return l10n_util::GetStringFUTF16(
      IDS_ASH_STATUS_TRAY_DISPLAY_REMOVED,
      base::UTF8ToUTF16(removed_display_info.name()));
}

base::string16 GetDisplayAddedMessage(int64_t added_display_id,
                                      base::string16* additional_message_out) {
  if (!display::Display::HasInternalDisplay()) {
    return l10n_util::GetStringUTF16(
        IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED_NO_INTERNAL);
  }

  return l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_EXTENDED,
                                    GetExternalDisplayName(added_display_id));
}

}  // namespace

const char ScreenLayoutObserver::kNotificationId[] =
    "chrome://settings/display";

ScreenLayoutObserver::ScreenLayoutObserver() {
  Shell::Get()->window_tree_host_manager()->AddObserver(this);
  UpdateDisplayInfo(nullptr);
}

ScreenLayoutObserver::~ScreenLayoutObserver() {
  Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
}

void ScreenLayoutObserver::SetDisplayChangedFromSettingsUI(int64_t display_id) {
  displays_changed_from_settings_ui_.insert(display_id);
}

void ScreenLayoutObserver::UpdateDisplayInfo(
    ScreenLayoutObserver::DisplayInfoMap* old_info) {
  if (old_info)
    old_info->swap(display_info_);
  display_info_.clear();

  display::DisplayManager* display_manager = GetDisplayManager();
  for (size_t i = 0; i < display_manager->GetNumDisplays(); ++i) {
    int64_t id = display_manager->GetDisplayAt(i).id();
    display_info_[id] = display_manager->GetDisplayInfo(id);
  }
}

bool ScreenLayoutObserver::GetDisplayMessageForNotification(
    const ScreenLayoutObserver::DisplayInfoMap& old_info,
    base::string16* out_message,
    base::string16* out_additional_message) {
  if (old_display_mode_ != current_display_mode_) {
    // Detect changes in the mirror mode status.
    if (current_display_mode_ == DisplayMode::MIRRORING) {
      *out_message = GetEnterMirrorModeMessage();
      return true;
    }
    if (old_display_mode_ == DisplayMode::MIRRORING &&
        GetExitMirrorModeMessage(out_message, out_additional_message)) {
      return true;
    }

    // Detect changes in the unified mode status.
    if (current_display_mode_ == DisplayMode::UNIFIED) {
      *out_message = GetEnterUnifiedModeMessage();
      return true;
    }
    if (old_display_mode_ == DisplayMode::UNIFIED) {
      *out_message = GetExitUnifiedModeMessage();
      return true;
    }

    if (current_display_mode_ == DisplayMode::DOCKED ||
        old_display_mode_ == DisplayMode::DOCKED) {
      // We no longer show any notification for docked mode events.
      // crbug.com/674719.
      return false;
    }
  }

  // Displays are added or removed.
  if (display_info_.size() < old_info.size()) {
    // A display has been removed.
    for (const auto& iter : old_info) {
      if (display_info_.count(iter.first))
        continue;

      *out_message =
          GetDisplayRemovedMessage(iter.second, out_additional_message);
      return true;
    }
  } else if (display_info_.size() > old_info.size()) {
    // A display has been added.
    for (const auto& iter : display_info_) {
      if (old_info.count(iter.first))
        continue;

      *out_message = GetDisplayAddedMessage(iter.first, out_additional_message);
      return true;
    }
  }

  for (const auto& iter : display_info_) {
    DisplayInfoMap::const_iterator old_iter = old_info.find(iter.first);
    if (old_iter == old_info.end()) {
      // The display's number is same but different displays. This happens
      // for the transition between docked mode and mirrored display.
      // This condition can never be reached here, since it is handled above.
      NOTREACHED() << "A display mode transition that should have been handled"
                      "earlier.";
      return false;
    }

    const auto ignore_display_iter =
        displays_changed_from_settings_ui_.find(iter.first);
    if (ignore_display_iter != displays_changed_from_settings_ui_.end()) {
      // Consume this state so that later changes are not affected.
      displays_changed_from_settings_ui_.erase(ignore_display_iter);
    } else {
      if ((iter.second.configured_ui_scale() !=
           old_iter->second.configured_ui_scale()) ||
          (GetDisplayManager()->IsInUnifiedMode() &&
           iter.second.size_in_pixel() != old_iter->second.size_in_pixel())) {
        *out_message = l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED_TITLE);
        *out_additional_message = l10n_util::GetStringFUTF16(
            IDS_ASH_STATUS_TRAY_DISPLAY_RESOLUTION_CHANGED,
            GetDisplayName(iter.first), GetDisplaySize(iter.first));
        return true;
      }
    }
    // Don't show rotation change notification if
    // a) no rotation change
    if (iter.second.GetActiveRotation() == old_iter->second.GetActiveRotation())
      continue;
    // b) the source is accelerometer.
    if (iter.second.active_rotation_source() ==
        display::Display::RotationSource::ACCELEROMETER) {
      continue;
    }
    // c) if the device is in tablet mode, and source is not user.
    if (Shell::Get()
            ->tablet_mode_controller()
            ->IsTabletModeWindowManagerEnabled() &&
        iter.second.active_rotation_source() !=
            display::Display::RotationSource::USER) {
      continue;
    }

    int rotation_text_id = 0;
    switch (iter.second.GetActiveRotation()) {
      case display::Display::ROTATE_0:
        rotation_text_id = IDS_ASH_STATUS_TRAY_DISPLAY_STANDARD_ORIENTATION;
        break;
      case display::Display::ROTATE_90:
        rotation_text_id = IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_90;
        break;
      case display::Display::ROTATE_180:
        rotation_text_id = IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_180;
        break;
      case display::Display::ROTATE_270:
        rotation_text_id = IDS_ASH_STATUS_TRAY_DISPLAY_ORIENTATION_270;
        break;
    }
    *out_additional_message = l10n_util::GetStringFUTF16(
        IDS_ASH_STATUS_TRAY_DISPLAY_ROTATED, GetDisplayName(iter.first),
        l10n_util::GetStringUTF16(rotation_text_id));
    return true;
  }

  // Found nothing special
  return false;
}

void ScreenLayoutObserver::CreateOrUpdateNotification(
    const base::string16& message,
    const base::string16& additional_message) {
  // Always remove the notification to make sure the notification appears
  // as a popup in any situation.
  message_center::MessageCenter::Get()->RemoveNotification(kNotificationId,
                                                           false /* by_user */);

  if (message.empty() && additional_message.empty())
    return;

  // Don't display notifications for accelerometer triggered screen rotations.
  // See http://crbug.com/364949
  if (Shell::Get()
          ->screen_orientation_controller()
          ->ignore_display_configuration_updates()) {
    return;
  }

  std::unique_ptr<Notification> notification =
      Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId, message,
          additional_message, gfx::Image(),
          base::string16(),  // display_source
          GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT, kNotifierDisplay),
          message_center::RichNotificationData(),
          new message_center::HandleNotificationClickDelegate(
              base::Bind(&OnNotificationClicked)),
          kNotificationScreenIcon,
          message_center::SystemNotificationWarningLevel::NORMAL);
  notification->set_priority(message_center::SYSTEM_PRIORITY);

  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DISPLAY_NOTIFICATION_CREATED);
  message_center::MessageCenter::Get()->AddNotification(
      std::move(notification));
}

void ScreenLayoutObserver::OnDisplayConfigurationChanged() {
  DisplayInfoMap old_info;
  UpdateDisplayInfo(&old_info);

  old_display_mode_ = current_display_mode_;
  if (GetDisplayManager()->IsInMirrorMode())
    current_display_mode_ = DisplayMode::MIRRORING;
  else if (GetDisplayManager()->IsInUnifiedMode())
    current_display_mode_ = DisplayMode::UNIFIED;
  else if (IsDockedModeEnabled())
    current_display_mode_ = DisplayMode::DOCKED;
  else if (GetDisplayManager()->GetNumDisplays() > 2)
    current_display_mode_ = DisplayMode::EXTENDED_3_PLUS;
  else if (GetDisplayManager()->GetNumDisplays() == 2)
    current_display_mode_ = DisplayMode::EXTENDED_2;
  else
    current_display_mode_ = DisplayMode::SINGLE;

  if (!show_notifications_for_testing_)
    return;

  base::string16 message;
  base::string16 additional_message;
  if (GetDisplayMessageForNotification(old_info, &message, &additional_message))
    CreateOrUpdateNotification(message, additional_message);
}

bool ScreenLayoutObserver::GetExitMirrorModeMessage(
    base::string16* out_message,
    base::string16* out_additional_message) {
  if (GetDisplayManager()->is_multi_mirroring_enabled()) {
    *out_message =
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT);
    return true;
  }
  switch (current_display_mode_) {
    case DisplayMode::EXTENDED_3_PLUS:
      // Mirror mode was turned off due to having more than two displays.
      // Show a message that mirror mode for 3+ displays is not supported.
      *out_message =
          l10n_util::GetStringUTF16(IDS_ASH_DISPLAY_MIRRORING_NOT_SUPPORTED);
      return true;

    case DisplayMode::DOCKED:
      // Handle disabling mirror mode as a result of going to docked mode
      // when we only have a single display (this means we actually have two
      // physical displays, one of which is the internal display, but they
      // were in mirror mode, and hence considered as one. Closing the
      // internal display disables mirror mode and we still have a single
      // active display).
      // Falls through.
    case DisplayMode::SINGLE:
      // We're exiting mirror mode because we removed one of the two
      // displays.
      *out_message =
          l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_DISPLAY_MIRROR_EXIT);
      return true;

    default:
      // Mirror mode was turned off; other messages should be shown e.g.
      // extended mode is on, ... etc.
      return false;
  }
}

}  // namespace ash
