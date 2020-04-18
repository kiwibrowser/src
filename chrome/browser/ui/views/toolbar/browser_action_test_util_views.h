// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_BROWSER_ACTION_TEST_UTIL_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_BROWSER_ACTION_TEST_UTIL_VIEWS_H_

#include "chrome/browser/ui/extensions/browser_action_test_util.h"

class BrowserActionTestUtilViews : public BrowserActionTestUtil {
 public:
  explicit BrowserActionTestUtilViews(Browser* browser);
  BrowserActionTestUtilViews(Browser* browser, bool is_real_window);
  ~BrowserActionTestUtilViews() override;

  // BrowserActionTestUtil:
  int NumberOfBrowserActions() override;
  int VisibleBrowserActions() override;
  void InspectPopup(int index) override;
  bool HasIcon(int index) override;
  gfx::Image GetIcon(int index) override;
  void Press(int index) override;
  std::string GetExtensionId(int index) override;
  std::string GetTooltip(int index) override;
  gfx::NativeView GetPopupNativeView() override;
  bool HasPopup() override;
  gfx::Size GetPopupSize() override;
  bool HidePopup() override;
  bool ActionButtonWantsToRun(size_t index) override;
  void SetWidth(int width) override;
  ToolbarActionsBar* GetToolbarActionsBar() override;
  std::unique_ptr<BrowserActionTestUtil> CreateOverflowBar() override;
  gfx::Size GetMinPopupSize() override;
  gfx::Size GetMaxPopupSize() override;
  bool CanBeResized() override;

 private:
  friend class BrowserActionTestUtil;

  Browser* const browser_;  // weak

  // Our test helper, which constructs and owns the views if we don't have a
  // real browser window, or if this is an overflow version.
  std::unique_ptr<TestToolbarActionsBarHelper> test_helper_;

  BrowserActionTestUtilViews(Browser* browser,
                             BrowserActionTestUtilViews* main_bar);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_BROWSER_ACTION_TEST_UTIL_VIEWS_H_
