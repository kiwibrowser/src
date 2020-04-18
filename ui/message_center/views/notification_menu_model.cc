// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/notification_menu_model.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/message_center/message_center.h"
#include "ui/strings/grit/ui_strings.h"

namespace message_center {

namespace {

// Menu constants
const int kTogglePermissionCommand = 0;

}  // namespace

NotificationMenuModel::NotificationMenuModel(const Notification& notification)
    : ui::SimpleMenuModel(this), notification_id_(notification.id()) {
  DCHECK(!notification.display_source().empty());
  AddItem(kTogglePermissionCommand,
          l10n_util::GetStringFUTF16(IDS_MESSAGE_CENTER_NOTIFIER_DISABLE,
                                     notification.display_source()));
}

NotificationMenuModel::~NotificationMenuModel() {}

bool NotificationMenuModel::IsCommandIdChecked(int command_id) const {
  return false;
}

bool NotificationMenuModel::IsCommandIdEnabled(int command_id) const {
  // TODO(estade): commands shouldn't always be enabled. For example, a
  // notification's enabled state might be controlled by policy. See
  // http://crbug.com/771269
  return true;
}

void NotificationMenuModel::ExecuteCommand(int command_id, int event_flags) {
  DCHECK_EQ(command_id, kTogglePermissionCommand);
  MessageCenter::Get()->DisableNotification(notification_id_);
}

}  // namespace message_center
