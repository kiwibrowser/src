// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/message_popup_collection.h"

#include <set>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification_list.h"
#include "ui/message_center/public/cpp/features.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/ui_controller.h"
#include "ui/message_center/views/message_view.h"
#include "ui/message_center/views/message_view_context_menu_controller.h"
#include "ui/message_center/views/message_view_factory.h"
#include "ui/message_center/views/popup_alignment_delegate.h"
#include "ui/message_center/views/toast_contents_view.h"
#include "ui/views/background.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace message_center {
namespace {

// The margin between messages (and between the anchor unless
// first_item_has_no_margin was specified).
const int kToastMarginY = kMarginBetweenPopups;

}  // namespace.

MessagePopupCollection::MessagePopupCollection(
    MessageCenter* message_center,
    UiController* tray,
    PopupAlignmentDelegate* alignment_delegate)
    : message_center_(message_center),
      tray_(tray),
      alignment_delegate_(alignment_delegate) {
  DCHECK(message_center_);
  message_center_->AddObserver(this);
  alignment_delegate_->set_collection(this);
#if !defined(OS_CHROMEOS)
  if (!base::FeatureList::IsEnabled(message_center::kNewStyleNotifications))
    context_menu_controller_.reset(new MessageViewContextMenuController());
#endif
}

MessagePopupCollection::~MessagePopupCollection() {
  weak_factory_.InvalidateWeakPtrs();

  message_center_->RemoveObserver(this);

  CloseAllWidgets();
}

void MessagePopupCollection::OnViewPreferredSizeChanged(
    views::View* observed_view) {
  OnNotificationUpdated(
      static_cast<MessageView*>(observed_view)->notification_id());
}

void MessagePopupCollection::OnViewIsDeleting(views::View* observed_view) {
  observed_views_.Remove(observed_view);
}

void MessagePopupCollection::MarkAllPopupsShown() {
  std::set<std::string> closed_ids = CloseAllWidgets();
  for (std::set<std::string>::iterator iter = closed_ids.begin();
       iter != closed_ids.end(); iter++) {
    message_center_->MarkSinglePopupAsShown(*iter, false);
  }
}

void MessagePopupCollection::PausePopupTimers() {
  DCHECK(timer_pause_counter_ >= 0);
  if (timer_pause_counter_ <= 0) {
    message_center_->PausePopupTimers();
    timer_pause_counter_ = 1;
  } else {
    timer_pause_counter_++;
  }
}

void MessagePopupCollection::RestartPopupTimers() {
  DCHECK(timer_pause_counter_ >= 1);
  if (timer_pause_counter_ <= 1) {
    message_center_->RestartPopupTimers();
    timer_pause_counter_ = 0;
  } else {
    timer_pause_counter_--;
  }
}

