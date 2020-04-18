// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/window_modality_controller.h"

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/test_child_modal_parent.h"
#include "ash/wm/window_util.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/capture_client.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/test/capture_tracking_view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace ash {

using WindowModalityControllerTest = AshTestBase;

namespace {

bool ValidateStacking(aura::Window* parent, int ids[], int count) {
  for (int i = 0; i < count; ++i) {
    if (parent->children().at(i)->id() != ids[i])
      return false;
  }
  return true;
}

}  // namespace

// Creates three windows, w1, w11, and w12. w11 is a non-modal transient, w12 is
// a modal transient.
// Validates:
// - it should be possible to activate w12 even when w11 is open.
// - activating w1 activates w12 and updates stacking order appropriately.
// - closing a window passes focus up the stack.
TEST_F(WindowModalityControllerTest, BasicActivation) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w11(
      CreateTestWindowInShellWithDelegate(&d, -11, gfx::Rect()));
  std::unique_ptr<aura::Window> w12(
      CreateTestWindowInShellWithDelegate(&d, -12, gfx::Rect()));

  ::wm::AddTransientChild(w1.get(), w11.get());
  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
  wm::ActivateWindow(w11.get());
  EXPECT_TRUE(wm::IsActiveWindow(w11.get()));

  w12->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
  ::wm::AddTransientChild(w1.get(), w12.get());
  wm::ActivateWindow(w12.get());
  EXPECT_TRUE(wm::IsActiveWindow(w12.get()));

  wm::ActivateWindow(w11.get());
  EXPECT_TRUE(wm::IsActiveWindow(w11.get()));

  int check1[] = {-1, -12, -11};
  EXPECT_TRUE(ValidateStacking(w1->parent(), check1, arraysize(check1)));

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w12.get()));
  // Transient children are always stacked above their transient parent, which
  // is why this order is not -11, -1, -12.
  int check2[] = {-1, -11, -12};
  EXPECT_TRUE(ValidateStacking(w1->parent(), check2, arraysize(check2)));

  w12.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w11.get()));
  w11.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
}

// Create two toplevel windows w1 and w2, and nest two modals w11 and w111 below
// w1.
// Validates:
// - activating w1 while w11/w111 is showing always activates most deeply nested
//   descendant.
// - closing a window passes focus up the stack.
TEST_F(WindowModalityControllerTest, NestedModals) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w11(
      CreateTestWindowInShellWithDelegate(&d, -11, gfx::Rect()));
  std::unique_ptr<aura::Window> w111(
      CreateTestWindowInShellWithDelegate(&d, -111, gfx::Rect()));
  std::unique_ptr<aura::Window> w2(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));

  ::wm::AddTransientChild(w1.get(), w11.get());
  ::wm::AddTransientChild(w11.get(), w111.get());

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(wm::IsActiveWindow(w2.get()));

  // Set up modality.
  w11->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
  w111->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w111.get()));
  int check1[] = {-2, -1, -11, -111};
  EXPECT_TRUE(ValidateStacking(w1->parent(), check1, arraysize(check1)));

  wm::ActivateWindow(w11.get());
  EXPECT_TRUE(wm::IsActiveWindow(w111.get()));
  EXPECT_TRUE(ValidateStacking(w1->parent(), check1, arraysize(check1)));

  wm::ActivateWindow(w111.get());
  EXPECT_TRUE(wm::IsActiveWindow(w111.get()));
  EXPECT_TRUE(ValidateStacking(w1->parent(), check1, arraysize(check1)));

  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(wm::IsActiveWindow(w2.get()));
  int check2[] = {-1, -11, -111, -2};
  EXPECT_TRUE(ValidateStacking(w1->parent(), check2, arraysize(check2)));

  w2.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w111.get()));
  w111.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w11.get()));
  w11.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
}

// Create two toplevel windows w1 and w2, and nest two modals w11 and w111 below
// w1.
// Validates:
// - destroying w11 while w111 is focused activates w1.
TEST_F(WindowModalityControllerTest, NestedModalsOuterClosed) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w11(
      CreateTestWindowInShellWithDelegate(&d, -11, gfx::Rect()));
  // |w111| will be owned and deleted by |w11|.
  aura::Window* w111 =
      CreateTestWindowInShellWithDelegate(&d, -111, gfx::Rect());
  std::unique_ptr<aura::Window> w2(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));

  ::wm::AddTransientChild(w1.get(), w11.get());
  ::wm::AddTransientChild(w11.get(), w111);

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(wm::IsActiveWindow(w2.get()));

  // Set up modality.
  w11->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
  w111->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w111));

  w111->Hide();
  EXPECT_TRUE(wm::IsActiveWindow(w11.get()));

  // TODO(oshima): Re-showing doesn't set the focus back to
  // modal window. There is no such use case right now, but it
  // probably should.

  w11.reset();
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
}

