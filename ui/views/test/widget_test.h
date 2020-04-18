// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TEST_WIDGET_TEST_H_
#define UI_VIEWS_TEST_WIDGET_TEST_H_

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace ui {
namespace internal {
class InputMethodDelegate;
}
class EventSink;
}

namespace views {

class Widget;

namespace internal {

class RootView;

}  // namespace internal

namespace test {

class WidgetTest : public ViewsTestBase {
 public:
  // This class can be used as a deleter for std::unique_ptr<Widget>
  // to call function Widget::CloseNow automatically.
  struct WidgetCloser {
    void operator()(Widget* widget) const;
  };

  using WidgetAutoclosePtr = std::unique_ptr<Widget, WidgetCloser>;

  WidgetTest();
  ~WidgetTest() override;

  // Create Widgets with |native_widget| in InitParams set to an instance of
  // platform specific widget type that has stubbled capture calls.
  Widget* CreateTopLevelPlatformWidget();
  Widget* CreateTopLevelFramelessPlatformWidget();
  Widget* CreateChildPlatformWidget(gfx::NativeView parent_native_view);

  // Create Widgets initialized without a |native_widget| set in InitParams.
  // Depending on the test environment, ViewsDelegate::OnBeforeWidgetInit() may
  // still provide one.
  Widget* CreateTopLevelNativeWidget();
  Widget* CreateChildNativeWidgetWithParent(Widget* parent);
  Widget* CreateChildNativeWidget();

  // Create a top-level Widget with |native_widget| in InitParams set to an
  // instance of the "native desktop" type. This is a PlatformNativeWidget on
  // ChromeOS, and a PlatformDesktopNativeWidget everywhere else.
  Widget* CreateNativeDesktopWidget();

  View* GetMousePressedHandler(internal::RootView* root_view);

  View* GetMouseMoveHandler(internal::RootView* root_view);

  View* GetGestureHandler(internal::RootView* root_view);

  // Simulate an activation of the native window held by |widget|, as if it was
  // clicked by the user. This is a synchronous method for use in
  // non-interactive tests that do not spin a RunLoop in the test body (since
  // that may cause real focus changes to flakily manifest).
  static void SimulateNativeActivate(Widget* widget);

  // Return true if |window| is visible according to the native platform.
  static bool IsNativeWindowVisible(gfx::NativeWindow window);

  // Return true if |above| is higher than |below| in the native window Z-order.
  // Both windows must be visible.
  static bool IsWindowStackedAbove(Widget* above, Widget* below);

  // Query the native window system for the minimum size configured for user
  // initiated window resizes.
  gfx::Size GetNativeWidgetMinimumContentSize(Widget* widget);

  // Return the event sink for |widget|. On aura platforms, this is an
  // aura::WindowEventDispatcher. Otherwise, it is a bridge to the OS event
  // sink.
  static ui::EventSink* GetEventSink(Widget* widget);

  // Get the InputMethodDelegate, for setting on a Mock InputMethod in tests.
  static ui::internal::InputMethodDelegate* GetInputMethodDelegateForWidget(
      Widget* widget);

  // Return true if |window| is transparent according to the native platform.
  static bool IsNativeWindowTransparent(gfx::NativeWindow window);

  // Returns whether |widget| has a Window shadow managed in this process. That
  // is, a shadow that is drawn outside of the Widget bounds, and managed by the
  // WindowManager.
  static bool WidgetHasInProcessShadow(Widget* widget);

  // Returns the set of all Widgets that currently have a NativeWindow.
  static Widget::Widgets GetAllWidgets();

 private:
  DISALLOW_COPY_AND_ASSIGN(WidgetTest);
};

// A helper WidgetDelegate for tests that require hooks into WidgetDelegate
// calls, and removes some of the boilerplate for initializing a Widget. Calls
// Widget::CloseNow() when destroyed if it hasn't already been done.
class TestDesktopWidgetDelegate : public WidgetDelegate {
 public:
  TestDesktopWidgetDelegate();
  ~TestDesktopWidgetDelegate() override;

  // Initialize the Widget, adding some meaningful default InitParams.
  void InitWidget(Widget::InitParams init_params);

  // Set the contents view to be used during Widget initialization. For Widgets
  // that use non-client views, this will be the contents_view used to
  // initialize the ClientView in WidgetDelegate::CreateClientView(). Otherwise,
  // it is the ContentsView of the Widget's RootView. Ownership passes to the
  // view hierarchy during InitWidget().
  void set_contents_view(View* contents_view) {
    contents_view_ = contents_view;
  }

  int window_closing_count() const { return window_closing_count_; }
  const gfx::Rect& initial_bounds() { return initial_bounds_; }

  // WidgetDelegate overrides:
  void WindowClosing() override;
  Widget* GetWidget() override;
  const Widget* GetWidget() const override;
  View* GetContentsView() override;
  bool ShouldAdvanceFocusToTopLevelWidget() const override;

 private:
  Widget* widget_;
  View* contents_view_ = nullptr;
  int window_closing_count_ = 0;
  gfx::Rect initial_bounds_ = gfx::Rect(100, 100, 200, 200);

  DISALLOW_COPY_AND_ASSIGN(TestDesktopWidgetDelegate);
};

// Testing widget delegate that creates a widget with a single view, which is
// the initially focused view.
class TestInitialFocusWidgetDelegate : public TestDesktopWidgetDelegate {
 public:
  explicit TestInitialFocusWidgetDelegate(gfx::NativeWindow context);
  ~TestInitialFocusWidgetDelegate() override;

  View* view() { return view_; }

  // WidgetDelegate override:
  View* GetInitiallyFocusedView() override;

 private:
  View* view_;

  DISALLOW_COPY_AND_ASSIGN(TestInitialFocusWidgetDelegate);
};

// Use in tests to wait until a Widget's activation change to a particular
// value. To use create and call Wait().
class WidgetActivationWaiter : public WidgetObserver {
 public:
  WidgetActivationWaiter(Widget* widget, bool active);
  ~WidgetActivationWaiter() override;

  // Returns when the active status matches that supplied to the constructor. If
  // the active status does not match that of the constructor a RunLoop is used
  // until the active status matches, otherwise this returns immediately.
  void Wait();

 private:
  // views::WidgetObserver override:
  void OnWidgetActivationChanged(Widget* widget, bool active) override;

  base::RunLoop run_loop_;
  bool observed_;
  bool active_;

  DISALLOW_COPY_AND_ASSIGN(WidgetActivationWaiter);
};

// Use in tests to provide functionality to observe the widget passed in the
// constructor for the widget closing event.
class WidgetClosingObserver : public WidgetObserver {
 public:
  explicit WidgetClosingObserver(Widget* widget);
  ~WidgetClosingObserver() override;

  // Returns immediately when |widget_| becomes NULL, otherwise a RunLoop is
  // used until widget closing event is received.
  void Wait();

  bool widget_closed() const { return !widget_; }

 private:
  // views::WidgetObserver override:
  void OnWidgetClosing(Widget* widget) override;

  Widget* widget_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WidgetClosingObserver);
};

}  // namespace test
}  // namespace views

#endif  // UI_VIEWS_TEST_WIDGET_TEST_H_