void MessagePopupCollection::UpdateWidgets() {
  if (message_center_->IsMessageCenterVisible()) {
    DCHECK_EQ(0u, message_center_->GetPopupNotifications().size());
    return;
  }

  NotificationList::PopupNotifications popups =
      message_center_->GetPopupNotifications();
  if (popups.empty()) {
    CloseAllWidgets();
    return;
  }

  bool top_down = alignment_delegate_->IsTopDown();
  int base = GetBaseline();
#if defined(OS_CHROMEOS)
  bool is_primary_display =
      alignment_delegate_->IsPrimaryDisplayForNotification();
#endif

  // Check if the popups contain a new notification.
  bool has_new_toasts = false;
  for (auto* popup : popups) {
    if (!FindToast(popup->id())) {
      has_new_toasts = true;
      break;
    }
  }

  // If a new notification is found, collapse all existing notifications
  // beforehand.
  if (has_new_toasts) {
    for (Toasts::const_iterator iter = toasts_.begin();
         iter != toasts_.end();) {
      // SetExpanded() may fire PreferredSizeChanged(), which may end up
      // removing the toast in OnNotificationUpdated(). So we have to increment
      // the iterator in a way that is safe even if the current iterator is
      // invalidated during the loop.
      MessageView* view = (*iter++)->message_view();
      if (view->IsMouseHovered() || view->IsManuallyExpandedOrCollapsed())
        continue;
      view->SetExpanded(false);
    }
  }

  // Iterate in the reverse order to keep the oldest toasts on screen. Newer
  // items may be ignored if there are no room to place them.
  for (NotificationList::PopupNotifications::const_reverse_iterator iter =
           popups.rbegin(); iter != popups.rend(); ++iter) {
    const Notification& notification = *(*iter);
    if (FindToast(notification.id()))
      continue;

#if defined(OS_CHROMEOS)
    // Disables popup of custom notification on non-primary displays, since
    // currently custom notification supports only on one display at the same
    // time.
    // TODO(yoshiki): Support custom popup notification on multiple display
    // (crbug.com/715370).
    if (!is_primary_display && notification.type() == NOTIFICATION_TYPE_CUSTOM)
      continue;
#endif

    // Create top-level notification.
    MessageView* view = MessageViewFactory::Create(notification, true);
    observed_views_.Add(view);
    if (!view->IsManuallyExpandedOrCollapsed())
      view->SetExpanded(view->IsAutoExpandingAllowed());

#if !defined(OS_CHROMEOS)
    view->set_context_menu_controller(context_menu_controller_.get());
#endif

    int view_height = ToastContentsView::GetToastSizeForView(view).height();
    int height_available =
        top_down ? alignment_delegate_->GetWorkArea().bottom() - base
                 : base - alignment_delegate_->GetWorkArea().y();

    if (height_available - view_height - kToastMarginY < 0) {
      delete view;
      break;
    }

    ToastContentsView* toast = new ToastContentsView(
        notification.id(), alignment_delegate_, weak_factory_.GetWeakPtr());

    const RichNotificationData& optional_fields =
        notification.rich_notification_data();
    bool a11y_feedback_for_updates =
        optional_fields.should_make_spoken_feedback_for_popup_updates;

    // There will be no contents already since this is a new ToastContentsView.
    toast->SetContents(view, a11y_feedback_for_updates);
    toasts_.push_back(toast);

    gfx::Size preferred_size = toast->GetPreferredSize();
    gfx::Point origin(
        alignment_delegate_->GetToastOriginX(gfx::Rect(preferred_size)), base);
    // The toast slides in from the edge of the screen horizontally.
    if (alignment_delegate_->IsFromLeft())
      origin.set_x(origin.x() - preferred_size.width());
    else
      origin.set_x(origin.x() + preferred_size.width());
    if (top_down)
      origin.set_y(origin.y() + view_height);

    toast->RevealWithAnimation(origin);

    // Shift the base line to be a few pixels above the last added toast or (few
    // pixels below last added toast if top-aligned).
    if (top_down)
      base += view_height + kToastMarginY;
    else
      base -= view_height + kToastMarginY;

    if (views::ViewsDelegate::GetInstance()) {
      views::ViewsDelegate::GetInstance()->NotifyAccessibilityEvent(
          toast, ax::mojom::Event::kAlert);
    }

    message_center_->DisplayedNotification(notification.id(),
                                           DISPLAY_SOURCE_POPUP);
  }
}

void MessagePopupCollection::OnMouseEntered(ToastContentsView* toast_entered) {
  // Sometimes we can get two MouseEntered/MouseExited in a row when animating
  // toasts.  So we need to keep track of which one is the currently active one.
  latest_toast_entered_ = toast_entered;

  PausePopupTimers();
}

void MessagePopupCollection::OnMouseExited(ToastContentsView* toast_exited) {
  // If we're exiting a toast after entering a different toast, then ignore
  // this mouse event.
  if (toast_exited != latest_toast_entered_)
    return;
  latest_toast_entered_ = NULL;

  RestartPopupTimers();
}

std::set<std::string> MessagePopupCollection::CloseAllWidgets() {
  std::set<std::string> closed_toast_ids;

  while (!toasts_.empty()) {
    ToastContentsView* toast = toasts_.front();
    toasts_.pop_front();
    closed_toast_ids.insert(toast->id());

    OnMouseExited(toast);

    // CloseWithAnimation will cause the toast to forget about |this| so it is
    // required when we forget a toast.
    toast->CloseWithAnimation();
  }

  return closed_toast_ids;
}

void MessagePopupCollection::ForgetToast(ToastContentsView* toast) {
  toasts_.remove(toast);
  OnMouseExited(toast);
}

void MessagePopupCollection::RemoveToast(ToastContentsView* toast,
                                         bool mark_as_shown) {
  ForgetToast(toast);

  toast->CloseWithAnimation();

  if (mark_as_shown)
    message_center_->MarkSinglePopupAsShown(toast->id(), false);
}

void MessagePopupCollection::RepositionWidgets() {
  bool top_down = alignment_delegate_->IsTopDown();
  // We don't want to position relative to last toast - we want re-position.
  int base = alignment_delegate_->GetBaseline();

  for (Toasts::const_iterator iter = toasts_.begin(); iter != toasts_.end();) {
    Toasts::const_iterator curr = iter++;
    gfx::Rect bounds((*curr)->bounds());
    bounds.set_x(alignment_delegate_->GetToastOriginX(bounds));
    bounds.set_y(top_down ? base : base - bounds.height());

    // The notification may scrolls the boundary of the screen due to image
    // load and such notifications should disappear. Do not call
    // CloseWithAnimation, we don't want to show the closing animation, and we
    // don't want to mark such notifications as shown. See crbug.com/233424
    if ((top_down
             ? alignment_delegate_->GetWorkArea().bottom() - bounds.bottom()
             : bounds.y() - alignment_delegate_->GetWorkArea().y()) >= 0)
      (*curr)->SetBoundsWithAnimation(bounds);
    else
      RemoveToast(*curr, /*mark_as_shown=*/false);

    // Shift the base line to be a few pixels above the last added toast or (few
    // pixels below last added toast if top-aligned).
    if (top_down)
      base += bounds.height() + kToastMarginY;
    else
      base -= bounds.height() + kToastMarginY;
  }
}