// Modality also prevents events from being passed to the transient parent.
TEST_F(WindowModalityControllerTest, Events) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect(0, 0, 100, 100)));
  std::unique_ptr<aura::Window> w11(
      CreateTestWindowInShellWithDelegate(&d, -11, gfx::Rect(20, 20, 50, 50)));
  std::unique_ptr<aura::Window> w111(
      CreateTestWindowInShellWithDelegate(&d, -111, gfx::Rect(20, 20, 50, 50)));

  ::wm::AddTransientChild(w1.get(), w11.get());

  // Add a non-modal child to the modal window in order to ensure modality still
  // works in this case. This is a regression test for https://crbug.com/456697.
  ::wm::AddTransientChild(w11.get(), w111.get());

  {
    // Clicking a point within w1 should activate that window.
    ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow(),
                                       gfx::Point(10, 10));
    generator.ClickLeftButton();
    EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
  }

  w11->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);

  {
    // Clicking a point within w1 should activate w11.
    ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow(),
                                       gfx::Point(10, 10));
    generator.ClickLeftButton();
    EXPECT_TRUE(wm::IsActiveWindow(w11.get()));
  }
}

// Events on modal parent activate.
TEST_F(WindowModalityControllerTest, EventsForEclipsedWindows) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect(0, 0, 100, 100)));
  std::unique_ptr<aura::Window> w11(
      CreateTestWindowInShellWithDelegate(&d, -11, gfx::Rect(20, 20, 50, 50)));
  ::wm::AddTransientChild(w1.get(), w11.get());
  std::unique_ptr<aura::Window> w2(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect(0, 0, 50, 50)));

  w11->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);

  // Partially eclipse w1 with w2.
  wm::ActivateWindow(w2.get());
  {
    // Clicking a point on w1 that is not eclipsed by w2.
    ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow(),
                                       gfx::Point(90, 90));
    generator.ClickLeftButton();
    EXPECT_TRUE(wm::IsActiveWindow(w11.get()));
  }
}

// Creates windows w1 and non activatiable child w11. Creates transient window
// w2 and adds it as a transeint child of w1. Ensures that w2 is parented to
// the parent of w1, and that GetModalTransient(w11) returns w2.
TEST_F(WindowModalityControllerTest, GetModalTransient) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w11(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w1.get()));
  std::unique_ptr<aura::Window> w2(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));
  w2->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);

  aura::Window* wt;
  wt = ::wm::GetModalTransient(w1.get());
  ASSERT_EQ(static_cast<aura::Window*>(NULL), wt);

  // Parent w2 to w1. It should get parented to the parent of w1.
  ::wm::AddTransientChild(w1.get(), w2.get());
  ASSERT_EQ(2U, w1->parent()->children().size());
  EXPECT_EQ(-2, w1->parent()->children().at(1)->id());

  // Request the modal transient window for w1, it should be w2.
  wt = ::wm::GetModalTransient(w1.get());
  ASSERT_NE(static_cast<aura::Window*>(NULL), wt);
  EXPECT_EQ(-2, wt->id());

  // Request the modal transient window for w11, it should also be w2.
  wt = ::wm::GetModalTransient(w11.get());
  ASSERT_NE(static_cast<aura::Window*>(NULL), wt);
  EXPECT_EQ(-2, wt->id());
}

