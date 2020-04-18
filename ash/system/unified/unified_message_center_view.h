// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_MESSAGE_CENTER_VIEW_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_MESSAGE_CENTER_VIEW_H_

#include <stddef.h>

#include "ash/ash_export.h"
#include "ash/message_center/message_list_view.h"
#include "base/macros.h"
#include "ui/message_center/message_center_observer.h"
#include "ui/message_center/notification_list.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace message_center {

class MessageCenter;

}  // namespace message_center

namespace ash {

// Container for message list view. Acts as a controller/delegate of message
// list view, passing data back and forth to message center.
class ASH_EXPORT UnifiedMessageCenterView
    : public views::View,
      public message_center::MessageCenterObserver,
      public views::ViewObserver {
 public:
  explicit UnifiedMessageCenterView(
      message_center::MessageCenter* message_center);
  ~UnifiedMessageCenterView() override;

  // Set the maximum height that the view can take.
  void SetMaxHeight(int max_height);

 protected:
  void SetNotifications(
      const message_center::NotificationList::Notifications& notifications);

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

  // message_center::MessageCenterObserver:
  void OnNotificationAdded(const std::string& id) override;
  void OnNotificationRemoved(const std::string& id, bool by_user) override;
  void OnNotificationUpdated(const std::string& id) override;

  // views::ViewObserver:
  void OnViewPreferredSizeChanged(views::View* observed_view) override;

 private:
  void Update();
  void AddNotificationAt(const message_center::Notification& notification,
                         int index);
  void UpdateNotification(const std::string& notification_id);

  message_center::MessageCenter* message_center_;

  views::ScrollView* const scroller_;
  MessageListView* const message_list_view_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedMessageCenterView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_MESSAGE_CENTER_VIEW_H_
