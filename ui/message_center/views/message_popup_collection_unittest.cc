// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/message_popup_collection.h"

#include <stddef.h>

#include <algorithm>
#include <list>
#include <memory>
#include <numeric>
#include <utility>

#include "base/optional.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/display.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/message_center/fake_message_center.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/views/desktop_popup_alignment_delegate.h"
#include "ui/message_center/views/toast_contents_view.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace message_center {

namespace {

std::unique_ptr<Notification> CreateTestNotification(std::string id,
                                                     std::string text) {
  return std::make_unique<Notification>(
      NOTIFICATION_TYPE_BASE_FORMAT, id, base::UTF8ToUTF16("test title"),
      base::ASCIIToUTF16(text), gfx::Image(),
      base::string16() /* display_source */, GURL(),
      NotifierId(NotifierId::APPLICATION, id), RichNotificationData(),
      new NotificationDelegate());
}

// Provides an aura window context for widget creation.
class TestPopupAlignmentDelegate : public DesktopPopupAlignmentDelegate {
 public:
  explicit TestPopupAlignmentDelegate(gfx::NativeWindow context)
      : context_(context) {}
  ~TestPopupAlignmentDelegate() override {}

  // PopupAlignmentDelegate:
  void ConfigureWidgetInitParamsForContainer(
      views::Widget* widget,
      views::Widget::InitParams* init_params) override {
    init_params->context = context_;
  }

 private:
  gfx::NativeWindow context_;

  DISALLOW_COPY_AND_ASSIGN(TestPopupAlignmentDelegate);
};

}  // namespace

namespace test {

class MessagePopupCollectionTest : public views::ViewsTestBase,
                                   public views::WidgetObserver {
 public:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    MessageCenter::Initialize();
    MessageCenter::Get()->DisableTimersForTest();
    alignment_delegate_.reset(new TestPopupAlignmentDelegate(GetContext()));
    collection_.reset(new MessagePopupCollection(MessageCenter::Get(), NULL,
                                                 alignment_delegate_.get()));
    // This size fits test machines resolution and also can keep a few toasts
    // w/o ill effects of hitting the screen overflow. This allows us to assume
    // and verify normal layout of the toast stack.
    SetDisplayInfo(gfx::Rect(0, 0, 1920, 1070),  // taskbar at the bottom.
                   gfx::Rect(0, 0, 1920, 1080));
    id_ = 0;
  }

  void TearDown() override {
    collection_.reset();
    MessageCenter::Shutdown();
    views::ViewsTestBase::TearDown();
  }

 protected:
  MessagePopupCollection* collection() { return collection_.get(); }

  size_t GetToastCounts() {
    return collection_->toasts_.size();
  }

  bool MouseInCollection() {
    return collection_->latest_toast_entered_ != NULL;
  }

  bool IsToastShown(const std::string& id) {
    views::Widget* widget = collection_->GetWidgetForTest(id);
    return widget && widget->IsVisible();
  }

  views::Widget* GetWidget(const std::string& id) {
    return collection_->GetWidgetForTest(id);
  }

  void SetDisplayInfo(const gfx::Rect& work_area,
                      const gfx::Rect& display_bounds) {
    display::Display dummy_display;
    dummy_display.set_bounds(display_bounds);
    dummy_display.set_work_area(work_area);
    alignment_delegate_->RecomputeAlignment(dummy_display);
  }

  gfx::Rect GetWorkArea() {
    return alignment_delegate_->work_area_;
  }

  ToastContentsView* GetToast(const std::string& id) {
    for (MessagePopupCollection::Toasts::iterator iter =
             collection_->toasts_.begin();
         iter != collection_->toasts_.end(); ++iter) {
      if ((*iter)->id() == id)
        return *iter;
    }
    return NULL;
  }

  std::string AddNotification() {
    std::string id = base::IntToString(id_++);
    std::unique_ptr<Notification> notification = std::make_unique<Notification>(
        NOTIFICATION_TYPE_BASE_FORMAT, id, base::UTF8ToUTF16("test title"),
        base::UTF8ToUTF16("test message"), gfx::Image(),
        base::string16() /* display_source */, GURL(), NotifierId(),
        RichNotificationData(), new NotificationDelegate());
    MessageCenter::Get()->AddNotification(std::move(notification));
    return id;
  }