// Verifies we generate a capture lost when showing a modal window.
TEST_F(WindowModalityControllerTest, ChangeCapture) {
  views::Widget* widget = views::Widget::CreateWindowWithContext(
      NULL, Shell::GetPrimaryRootWindow());
  std::unique_ptr<aura::Window> widget_window(widget->GetNativeView());
  views::test::CaptureTrackingView* view = new views::test::CaptureTrackingView;
  widget->client_view()->AddChildView(view);
  widget->SetBounds(gfx::Rect(0, 0, 200, 200));
  view->SetBoundsRect(widget->client_view()->GetLocalBounds());
  widget->Show();

  gfx::Point center(view->width() / 2, view->height() / 2);
  views::View::ConvertPointToScreen(view, &center);
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow(), center);
  generator.PressLeftButton();
  EXPECT_TRUE(view->got_press());

  views::Widget* modal_widget =
      views::Widget::CreateWindowWithParent(NULL, widget->GetNativeView());
  std::unique_ptr<aura::Window> modal_window(modal_widget->GetNativeView());
  modal_window->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
  views::test::CaptureTrackingView* modal_view =
      new views::test::CaptureTrackingView;
  modal_widget->client_view()->AddChildView(modal_view);
  modal_widget->SetBounds(gfx::Rect(50, 50, 200, 200));
  modal_view->SetBoundsRect(modal_widget->client_view()->GetLocalBounds());
  modal_widget->Show();

  EXPECT_TRUE(view->got_capture_lost());
  generator.ReleaseLeftButton();

  view->reset();

  EXPECT_FALSE(modal_view->got_capture_lost());
  EXPECT_FALSE(modal_view->got_press());

  gfx::Point modal_center(modal_view->width() / 2, modal_view->height() / 2);
  views::View::ConvertPointToScreen(modal_view, &modal_center);
  generator.MoveMouseTo(modal_center, 1);
  generator.PressLeftButton();
  EXPECT_TRUE(modal_view->got_press());
  EXPECT_FALSE(modal_view->got_capture_lost());
  EXPECT_FALSE(view->got_capture_lost());
  EXPECT_FALSE(view->got_press());
}

// Test that when a child modal window becomes visible, we only release the
// capture window if the current capture window is in the hierarchy of the child
// modal window's modal parent window.
TEST_F(WindowModalityControllerTest, ReleaseCapture) {
  // Create a window hierarchy like this:
  //            _______________w0______________
  //            |               |              |
  //           w1     <------   w3             w2
  //            |    (modal to)
  //           w11

  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w11(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w1.get()));
  std::unique_ptr<aura::Window> w2(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));
  std::unique_ptr<aura::Window> w3(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));
  w3->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_CHILD);
  ::wm::SetModalParent(w3.get(), w1.get());

  // w1's capture should be released when w3 becomes visible.
  w3->Hide();
  aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
      ->SetCapture(w1.get());
  EXPECT_EQ(w1.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());
  w3->Show();
  EXPECT_NE(w1.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());

  // w11's capture should be released when w3 becomes visible.
  w3->Hide();
  aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
      ->SetCapture(w11.get());
  EXPECT_EQ(w11.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());
  w3->Show();
  EXPECT_NE(w11.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());

  // w2's capture should not be released when w3 becomes visible.
  w3->Hide();
  aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
      ->SetCapture(w2.get());
  EXPECT_EQ(w2.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());
  w3->Show();
  EXPECT_EQ(w2.get(),
            aura::client::GetCaptureClient(Shell::GetPrimaryRootWindow())
                ->GetGlobalCaptureWindow());
}

class TouchTrackerWindowDelegate : public aura::test::TestWindowDelegate {
 public:
  TouchTrackerWindowDelegate()
      : received_touch_(false), last_event_type_(ui::ET_UNKNOWN) {}
  ~TouchTrackerWindowDelegate() override = default;

  void reset() {
    received_touch_ = false;
    last_event_type_ = ui::ET_UNKNOWN;
  }

  bool received_touch() const { return received_touch_; }
  ui::EventType last_event_type() const { return last_event_type_; }

 private:
  // Overridden from aura::test::TestWindowDelegate.
  void OnTouchEvent(ui::TouchEvent* event) override {
    received_touch_ = true;
    last_event_type_ = event->type();
    aura::test::TestWindowDelegate::OnTouchEvent(event);
  }

  bool received_touch_;
  ui::EventType last_event_type_;

  DISALLOW_COPY_AND_ASSIGN(TouchTrackerWindowDelegate);
};

