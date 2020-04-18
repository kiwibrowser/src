// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_MESSAGE_POPUP_COLLECTION_H_
#define UI_MESSAGE_CENTER_VIEWS_MESSAGE_POPUP_COLLECTION_H_

#include <stddef.h>

#include <list>
#include <map>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/message_center_observer.h"
#include "ui/message_center/views/toast_contents_view.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_observer.h"

namespace display {
class Display;
}

namespace views {
class Widget;
}

namespace message_center {
namespace test {
class MessagePopupCollectionTest;
}

class MessageCenter;
class UiController;
class MessageViewContextMenuController;
class PopupAlignmentDelegate;

// Container for popup toasts. Because each toast is a frameless window rather
// than a view in a bubble, now the container just manages all of those toasts.
// This is similar to chrome/browser/notifications/balloon_collection, but the
// contents of each toast are for the message center and layout strategy would
// be slightly different.
class MESSAGE_CENTER_EXPORT MessagePopupCollection
    : public MessageCenterObserver,
      public views::ViewObserver {
 public:
  MessagePopupCollection(MessageCenter* message_center,
                         UiController* tray,
                         PopupAlignmentDelegate* alignment_delegate);
  ~MessagePopupCollection() override;

  // Overridden from views::ViewObserver:
  void OnViewPreferredSizeChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  void MarkAllPopupsShown();

  // Inclement the timer counter, and pause the popup timer if necessary.
  void PausePopupTimers();
  // Declement the timer counter, and restart the popup timer if necessary.
  void RestartPopupTimers();

  // Since these events are really coming from individual toast widgets,
  // it helps to be able to keep track of the sender.
  void OnMouseEntered(ToastContentsView* toast_entered);
  void OnMouseExited(ToastContentsView* toast_exited);

  // Runs the next step in update/animate sequence.
  void DoUpdate();

  // Removes the toast from our internal list of toasts; this is called when the
  // toast is irrevocably closed (such as within RemoveToast).
  void ForgetToast(ToastContentsView* toast);

  // Called when the display bounds has been changed. Used in Windows only.
  void OnDisplayMetricsChanged(const display::Display& display);

 private:
  friend class test::MessagePopupCollectionTest;
  typedef std::list<ToastContentsView*> Toasts;

  // Iterates toasts and starts closing them.
  std::set<std::string> CloseAllWidgets();

  // Called by ToastContentsView when its window is closed.
  void RemoveToast(ToastContentsView* toast, bool mark_as_shown);

  // Creates new widgets for new toast notifications, and updates |toasts_| and
  // |widgets_| correctly.
  void UpdateWidgets();

  // Repositions all of the widgets based on the current work area.
  void RepositionWidgets();

  // The baseline is an (imaginary) line that would touch the bottom of the
  // next created notification if bottom-aligned or its top if top-aligned.
  int GetBaseline() const;

  // Returns the top of the toast when IsTopDown() is true, otherwise returns
  // the bottom of the toast.
  int GetBaselineForToast(ToastContentsView* toast) const;

  // Overridden from MessageCenterObserver:
  void OnNotificationAdded(const std::string& notification_id) override;
  void OnNotificationRemoved(const std::string& notification_id,
                             bool by_user) override;
  void OnNotificationUpdated(const std::string& notification_id) override;

  ToastContentsView* FindToast(const std::string& notification_id) const;

  // While the toasts are animated, avoid updating the collection, to reduce
  // user confusion. Instead, update the collection when all animations are
  // done. This method is run when defer counter is zero, may initiate next
  // update/animation step.
  void OnDeferTimerExpired();

  // "ForTest" methods.
  views::Widget* GetWidgetForTest(const std::string& id) const;
  gfx::Rect GetToastRectAt(size_t index) const;

  MessageCenter* message_center_;
  UiController* tray_;
  Toasts toasts_;

  PopupAlignmentDelegate* alignment_delegate_;

  // This is only used to compare with incoming events, do not assume that
  // the toast will be valid if this pointer is non-NULL.
  ToastContentsView* latest_toast_entered_ = nullptr;

  // This is the number of pause request for timer. If it's more than zero, the
  // timer is paused. If zero, the timer is not paused.
  int timer_pause_counter_ = 0;

  std::unique_ptr<MessageViewContextMenuController> context_menu_controller_;

  ScopedObserver<views::View, views::ViewObserver> observed_views_{this};

  // Gives out weak pointers to toast contents views which have an unrelated
  // lifetime.  Must remain the last member variable.
  base::WeakPtrFactory<MessagePopupCollection> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(MessagePopupCollection);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_MESSAGE_POPUP_COLLECTION_H_