  std::string AddImageNotification() {
    std::string id = base::IntToString(id_++);
    std::unique_ptr<Notification> notification = std::make_unique<Notification>(
        NOTIFICATION_TYPE_IMAGE, id, base::UTF8ToUTF16("test title"),
        base::UTF8ToUTF16("test message"), gfx::Image(),
        base::string16() /* display_source */, GURL(), NotifierId(),
        RichNotificationData(), new NotificationDelegate());
    notification->set_image(gfx::test::CreateImage(100, 100));
    MessageCenter::Get()->AddNotification(std::move(notification));
    return id;
  }

  void CloseAllToasts() {
    // Assumes there is at least one toast to close.
    EXPECT_TRUE(GetToastCounts() > 0);

    auto toasts = collection_->toasts_;
    for (ToastContentsView* toast : toasts) {
      toast->GetWidget()->CloseNow();
    }
  }

  gfx::Rect GetToastRectAt(size_t index) {
    return collection_->GetToastRectAt(index);
  }

  void RemoveToastAndWaitForClose(const std::string& id) {
    GetWidget(id)->AddObserver(this);
    MessageCenter::Get()->RemoveNotification(id, true /* by_user */);
    widget_close_run_loop_.Run();
  }

  // views::WidgetObserver
  void OnWidgetDestroyed(views::Widget* widget) override {
    widget_close_run_loop_.Quit();
  }

  // Checks:
  //  1) sizes of toast and corresponding widget are equal;
  //  2) widgets do not owerlap;
  //  3) after animation is done, aligment is propper;
  class CheckedAnimationDelegate : public gfx::AnimationDelegate {
   public:
    explicit CheckedAnimationDelegate(MessagePopupCollectionTest* test);
    ~CheckedAnimationDelegate() override;

    // returns first encountered error
    const base::Optional<std::string>& error_msg() const { return error_msg_; }

    // gfx::AnimationDelegate overrides
    void AnimationEnded(const gfx::Animation* animation) override;
    void AnimationProgressed(const gfx::Animation* animation) override;
    void AnimationCanceled(const gfx::Animation* animation) override;

   private:
    // we attach ourselves to the last toast, because we accept
    // invalidation of invariants during intermidiate
    // notification updates.
    ToastContentsView& animation_delegate() { return *toasts_->back(); }
    const ToastContentsView& animation_delegate() const {
      return *toasts_->back();
    }

    void CheckWidgetLEView(const std::string& calling_func);
    void CheckToastsAreAligned(const std::string& calling_func);
    void CheckToastsDontOverlap(const std::string& calling_func);

    static int ComputeYDistance(const ToastContentsView& top,
                                const ToastContentsView& bottom);

    MessagePopupCollection::Toasts* toasts_;

    // first encountered error
    base::Optional<std::string> error_msg_;

    DISALLOW_COPY_AND_ASSIGN(CheckedAnimationDelegate);
  };

  static std::string YPositionsToString(
      const MessagePopupCollection::Toasts& toasts);

 private:
  base::RunLoop widget_close_run_loop_;
  std::unique_ptr<MessagePopupCollection> collection_;
  std::unique_ptr<DesktopPopupAlignmentDelegate> alignment_delegate_;
  int id_;
};

MessagePopupCollectionTest::CheckedAnimationDelegate::CheckedAnimationDelegate(
    MessagePopupCollectionTest* test)
    : toasts_(&test->collection_->toasts_) {
  DCHECK(!toasts_->empty());
  animation_delegate().bounds_animation_->set_delegate(this);
}

MessagePopupCollectionTest::CheckedAnimationDelegate::
    ~CheckedAnimationDelegate() {
  animation_delegate().bounds_animation_->set_delegate(&animation_delegate());
}

void MessagePopupCollectionTest::CheckedAnimationDelegate::AnimationEnded(
    const gfx::Animation* animation) {
  animation_delegate().AnimationEnded(animation);
  CheckWidgetLEView("AnimationEnded");
  CheckToastsAreAligned("AnimationEnded");
}