// Modality should prevent events from being passed to the transient parent.
TEST_F(WindowModalityControllerTest, TouchEvent) {
  TouchTrackerWindowDelegate d1;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d1, -1, gfx::Rect(0, 0, 100, 100)));
  TouchTrackerWindowDelegate d11;
  std::unique_ptr<aura::Window> w11(CreateTestWindowInShellWithDelegate(
      &d11, -11, gfx::Rect(20, 20, 50, 50)));
  // Make |w11| non-resizable to avoid touch events inside its transient parent
  // |w1| from going to |w11| because of EasyResizeWindowTargeter.
  w11->SetProperty(aura::client::kResizeBehaviorKey,
                   ui::mojom::kResizeBehaviorCanMaximize |
                       ui::mojom::kResizeBehaviorCanMinimize);
  ui::test::EventGenerator generator(Shell::GetPrimaryRootWindow(),
                                     gfx::Point(10, 10));

  ::wm::AddTransientChild(w1.get(), w11.get());
  d1.reset();
  d11.reset();

  {
    // Clicking a point within w1 should activate that window.
    generator.PressMoveAndReleaseTouchTo(gfx::Point(10, 10));
    EXPECT_TRUE(wm::IsActiveWindow(w1.get()));
    EXPECT_TRUE(d1.received_touch());
    EXPECT_FALSE(d11.received_touch());
  }

  {
    // Adding a modal window while a touch is down should fire a touch cancel.
    generator.PressTouch();
    generator.MoveTouch(gfx::Point(10, 10));
    d1.reset();
    d11.reset();

    w11->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
    EXPECT_TRUE(d1.received_touch());
    EXPECT_EQ(ui::ET_TOUCH_CANCELLED, d1.last_event_type());
    EXPECT_FALSE(d11.received_touch());
  }
}

// Child-modal test.
// Creates:
// - A |parent| window that hosts a |modal_parent| window within itself. The
//   |parent| and |modal_parent| windows are not the same window.  The
//   |modal_parent| window is not activatable, because it's contained within the
//   |parent| window.
// - A |child| window with parent window |parent|, but is modal to
//   |modal_parent| window.
// Validates:
// - Clicking on the |modal_parent| should activate the |child| window.
// - Clicking on the |parent| window outside of the |modal_parent| bounds should
//   activate the |parent| window.
// - Clicking on the |child| while |parent| is active should activate the
//   |child| window.
// - Focus should follow the active window.
TEST_F(WindowModalityControllerTest, ChildModal) {
  TestChildModalParent* delegate = new TestChildModalParent();
  views::Widget* widget = views::Widget::CreateWindowWithContextAndBounds(
      delegate, nullptr /* context */, gfx::Rect(0, 0, 400, 400));
  widget->Show();

  aura::Window* parent = widget->GetNativeView();
  EXPECT_TRUE(wm::IsActiveWindow(parent));

  aura::Window* modal_parent = delegate->GetModalParent();
  EXPECT_NE(static_cast<aura::Window*>(NULL), modal_parent);
  EXPECT_NE(parent, modal_parent);
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));

  delegate->ShowChild();
  aura::Window* child = delegate->GetChild();
  EXPECT_NE(nullptr, child);

  EXPECT_TRUE(wm::IsActiveWindow(child));
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
  EXPECT_FALSE(wm::IsActiveWindow(parent));

  EXPECT_TRUE(child->HasFocus());
  EXPECT_FALSE(modal_parent->HasFocus());
  EXPECT_FALSE(parent->HasFocus());

  wm::ActivateWindow(modal_parent);

  EXPECT_TRUE(wm::IsActiveWindow(child));
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
  EXPECT_FALSE(wm::IsActiveWindow(parent));

  EXPECT_TRUE(child->HasFocus());
  EXPECT_FALSE(modal_parent->HasFocus());
  EXPECT_FALSE(parent->HasFocus());

  wm::ActivateWindow(parent);

  EXPECT_FALSE(wm::IsActiveWindow(child));
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
  EXPECT_TRUE(wm::IsActiveWindow(parent));

  EXPECT_FALSE(child->HasFocus());
  EXPECT_FALSE(modal_parent->HasFocus());
  EXPECT_TRUE(parent->HasFocus());

  wm::ActivateWindow(child);

  EXPECT_TRUE(wm::IsActiveWindow(child));
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
  EXPECT_FALSE(wm::IsActiveWindow(parent));

  EXPECT_TRUE(child->HasFocus());
  EXPECT_FALSE(modal_parent->HasFocus());
  EXPECT_FALSE(parent->HasFocus());
}

