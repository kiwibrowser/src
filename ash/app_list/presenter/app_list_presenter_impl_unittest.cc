// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/app_list_presenter_impl.h"

#include <memory>

#include "ash/app_list/presenter/app_list_presenter_delegate_factory.h"
#include "ash/app_list/presenter/test/app_list_presenter_impl_test_api.h"
#include "base/memory/ptr_util.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/wm/core/default_activation_client.h"
#include "ui/wm/core/window_util.h"

namespace app_list {

namespace {

// Test stub for AppListPresenterDelegate
class AppListPresenterDelegateTest : public AppListPresenterDelegate {
 public:
  AppListPresenterDelegateTest(aura::Window* container,
                               test::AppListTestViewDelegate* view_delegate)
      : container_(container), view_delegate_(view_delegate) {}
  ~AppListPresenterDelegateTest() override {}

  bool init_called() const { return init_called_; }
  bool on_shown_called() const { return on_shown_called_; }
  bool on_dismissed_called() const { return on_dismissed_called_; }

 private:
  // AppListPresenterDelegate:
  AppListViewDelegate* GetViewDelegate() override { return view_delegate_; }
  void Init(AppListView* view,
            int64_t display_id,
            int current_apps_page) override {
    init_called_ = true;
    view_ = view;
    AppListView::InitParams params;
    params.parent = container_;
    params.initial_apps_page = current_apps_page;
    view->Initialize(params);
  }
  void OnShown(int64_t display_id) override { on_shown_called_ = true; }
  void OnDismissed() override { on_dismissed_called_ = true; }
  gfx::Vector2d GetVisibilityAnimationOffset(aura::Window*) override {
    return gfx::Vector2d(0, 0);
  }
  base::TimeDelta GetVisibilityAnimationDuration(aura::Window* root_window,
                                                 bool is_visible) override {
    return base::TimeDelta::FromMilliseconds(0);
  }

 private:
  aura::Window* container_;
  test::AppListTestViewDelegate* view_delegate_;
  AppListView* view_ = nullptr;
  bool init_called_ = false;
  bool on_shown_called_ = false;
  bool on_dismissed_called_ = false;
  views::TestViewsDelegate views_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterDelegateTest);
};

// Test fake for AppListPresenterDelegateFactory, creates instances of
// AppListPresenterDelegateTest.
class AppListPresenterDelegateFactoryTest
    : public AppListPresenterDelegateFactory {
 public:
  explicit AppListPresenterDelegateFactoryTest(aura::Window* container)
      : container_(container) {}
  ~AppListPresenterDelegateFactoryTest() override {}

  // AppListPresenterDelegateFactory:
  std::unique_ptr<AppListPresenterDelegate> GetDelegate(
      AppListPresenterImpl* presenter) override {
    return std::make_unique<AppListPresenterDelegateTest>(
        container_, &app_list_view_delegate_);
  }

 private:
  aura::Window* container_;
  test::AppListTestViewDelegate app_list_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterDelegateFactoryTest);
};

}  // namespace

class AppListPresenterImplTest : public aura::test::AuraTestBase {
 public:
  AppListPresenterImplTest();
  ~AppListPresenterImplTest() override;

  AppListPresenterImpl* presenter() { return presenter_.get(); }
  aura::Window* container() { return container_.get(); }
  int64_t GetDisplayId() { return test_screen()->GetPrimaryDisplay().id(); }

  // Don't cache the return of this method - a new delegate is created every
  // time the app list is shown.
  AppListPresenterDelegateTest* delegate() {
    return static_cast<AppListPresenterDelegateTest*>(
        presenter_test_api_->presenter_delegate());
  }

  // aura::test::AuraTestBase:
  void SetUp() override;
  void TearDown() override;

 private:
  std::unique_ptr<AppListPresenterImpl> presenter_;
  std::unique_ptr<aura::Window> container_;
  std::unique_ptr<test::AppListPresenterImplTestApi> presenter_test_api_;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterImplTest);
};