void MessagePopupCollectionTest::CheckedAnimationDelegate::AnimationProgressed(
    const gfx::Animation* animation) {
  animation_delegate().AnimationProgressed(animation);
  CheckWidgetLEView("AnimationProgressed");
  CheckToastsDontOverlap("AnimationProgressed");
}

void MessagePopupCollectionTest::CheckedAnimationDelegate::AnimationCanceled(
    const gfx::Animation* animation) {
  animation_delegate().AnimationCanceled(animation);
  CheckWidgetLEView("AnimationCanceled");
  CheckToastsDontOverlap("AnimationCanceled");
}

void MessagePopupCollectionTest::CheckedAnimationDelegate::CheckWidgetLEView(
    const std::string& calling_func) {
  if (error_msg_)
    return;
  for (ToastContentsView* toast : *toasts_) {
    auto* widget = toast->GetWidget();
    DCHECK(widget) << "no widget for: " << toast->id();
    if (toast->bounds().height() < widget->GetWindowBoundsInScreen().height()) {
      error_msg_ = calling_func + " CheckWidgetSizeLEView: id: " + toast->id() +
                   "\ntoast size: " + toast->bounds().size().ToString() +
                   "\nwidget size: " +
                   widget->GetWindowBoundsInScreen().size().ToString();
      return;
    }
  }
};

void MessagePopupCollectionTest::CheckedAnimationDelegate::
    CheckToastsAreAligned(const std::string& calling_func) {
  if (error_msg_)
    return;
  auto poorly_aligned = std::adjacent_find(
      toasts_->begin(), toasts_->end(),
      [](ToastContentsView* top, ToastContentsView* bottom) {
        return ComputeYDistance(*top, *bottom) != kMarginBetweenPopups;
      });
  if (poorly_aligned != toasts_->end())
    error_msg_ = calling_func + " CheckToastsAreAligned: distance between: " +
                 (*poorly_aligned)->id() + ' ' +
                 (*std::next(poorly_aligned))->id() + ": " +
                 std::to_string(ComputeYDistance(**poorly_aligned,
                                                 **std::next(poorly_aligned))) +
                 " expected: " + std::to_string(kMarginBetweenPopups) +
                 "\nLayout:\n" + YPositionsToString(*toasts_);
}

void MessagePopupCollectionTest::CheckedAnimationDelegate::
    CheckToastsDontOverlap(const std::string& calling_func) {
  if (error_msg_)
    return;
  auto poorly_aligned = std::adjacent_find(
      toasts_->begin(), toasts_->end(),
      [](ToastContentsView* top, ToastContentsView* bottom) {
        return ComputeYDistance(*top, *bottom) < 0;
      });
  if (poorly_aligned != toasts_->end())
    error_msg_ = calling_func + " CheckToastsDontOverlap: distance between: " +
                 (*poorly_aligned)->id() + ' ' +
                 (*std::next(poorly_aligned))->id() + ": " +
                 std::to_string(ComputeYDistance(**poorly_aligned,
                                                 **std::next(poorly_aligned))) +
                 "\nLayout:\n" + YPositionsToString(*toasts_);
}

// static
std::string MessagePopupCollectionTest::YPositionsToString(
    const MessagePopupCollection::Toasts& toasts) {
  return std::accumulate(toasts.begin(), toasts.end(), std::string(),
                         [](std::string res, const ToastContentsView* toast) {
                           const auto& bounds =
                               toast->GetWidget()->GetWindowBoundsInScreen();
                           res += toast->id();
                           res += ' ';
                           res += std::to_string(bounds.y());
                           res += ", ";
                           res += std::to_string(bounds.y() + bounds.height());
                           res += '\n';
                           return res;
                         });
}

// static
int MessagePopupCollectionTest::CheckedAnimationDelegate::ComputeYDistance(
    const ToastContentsView& top,
    const ToastContentsView& bottom) {
  const auto* top_widget = top.GetWidget();
  const auto* bottom_widget = bottom.GetWidget();
  const auto& top_bounds = top_widget->GetWindowBoundsInScreen();
  const auto& bottom_bounds = bottom_widget->GetWindowBoundsInScreen();
  return bottom_bounds.y() - (top_bounds.y() + top_bounds.height());
}