// Same as |ChildModal| test, but using |EventGenerator| rather than bypassing
// it by calling |ActivateWindow|.
TEST_F(WindowModalityControllerTest, ChildModalEventGenerator) {
  TestChildModalParent* delegate = new TestChildModalParent();
  views::Widget* widget = views::Widget::CreateWindowWithContextAndBounds(
      delegate, nullptr /* context */, gfx::Rect(0, 0, 400, 400));
  widget->Show();

  aura::Window* parent = widget->GetNativeView();
  EXPECT_TRUE(wm::IsActiveWindow(parent));

  aura::Window* modal_parent = delegate->GetModalParent();
  EXPECT_NE(static_cast<aura::Window*>(NULL), modal_parent);
  EXPECT_NE(parent, modal_parent);
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));

  delegate->ShowChild();
  aura::Window* child = delegate->GetChild();
  EXPECT_NE(static_cast<aura::Window*>(NULL), child);

  EXPECT_TRUE(wm::IsActiveWindow(child));
  EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
  EXPECT_FALSE(wm::IsActiveWindow(parent));

  EXPECT_TRUE(child->HasFocus());
  EXPECT_FALSE(modal_parent->HasFocus());
  EXPECT_FALSE(parent->HasFocus());

  {
    ui::test::EventGenerator generator(
        Shell::GetPrimaryRootWindow(),
        parent->bounds().origin() +
            gfx::Vector2d(10, parent->bounds().height() - 10));
    generator.ClickLeftButton();
    generator.ClickLeftButton();

    EXPECT_TRUE(wm::IsActiveWindow(child));
    EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
    EXPECT_FALSE(wm::IsActiveWindow(parent));

    EXPECT_TRUE(child->HasFocus());
    EXPECT_FALSE(modal_parent->HasFocus());
    EXPECT_FALSE(parent->HasFocus());
  }

  {
    ui::test::EventGenerator generator(
        Shell::GetPrimaryRootWindow(),
        parent->bounds().origin() + gfx::Vector2d(10, 10));
    generator.ClickLeftButton();

    EXPECT_FALSE(wm::IsActiveWindow(child));
    EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
    EXPECT_TRUE(wm::IsActiveWindow(parent));

    EXPECT_FALSE(child->HasFocus());
    EXPECT_FALSE(modal_parent->HasFocus());
    EXPECT_TRUE(parent->HasFocus());
  }

  {
    ui::test::EventGenerator generator(
        Shell::GetPrimaryRootWindow(),
        child->bounds().origin() + gfx::Vector2d(10, 10));
    generator.ClickLeftButton();

    EXPECT_TRUE(wm::IsActiveWindow(child));
    EXPECT_FALSE(wm::IsActiveWindow(modal_parent));
    EXPECT_FALSE(wm::IsActiveWindow(parent));

    EXPECT_TRUE(child->HasFocus());
    EXPECT_FALSE(modal_parent->HasFocus());
    EXPECT_FALSE(parent->HasFocus());
  }
}

// Window-modal test for the case when the originally clicked window is an
// ancestor of the modal parent.
TEST_F(WindowModalityControllerTest, WindowModalAncestor) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w2(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w1.get()));
  std::unique_ptr<aura::Window> w3(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w2.get()));
  std::unique_ptr<aura::Window> w4(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));
  w4->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_WINDOW);
  ::wm::AddTransientChild(w1.get(), w4.get());

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));

  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));

  wm::ActivateWindow(w3.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));

  wm::ActivateWindow(w4.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));
}

// Child-modal test for the case when the originally clicked window is an
// ancestor of the modal parent.
TEST_F(WindowModalityControllerTest, ChildModalAncestor) {
  aura::test::TestWindowDelegate d;
  std::unique_ptr<aura::Window> w1(
      CreateTestWindowInShellWithDelegate(&d, -1, gfx::Rect()));
  std::unique_ptr<aura::Window> w2(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w1.get()));
  std::unique_ptr<aura::Window> w3(
      aura::test::CreateTestWindowWithDelegate(&d, -11, gfx::Rect(), w2.get()));
  std::unique_ptr<aura::Window> w4(
      CreateTestWindowInShellWithDelegate(&d, -2, gfx::Rect()));
  w4->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_CHILD);
  ::wm::SetModalParent(w4.get(), w2.get());
  ::wm::AddTransientChild(w1.get(), w4.get());

  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));

  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));

  wm::ActivateWindow(w3.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));

  wm::ActivateWindow(w4.get());
  EXPECT_TRUE(wm::IsActiveWindow(w4.get()));
}

}  // namespace ash
