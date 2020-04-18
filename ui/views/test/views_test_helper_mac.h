// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TEST_VIEWS_TEST_HELPER_MAC_H_
#define UI_VIEWS_TEST_VIEWS_TEST_HELPER_MAC_H_

#include <memory>

#include "base/macros.h"
#include "ui/views/test/views_test_helper.h"

namespace ui {
namespace test {
class ScopedFakeFullKeyboardAccess;
class ScopedFakeNSWindowFocus;
class ScopedFakeNSWindowFullscreen;
}
class ScopedAnimationDurationScaleMode;
}

namespace views {

class ViewsTestHelperMac : public ViewsTestHelper {
 public:
  ViewsTestHelperMac();
  ~ViewsTestHelperMac() override;

  // ViewsTestHelper:
  void SetUp() override;
  void TearDown() override;

 private:
  // Disable animations during tests.
  std::unique_ptr<ui::ScopedAnimationDurationScaleMode> zero_duration_mode_;

  // When using desktop widgets on Mac, window activation is asynchronous
  // because the window server is involved. A window may also be deactivated by
  // a test running in parallel, making it flaky. In non-interactive/sharded
  // tests, |faked_focus_| is initialized, permitting a unit test to "fake" this
  // activation, causing it to be synchronous and per-process instead.
  std::unique_ptr<ui::test::ScopedFakeNSWindowFocus> faked_focus_;

  // Toggling fullscreen mode on Mac can be flaky for tests run in parallel
  // because only one window may be animating into or out of fullscreen at a
  // time. In non-interactive/sharded tests, |faked_fullscreen_| is initialized,
  // permitting a unit test to 'fake' toggling fullscreen mode.
  std::unique_ptr<ui::test::ScopedFakeNSWindowFullscreen> faked_fullscreen_;

  // Enable fake full keyboard access by default, so that tests don't depend on
  // system setting of the test machine. Also, this helps to make tests on Mac
  // more consistent with other platforms, where most views are focusable by
  // default.
  std::unique_ptr<ui::test::ScopedFakeFullKeyboardAccess>
      faked_full_keyboard_access_;

  DISALLOW_COPY_AND_ASSIGN(ViewsTestHelperMac);
};

}  // namespace views

#endif  // UI_VIEWS_TEST_VIEWS_TEST_HELPER_MAC_H_