#if defined(OS_CHROMEOS)
TEST_F(MessagePopupCollectionTest, DismissOnClick) {

  std::string id1 = AddNotification();
  std::string id2 = AddNotification();

  EXPECT_EQ(2u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  MessageCenter::Get()->ClickOnNotification(id2);

  EXPECT_EQ(1u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_FALSE(IsToastShown(id2));

  MessageCenter::Get()->ClickOnNotificationButton(id1, 0);
  EXPECT_EQ(0u, GetToastCounts());
  EXPECT_FALSE(IsToastShown(id1));
  EXPECT_FALSE(IsToastShown(id2));
}

#else

TEST_F(MessagePopupCollectionTest, NotDismissedOnClick) {
  std::string id1 = AddNotification();
  std::string id2 = AddNotification();

  EXPECT_EQ(2u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  MessageCenter::Get()->ClickOnNotification(id2);
  collection()->DoUpdate();

  EXPECT_EQ(2u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  MessageCenter::Get()->ClickOnNotificationButton(id1, 0);
  collection()->DoUpdate();
  EXPECT_EQ(2u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  GetWidget(id1)->CloseNow();
  GetWidget(id2)->CloseNow();
}

#endif  // OS_CHROMEOS

TEST_F(MessagePopupCollectionTest, ShutdownDuringShowing) {
  std::string id1 = AddNotification();
  std::string id2 = AddNotification();
  EXPECT_EQ(2u, GetToastCounts());
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  // Finish without cleanup of notifications, which may cause use-after-free.
  // See crbug.com/236448
  GetWidget(id1)->CloseNow();
  collection()->OnMouseExited(GetToast(id2));

  GetWidget(id2)->CloseNow();
}

TEST_F(MessagePopupCollectionTest, DefaultPositioning) {
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();
  std::string id2 = AddNotification();
  std::string id3 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);
  gfx::Rect r2 = GetToastRectAt(2);
  gfx::Rect r3 = GetToastRectAt(3);

  // 3 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));
  EXPECT_TRUE(IsToastShown(id2));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r1.width(), r2.width());

  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_EQ(r1.height(), r2.height());

  EXPECT_GT(r0.y(), r1.y());
  EXPECT_GT(r1.y(), r2.y());

  EXPECT_EQ(r0.x(), r1.x());
  EXPECT_EQ(r1.x(), r2.x());

  // The 4th toast is not shown yet.
  EXPECT_FALSE(IsToastShown(id3));
  EXPECT_EQ(0, r3.width());
  EXPECT_EQ(0, r3.height());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, DefaultPositioningWithRightTaskbar) {
  // If taskbar is on the right we show the toasts bottom to top as usual.

  // Simulate a taskbar at the right.
  SetDisplayInfo(gfx::Rect(0, 0, 590, 400),   // Work-area.
                 gfx::Rect(0, 0, 600, 400));  // Display-bounds.
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);

  // 2 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_GT(r0.y(), r1.y());
  EXPECT_EQ(r0.x(), r1.x());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, TopDownPositioningWithTopTaskbar) {
  // Simulate a taskbar at the top.
  SetDisplayInfo(gfx::Rect(0, 10, 600, 390),  // Work-area.
                 gfx::Rect(0, 0, 600, 400));  // Display-bounds.
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);

  // 2 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_LT(r0.y(), r1.y());
  EXPECT_EQ(r0.x(), r1.x());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, TopDownPositioningWithLeftAndTopTaskbar) {
  // If there "seems" to be a taskbar on left and top (like in Unity), it is
  // assumed that the actual taskbar is the top one.

  // Simulate a taskbar at the top and left.
  SetDisplayInfo(gfx::Rect(10, 10, 590, 390),  // Work-area.
                 gfx::Rect(0, 0, 600, 400));   // Display-bounds.
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);

  // 2 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_LT(r0.y(), r1.y());
  EXPECT_EQ(r0.x(), r1.x());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, TopDownPositioningWithBottomAndTopTaskbar) {
  // If there "seems" to be a taskbar on bottom and top (like in Gnome), it is
  // assumed that the actual taskbar is the top one.

  // Simulate a taskbar at the top and bottom.
  SetDisplayInfo(gfx::Rect(0, 10, 580, 400),  // Work-area.
                 gfx::Rect(0, 0, 600, 400));  // Display-bounds.
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);

  // 2 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_LT(r0.y(), r1.y());
  EXPECT_EQ(r0.x(), r1.x());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, LeftPositioningWithLeftTaskbar) {
  // Simulate a taskbar at the left.
  SetDisplayInfo(gfx::Rect(10, 0, 590, 400),  // Work-area.
                 gfx::Rect(0, 0, 600, 400));  // Display-bounds.
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  gfx::Rect r0 = GetToastRectAt(0);
  gfx::Rect r1 = GetToastRectAt(1);

  // 2 toasts are shown, equal size, vertical stack.
  EXPECT_TRUE(IsToastShown(id0));
  EXPECT_TRUE(IsToastShown(id1));

  EXPECT_EQ(r0.width(), r1.width());
  EXPECT_EQ(r0.height(), r1.height());
  EXPECT_GT(r0.y(), r1.y());
  EXPECT_EQ(r0.x(), r1.x());

  // Ensure that toasts are on the left.
  EXPECT_LT(r1.x(), GetWorkArea().CenterPoint().x());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

// Regression test for https://crbug.com/679397
TEST_F(MessagePopupCollectionTest, MultipleNotificationHeight) {
  std::string id0 = AddNotification();
  std::string id1 = AddImageNotification();
  EXPECT_EQ(2u, GetToastCounts());

  gfx::Rect r0 = GetToast(id0)->bounds();

  RemoveToastAndWaitForClose(id0);
  EXPECT_EQ(1u, GetToastCounts());

  gfx::Rect r1 = GetToast(id1)->bounds();

  // The heights should be different as one is an image notification while
  // another is a basic notification, but the bottom positions after the
  // animation should be same even though the heights are different.
  EXPECT_NE(r0.height(), r1.height());
  EXPECT_EQ(r0.bottom(), r1.bottom());

  CloseAllToasts();
  EXPECT_EQ(0u, GetToastCounts());
}

TEST_F(MessagePopupCollectionTest, DetectMouseHover) {
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  views::WidgetDelegateView* toast0 = GetToast(id0);
  EXPECT_TRUE(toast0 != NULL);
  views::WidgetDelegateView* toast1 = GetToast(id1);
  EXPECT_TRUE(toast1 != NULL);

  ui::MouseEvent event(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), 0, 0);

  // Test that mouse detection logic works in presence of out-of-order events.
  toast0->OnMouseEntered(event);
  EXPECT_TRUE(MouseInCollection());
  toast1->OnMouseEntered(event);
  EXPECT_TRUE(MouseInCollection());
  toast0->OnMouseExited(event);
  EXPECT_TRUE(MouseInCollection());
  toast1->OnMouseExited(event);
  EXPECT_FALSE(MouseInCollection());

  // Test that mouse detection logic works in presence of WindowClosing events.
  toast0->OnMouseEntered(event);
  EXPECT_TRUE(MouseInCollection());
  toast1->OnMouseEntered(event);
  EXPECT_TRUE(MouseInCollection());
  toast0->GetWidget()->CloseNow();
  EXPECT_TRUE(MouseInCollection());
  toast1->GetWidget()->CloseNow();
  EXPECT_FALSE(MouseInCollection());
}

