// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_NOTIFICATION_MENU_MODEL_H_
#define UI_MESSAGE_CENTER_NOTIFICATION_MENU_MODEL_H_

#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/public/cpp/notification.h"

namespace message_center {

// The model of the context menu for a notification card.
class MESSAGE_CENTER_EXPORT NotificationMenuModel
    : public ui::SimpleMenuModel,
      public ui::SimpleMenuModel::Delegate {
 public:
  NotificationMenuModel(const Notification& notification);
  ~NotificationMenuModel() override;

  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  const std::string notification_id_;
  DISALLOW_COPY_AND_ASSIGN(NotificationMenuModel);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_UI_CONTROLLER_H_
