// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/message_view_context_menu_controller.h"

#include "ui/base/models/menu_model.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/views/message_view.h"
#include "ui/message_center/views/notification_menu_model.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace message_center {

MessageViewContextMenuController::MessageViewContextMenuController() = default;

MessageViewContextMenuController::~MessageViewContextMenuController() = default;

void MessageViewContextMenuController::ShowContextMenuForView(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  // Assumes that the target view has to be MessageView.
  MessageView* message_view = static_cast<MessageView*>(source);
  Notification* notification =
      MessageCenter::Get()->FindVisibleNotificationById(
          message_view->notification_id());

  // Notification is null if the notification view is being removed or some
  // invalid status. In this case, just returns.
  if (!notification)
    return;

  menu_model_ = std::make_unique<NotificationMenuModel>(*notification);

  if (!menu_model_ || menu_model_->GetItemCount() == 0)
    return;

  menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_model_.get(), views::MenuRunner::HAS_MNEMONICS,
      base::Bind(&MessageViewContextMenuController::OnMenuClosed,
                 base::Unretained(this)));

  menu_runner_->RunMenuAt(source->GetWidget()->GetTopLevelWidget(), NULL,
                          gfx::Rect(point, gfx::Size()),
                          views::MENU_ANCHOR_TOPRIGHT, source_type);
}

void MessageViewContextMenuController::OnMenuClosed() {
  menu_runner_.reset();
  menu_model_.reset();
}

}  // namespace message_center