// TODO(dimich): Test repositioning - both normal one and when user is closing
// the toasts.
TEST_F(MessagePopupCollectionTest, DetectMouseHoverWithUserClose) {
  std::string id0 = AddNotification();
  std::string id1 = AddNotification();

  views::WidgetDelegateView* toast0 = GetToast(id0);
  EXPECT_TRUE(toast0 != NULL);
  views::WidgetDelegateView* toast1 = GetToast(id1);
  ASSERT_TRUE(toast1 != NULL);

  ui::MouseEvent event(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), 0, 0);
  toast1->OnMouseEntered(event);
  RemoveToastAndWaitForClose(id1);

  EXPECT_FALSE(MouseInCollection());
  std::string id2 = AddNotification();

  views::WidgetDelegateView* toast2 = GetToast(id2);
  EXPECT_TRUE(toast2 != NULL);

  CloseAllToasts();
}

TEST_F(MessagePopupCollectionTest, ManyPopupNotifications) {
  // Add the max visible popup notifications +1, ensure the correct num visible.
  size_t notifications_to_add = 3 + 1;
  std::vector<std::string> ids(notifications_to_add);
  for (size_t i = 0; i < notifications_to_add; ++i) {
    ids[i] = AddNotification();
  }


  for (size_t i = 0; i < notifications_to_add - 1; ++i) {
    EXPECT_TRUE(IsToastShown(ids[i])) << "Should show the " << i << "th ID";
  }
  EXPECT_FALSE(IsToastShown(ids[notifications_to_add - 1]));

  CloseAllToasts();
}

