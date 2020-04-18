// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/widget_test.h"

#include "build/build_config.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/test/native_widget_factory.h"
#include "ui/views/widget/root_view.h"

namespace views {
namespace test {
namespace {

Widget* CreateTopLevelPlatformWidgetWithStubbedCapture(
    ViewsTestBase* test,
    Widget::InitParams::Type type) {
  Widget* widget = new Widget;
  Widget::InitParams params = test->CreateParams(type);
  params.native_widget =
      CreatePlatformNativeWidgetImpl(params, widget, kStubCapture, nullptr);
  widget->Init(params);
  return widget;
}

}  // namespace

void WidgetTest::WidgetCloser::operator()(Widget* widget) const {
  widget->CloseNow();
}

WidgetTest::WidgetTest() {}
WidgetTest::~WidgetTest() {}

Widget* WidgetTest::CreateTopLevelPlatformWidget() {
  return CreateTopLevelPlatformWidgetWithStubbedCapture(
      this, Widget::InitParams::TYPE_WINDOW);
}

Widget* WidgetTest::CreateTopLevelFramelessPlatformWidget() {
  return CreateTopLevelPlatformWidgetWithStubbedCapture(
      this, Widget::InitParams::TYPE_WINDOW_FRAMELESS);
}

Widget* WidgetTest::CreateChildPlatformWidget(
    gfx::NativeView parent_native_view) {
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_CONTROL);
  params.parent = parent_native_view;
  Widget* child = new Widget;
  params.native_widget =
      CreatePlatformNativeWidgetImpl(params, child, kStubCapture, nullptr);
  child->Init(params);
  child->SetContentsView(new View);
  return child;
}

Widget* WidgetTest::CreateTopLevelNativeWidget() {
  Widget* toplevel = new Widget;
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
  toplevel->Init(params);
  return toplevel;
}

Widget* WidgetTest::CreateChildNativeWidgetWithParent(Widget* parent) {
  Widget* child = new Widget;
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_CONTROL);
  params.parent = parent->GetNativeView();
  child->Init(params);
  child->SetContentsView(new View);
  return child;
}

Widget* WidgetTest::CreateChildNativeWidget() {
  return CreateChildNativeWidgetWithParent(nullptr);
}

Widget* WidgetTest::CreateNativeDesktopWidget() {
  Widget* widget = new Widget;
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
  params.native_widget =
      CreatePlatformDesktopNativeWidgetImpl(params, widget, nullptr);
  widget->Init(params);
  return widget;
}

View* WidgetTest::GetMousePressedHandler(internal::RootView* root_view) {
  return root_view->mouse_pressed_handler_;
}

View* WidgetTest::GetMouseMoveHandler(internal::RootView* root_view) {
  return root_view->mouse_move_handler_;
}

View* WidgetTest::GetGestureHandler(internal::RootView* root_view) {
  return root_view->gesture_handler_;
}

TestDesktopWidgetDelegate::TestDesktopWidgetDelegate() : widget_(new Widget) {
}

TestDesktopWidgetDelegate::~TestDesktopWidgetDelegate() {
  if (widget_)
    widget_->CloseNow();
  EXPECT_FALSE(widget_);
}

void TestDesktopWidgetDelegate::InitWidget(Widget::InitParams init_params) {
  init_params.delegate = this;
#if !defined(OS_CHROMEOS)
  init_params.native_widget =
      CreatePlatformDesktopNativeWidgetImpl(init_params, widget_, nullptr);
#endif
  init_params.bounds = initial_bounds_;
  widget_->Init(init_params);
}

void TestDesktopWidgetDelegate::WindowClosing() {
  window_closing_count_++;
  widget_ = nullptr;
}

Widget* TestDesktopWidgetDelegate::GetWidget() {
  return widget_;
}

const Widget* TestDesktopWidgetDelegate::GetWidget() const {
  return widget_;
}

View* TestDesktopWidgetDelegate::GetContentsView() {
  return contents_view_ ? contents_view_ : WidgetDelegate::GetContentsView();
}

bool TestDesktopWidgetDelegate::ShouldAdvanceFocusToTopLevelWidget() const {
  return true;  // Same default as DefaultWidgetDelegate in widget.cc.
}

TestInitialFocusWidgetDelegate::TestInitialFocusWidgetDelegate(
    gfx::NativeWindow context)
    : view_(new View) {
  view_->SetFocusBehavior(View::FocusBehavior::ALWAYS);

  Widget::InitParams params(Widget::InitParams::TYPE_WINDOW);
  params.context = context;
  params.delegate = this;
  GetWidget()->Init(params);
  GetWidget()->GetContentsView()->AddChildView(view_);
}

TestInitialFocusWidgetDelegate::~TestInitialFocusWidgetDelegate() {}

View* TestInitialFocusWidgetDelegate::GetInitiallyFocusedView() {
  return view_;
}

WidgetActivationWaiter::WidgetActivationWaiter(Widget* widget, bool active)
    : observed_(false), active_(active) {
  if (active == widget->IsActive()) {
    observed_ = true;
    return;
  }
  widget->AddObserver(this);
}

WidgetActivationWaiter::~WidgetActivationWaiter() {}

void WidgetActivationWaiter::Wait() {
  if (!observed_)
    run_loop_.Run();
}

void WidgetActivationWaiter::OnWidgetActivationChanged(Widget* widget,
                                                       bool active) {
  if (active_ != active)
    return;

  observed_ = true;
  widget->RemoveObserver(this);
  if (run_loop_.running())
    run_loop_.Quit();
}

WidgetClosingObserver::WidgetClosingObserver(Widget* widget) : widget_(widget) {
  widget_->AddObserver(this);
}

WidgetClosingObserver::~WidgetClosingObserver() {
  if (widget_)
    widget_->RemoveObserver(this);
}

void WidgetClosingObserver::Wait() {
  if (widget_)
    run_loop_.Run();
}

void WidgetClosingObserver::OnWidgetClosing(Widget* widget) {
  DCHECK_EQ(widget_, widget);
  widget_->RemoveObserver(this);
  widget_ = nullptr;
  if (run_loop_.running())
    run_loop_.Quit();
}

}  // namespace test
}  // namespace views
