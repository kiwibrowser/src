// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_H_
#define UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/message_center/views/slide_out_controller.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/view.h"

namespace views {
class Painter;
class ScrollView;
}

namespace message_center {

namespace test {
class MessagePopupCollectionTest;
}

class Notification;
class NotificationControlButtonsView;

// An base class for a notification entry. Contains background and other
// elements shared by derived notification views.
class MESSAGE_CENTER_EXPORT MessageView : public views::InkDropHostView,
                                          public SlideOutController::Delegate {
 public:
  static const char kViewClassName[];

  // Notify this notification view is in the sidebar. This is necessary until
  // removing the experimental flag for Sidebar, since the flag exists in Ash,
  // so we don't refer the flag directly. Some layout and behavior may change by
  // this flag.
  // TODO(yoshiki, tetsui): Remove this after removing the flag for Sidebar.
  static void SetSidebarEnabled();

  explicit MessageView(const Notification& notification);
  ~MessageView() override;

  // Updates this view with the new data contained in the notification.
  virtual void UpdateWithNotification(const Notification& notification);

  // Creates a shadow around the notification and changes slide-out behavior.
  void SetIsNested();

  bool IsCloseButtonFocused() const;
  void RequestFocusOnCloseButton();

  virtual NotificationControlButtonsView* GetControlButtonsView() const = 0;
  virtual void UpdateControlButtonsVisibility() = 0;

  virtual void SetExpanded(bool expanded);
  virtual bool IsExpanded() const;
  virtual bool IsAutoExpandingAllowed() const;
  virtual bool IsManuallyExpandedOrCollapsed() const;
  virtual void SetManuallyExpandedOrCollapsed(bool value);

  // Invoked when the container view of MessageView (e.g. MessageCenterView in
  // ash) is starting the animation that possibly hides some part of
  // the MessageView.
  // During the animation, MessageView should comply with the Z order in views.
  virtual void OnContainerAnimationStarted();
  virtual void OnContainerAnimationEnded();

  void OnCloseButtonPressed();
  virtual void OnSettingsButtonPressed(const ui::Event& event);

  // views::View
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;
  void Layout() override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  const char* GetClassName() const final;

  // message_center::SlideOutController::Delegate
  ui::Layer* GetSlideOutLayer() override;
  void OnSlideChanged() override;
  void OnSlideOut() override;

  bool GetPinned() const;

  void set_scroller(views::ScrollView* scroller) { scroller_ = scroller; }
  std::string notification_id() const { return notification_id_; }

 protected:
  // Creates and add close button to view hierarchy when necessary. Derived
  // classes should call this after its view hierarchy is populated to ensure
  // it is on top of other views.
  void CreateOrUpdateCloseButtonView(const Notification& notification);

  // Changes the background color being used by |background_view_| and schedules
  // a paint.
  virtual void SetDrawBackgroundAsActive(bool active);

  views::View* background_view() { return background_view_; }
  views::ScrollView* scroller() { return scroller_; }

  bool is_nested() const { return is_nested_; }

 private:
  friend class test::MessagePopupCollectionTest;

  std::string notification_id_;
  views::View* background_view_ = nullptr;  // Owned by views hierarchy.
  views::ScrollView* scroller_ = nullptr;

  base::string16 accessible_name_;

  // Flag if the notification is set to pinned or not.
  bool pinned_ = false;

  std::unique_ptr<views::Painter> focus_painter_;

  message_center::SlideOutController slide_out_controller_;

  // True if |this| is embedded in another view. Equivalent to |!top_level| in
  // MessageViewFactory parlance.
  bool is_nested_ = false;

  DISALLOW_COPY_AND_ASSIGN(MessageView);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_VIEWS_MESSAGE_VIEW_H_