#if defined(OS_CHROMEOS)

TEST_F(MessagePopupCollectionTest, CloseNonClosableNotifications) {
  const char* kNotificationId = "NOTIFICATION1";

  std::unique_ptr<Notification> notification(new Notification(
      NOTIFICATION_TYPE_BASE_FORMAT, kNotificationId,
      base::UTF8ToUTF16("test title"), base::UTF8ToUTF16("test message"),
      gfx::Image(), base::string16() /* display_source */, GURL(),
      NotifierId(NotifierId::APPLICATION, kNotificationId),
      RichNotificationData(), new NotificationDelegate()));
  notification->set_pinned(true);

  // Add a pinned notification.
  MessageCenter::Get()->AddNotification(std::move(notification));

  // Confirms that there is a toast.
  EXPECT_EQ(1u, GetToastCounts());
  EXPECT_EQ(1u, MessageCenter::Get()->NotificationCount());

  // Close the toast.
  views::WidgetDelegateView* toast1 = GetToast(kNotificationId);
  ASSERT_TRUE(toast1 != NULL);
  ui::MouseEvent event(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), 0, 0);
  toast1->OnMouseEntered(event);
  static_cast<MessageCenterObserver*>(collection())
      ->OnNotificationRemoved(kNotificationId, true);

  // Confirms that there is no toast.
  EXPECT_EQ(0u, GetToastCounts());
  // But the notification still exists.
  EXPECT_EQ(1u, MessageCenter::Get()->NotificationCount());
}

#endif  // defined(OS_CHROMEOS)

// When notifications were displayed on top, change of notification
// size didn't affect corresponding widget.
TEST_F(MessagePopupCollectionTest, ChangingNotificationSize) {
  // Simulate a taskbar at the top.
  SetDisplayInfo(gfx::Rect(0, 10, 600, 390),  // Work-area.
                 gfx::Rect(0, 0, 600, 400));  // Display-bounds.

  struct TestCase {
    std::string name;
    std::string text;
  };
  std::vector<TestCase> updates = {
      {"shrinking", ""},
      {"enlarging", "abc\ndef\nghi\n"},
      {"restoring", "abc\n"},
  };

  std::vector<std::string> notification_ids;
  // adding notifications
  {
    // adding popup notifications
    constexpr int max_visible_popup_notifications = 3;
    notification_ids.reserve(max_visible_popup_notifications);
    for (int i = 0; i < max_visible_popup_notifications; ++i) {
      notification_ids.push_back(std::to_string(i));
      auto notification =
          CreateTestNotification(notification_ids.back(), updates.back().text);
      MessageCenter::Get()->AddNotification(std::move(notification));
    }
  }


  // Confirms that there are 2 toasts of 3 notifications.
  EXPECT_EQ(3u, GetToastCounts());
  EXPECT_EQ(3u, MessageCenter::Get()->NotificationCount());

  // updating notifications one by one
  for (const std::string& id : notification_ids) {
    for (const auto& update : updates) {
      MessageCenter::Get()->UpdateNotification(
          id, CreateTestNotification(id, update.text));

      CheckedAnimationDelegate checked_animation(this);

      EXPECT_FALSE(checked_animation.error_msg())
          << "Animation error, test case: " << id << ' ' << update.name << ":\n"
          << *checked_animation.error_msg();
    }
  }

  CloseAllToasts();
}

}  // namespace test
}  // namespace message_center
