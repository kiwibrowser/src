// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_message_center_view.h"

#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_types.h"
#include "ui/message_center/views/message_view.h"
#include "ui/message_center/views/message_view_factory.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/scrollbar/overlay_scroll_bar.h"

using message_center::MessageCenter;
using message_center::MessageView;
using message_center::Notification;
using message_center::NotificationList;

namespace ash {

namespace {

const int kMaxVisibleNotifications = 100;

}  // namespace

// UnifiedMessageCenterView
// ///////////////////////////////////////////////////////////

UnifiedMessageCenterView::UnifiedMessageCenterView(
    MessageCenter* message_center)
    : message_center_(message_center),
      scroller_(new views::ScrollView()),
      message_list_view_(new MessageListView()) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  message_center_->AddObserver(this);
  set_notify_enter_exit_on_child(true);
  SetFocusBehavior(views::View::FocusBehavior::NEVER);

  // Need to set the transparent background explicitly, since ScrollView has
  // set the default opaque background color.
  scroller_->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroller_->SetVerticalScrollBar(new views::OverlayScrollBar(false));
  scroller_->SetHorizontalScrollBar(new views::OverlayScrollBar(true));
  AddChildView(scroller_);

  message_list_view_->set_use_fixed_height(false);
  message_list_view_->set_scroller(scroller_);
  scroller_->SetContents(message_list_view_);

  MessageView::SetSidebarEnabled();

  SetNotifications(message_center_->GetVisibleNotifications());
}

UnifiedMessageCenterView::~UnifiedMessageCenterView() {
  message_center_->RemoveObserver(this);
}

void UnifiedMessageCenterView::SetMaxHeight(int max_height) {
  scroller_->ClipHeightTo(0, max_height);
  Update();
}

void UnifiedMessageCenterView::SetNotifications(
    const NotificationList::Notifications& notifications) {
  int index = message_list_view_->GetNotificationCount();
  for (const Notification* notification : notifications) {
    if (index >= kMaxVisibleNotifications) {
      break;
    }

    AddNotificationAt(*notification, index++);
    message_center_->DisplayedNotification(
        notification->id(), message_center::DISPLAY_SOURCE_MESSAGE_CENTER);
  }

  Update();
}

void UnifiedMessageCenterView::Layout() {
  scroller_->SetBounds(0, 0, width(), height());
}

gfx::Size UnifiedMessageCenterView::CalculatePreferredSize() const {
  return scroller_->GetPreferredSize();
}

void UnifiedMessageCenterView::OnNotificationAdded(const std::string& id) {
  if (message_list_view_->GetNotificationCount() >= kMaxVisibleNotifications)
    return;

  int index = 0;
  const NotificationList::Notifications& notifications =
      message_center_->GetVisibleNotifications();
  for (const Notification* notification : notifications) {
    if (notification->id() == id) {
      AddNotificationAt(*notification, index);
      break;
    }
    ++index;
  }
  Update();
}

void UnifiedMessageCenterView::OnNotificationRemoved(const std::string& id,
                                                     bool by_user) {
  auto view_pair = message_list_view_->GetNotificationById(id);
  MessageView* view = view_pair.second;
  if (!view)
    return;

  message_list_view_->RemoveNotification(view);
  Update();
}

void UnifiedMessageCenterView::OnNotificationUpdated(const std::string& id) {
  UpdateNotification(id);
}

void UnifiedMessageCenterView::OnViewPreferredSizeChanged(
    views::View* observed_view) {
  DCHECK_EQ(std::string(MessageView::kViewClassName),
            observed_view->GetClassName());
  UpdateNotification(
      static_cast<MessageView*>(observed_view)->notification_id());
}

void UnifiedMessageCenterView::Update() {
  SetVisible(message_list_view_->GetNotificationCount() > 0);

  scroller_->Layout();
  PreferredSizeChanged();
}

void UnifiedMessageCenterView::AddNotificationAt(
    const Notification& notification,
    int index) {
  MessageView* view = message_center::MessageViewFactory::Create(
      notification, /*top-level=*/false);
  view->AddObserver(this);
  view->set_scroller(scroller_);
  message_list_view_->AddNotificationAt(view, index);
}

void UnifiedMessageCenterView::UpdateNotification(const std::string& id) {
  MessageView* view = message_list_view_->GetNotificationById(id).second;
  if (!view)
    return;

  Notification* notification = message_center_->FindVisibleNotificationById(id);
  if (!notification)
    return;

  int old_width = view->width();
  int old_height = view->height();
  bool old_pinned = view->GetPinned();
  message_list_view_->UpdateNotification(view, *notification);
  if (view->GetHeightForWidth(old_width) != old_height ||
      view->GetPinned() != old_pinned) {
    Update();
  }
}

}  // namespace ash