AppListPresenterImplTest::AppListPresenterImplTest() {}

AppListPresenterImplTest::~AppListPresenterImplTest() {}

void AppListPresenterImplTest::SetUp() {
  AuraTestBase::SetUp();
  new wm::DefaultActivationClient(root_window());
  container_.reset(CreateNormalWindow(0, root_window(), nullptr));
  presenter_ = std::make_unique<AppListPresenterImpl>(
      std::make_unique<AppListPresenterDelegateFactoryTest>(container_.get()),
      nullptr);
  presenter_test_api_ =
      std::make_unique<test::AppListPresenterImplTestApi>(presenter());
}

void AppListPresenterImplTest::TearDown() {
  container_.reset();
  AuraTestBase::TearDown();
}

// Tests that app launcher is dismissed when focus moves to a window which is
// not app list window's sibling and that appropriate delegate callbacks are
// executed when the app launcher is shown and then when the app launcher is
// dismissed.
TEST_F(AppListPresenterImplTest, DISABLED_HideOnFocusOut) {
  // TODO(newcomer): this test needs to be reevaluated for the fullscreen app
  // list (http://crbug.com/759779).
  aura::client::FocusClient* focus_client =
      aura::client::GetFocusClient(root_window());
  presenter()->Show(GetDisplayId(), base::TimeTicks());
  EXPECT_TRUE(delegate()->init_called());
  EXPECT_TRUE(delegate()->on_shown_called());
  EXPECT_FALSE(delegate()->on_dismissed_called());
  focus_client->FocusWindow(presenter()->GetWindow());
  EXPECT_TRUE(presenter()->GetTargetVisibility());

  std::unique_ptr<aura::Window> window(
      CreateNormalWindow(1, root_window(), nullptr));
  focus_client->FocusWindow(window.get());

  EXPECT_TRUE(delegate()->on_dismissed_called());
  EXPECT_FALSE(presenter()->GetTargetVisibility());
}

// Tests that app launcher remains visible when focus moves to a window which
// is app list window's sibling and that appropriate delegate callbacks are
// executed when the app launcher is shown.
TEST_F(AppListPresenterImplTest, DISABLED_RemainVisibleWhenFocusingToSibling) {
  // TODO(newcomer): this test needs to be reevaluated for the fullscreen app
  // list (http://crbug.com/759779).
  aura::client::FocusClient* focus_client =
      aura::client::GetFocusClient(root_window());
  presenter()->Show(GetDisplayId(), base::TimeTicks());
  focus_client->FocusWindow(presenter()->GetWindow());
  EXPECT_TRUE(presenter()->GetTargetVisibility());
  EXPECT_TRUE(delegate()->init_called());
  EXPECT_TRUE(delegate()->on_shown_called());
  EXPECT_FALSE(delegate()->on_dismissed_called());

  // Create a sibling window.
  std::unique_ptr<aura::Window> window(
      CreateNormalWindow(1, container(), nullptr));
  focus_client->FocusWindow(window.get());

  EXPECT_TRUE(presenter()->GetTargetVisibility());
  EXPECT_FALSE(delegate()->on_dismissed_called());
}

// Tests that the app list is dismissed and the delegate is destroyed when the
// app list's widget is destroyed.
TEST_F(AppListPresenterImplTest, DISABLED_WidgetDestroyed) {
  // TODO(newcomer): this test needs to be reevaluated for the fullscreen app
  // list (http://crbug.com/759779).
  presenter()->Show(GetDisplayId(), base::TimeTicks());
  EXPECT_TRUE(presenter()->GetTargetVisibility());
  presenter()->GetView()->GetWidget()->CloseNow();
  EXPECT_FALSE(presenter()->GetTargetVisibility());
  EXPECT_FALSE(delegate());
}

}  // namespace app_list