int MessagePopupCollection::GetBaseline() const {
  if (toasts_.empty())
    return alignment_delegate_->GetBaseline();

  if (alignment_delegate_->IsTopDown())
    return toasts_.back()->bounds().bottom() + kToastMarginY;

  return toasts_.back()->origin().y() - kToastMarginY;
}

int MessagePopupCollection::GetBaselineForToast(
    ToastContentsView* toast) const {
  if (alignment_delegate_->IsTopDown())
    return toast->bounds().y();
  else
    return toast->bounds().bottom();
}

void MessagePopupCollection::OnNotificationAdded(
    const std::string& notification_id) {
  DoUpdate();
}

void MessagePopupCollection::OnNotificationRemoved(
    const std::string& notification_id,
    bool by_user) {
  // Find a toast.
  Toasts::const_iterator iter = toasts_.begin();
  for (; iter != toasts_.end(); ++iter) {
    if ((*iter)->id() == notification_id)
      break;
  }
  if (iter == toasts_.end())
    return;

  RemoveToast(*iter, /*mark_as_shown=*/true);

  DoUpdate();
}

void MessagePopupCollection::OnNotificationUpdated(
    const std::string& notification_id) {
  // Find a toast.
  Toasts::const_iterator toast_iter = toasts_.begin();
  for (; toast_iter != toasts_.end(); ++toast_iter) {
    if ((*toast_iter)->id() == notification_id)
      break;
  }
  if (toast_iter == toasts_.end())
    return;

  NotificationList::PopupNotifications notifications =
      message_center_->GetPopupNotifications();
  bool updated = false;

  for (NotificationList::PopupNotifications::iterator iter =
           notifications.begin(); iter != notifications.end(); ++iter) {
    Notification* notification = *iter;
    DCHECK(notification);
    ToastContentsView* toast_contents_view = *toast_iter;
    DCHECK(toast_contents_view);

    if (notification->id() != notification_id)
      continue;

    const RichNotificationData& optional_fields =
        notification->rich_notification_data();
    bool a11y_feedback_for_updates =
        optional_fields.should_make_spoken_feedback_for_popup_updates;

    toast_contents_view->UpdateContents(*notification,
                                        a11y_feedback_for_updates);

    updated = true;
  }

  // OnNotificationUpdated() can be called when a notification is excluded from
  // the popup notification list but still remains in the full notification
  // list. In that case the widget for the notification has to be closed here.
  if (!updated)
    RemoveToast(*toast_iter, /*mark_as_shown=*/true);

  DoUpdate();
}

ToastContentsView* MessagePopupCollection::FindToast(
    const std::string& notification_id) const {
  for (Toasts::const_iterator iter = toasts_.begin(); iter != toasts_.end();
       ++iter) {
    if ((*iter)->id() == notification_id)
      return *iter;
  }
  return NULL;
}

// This is the main sequencer of tasks. It does a step, then waits for
// all started transitions to play out before doing the next step.
// First, remove all expired toasts.
// Then, reposition widgets.
// Then, see if there is vacant space for new toasts.
void MessagePopupCollection::DoUpdate() {
  RepositionWidgets();

  // Reposition could create extra space which allows additional widgets.
  UpdateWidgets();
}

void MessagePopupCollection::OnDisplayMetricsChanged(
    const display::Display& display) {
  alignment_delegate_->RecomputeAlignment(display);
}

views::Widget* MessagePopupCollection::GetWidgetForTest(const std::string& id)
    const {
  for (Toasts::const_iterator iter = toasts_.begin(); iter != toasts_.end();
       ++iter) {
    if ((*iter)->id() == id)
      return (*iter)->GetWidget();
  }
  return NULL;
}

gfx::Rect MessagePopupCollection::GetToastRectAt(size_t index) const {
  size_t i = 0;
  for (Toasts::const_iterator iter = toasts_.begin(); iter != toasts_.end();
       ++iter) {
    if (i++ == index) {
      views::Widget* widget = (*iter)->GetWidget();
      if (widget)
        return widget->GetWindowBoundsInScreen();
      break;
    }
  }
  return gfx::Rect();
}

}  // namespace message_center
