// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTION_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTION_VIEW_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"

// A minimalistic and configurable ToolbarActionViewController for use in
// testing.
class TestToolbarActionViewController : public ToolbarActionViewController {
 public:
  explicit TestToolbarActionViewController(const std::string& id);
  ~TestToolbarActionViewController() override;

  // ToolbarActionViewController:
  std::string GetId() const override;
  void SetDelegate(ToolbarActionViewDelegate* delegate) override;
  gfx::Image GetIcon(content::WebContents* web_contents,
                     const gfx::Size& size,
                     ToolbarActionButtonState state) override;
  base::string16 GetActionName() const override;
  base::string16 GetAccessibleName(content::WebContents* web_contents)
      const override;
  base::string16 GetTooltip(content::WebContents* web_contents)
      const override;
  bool IsEnabled(content::WebContents* web_contents) const override;
  bool WantsToRun(content::WebContents* web_contents) const override;
  bool HasPopup(content::WebContents* web_contents) const override;
  void HidePopup() override;
  gfx::NativeView GetPopupNativeView() override;
  ui::MenuModel* GetContextMenu() override;
  bool ExecuteAction(bool by_user) override;
  void UpdateState() override;
  bool DisabledClickOpensMenu() const override;

  // Instruct the controller to fake showing a popup.
  void ShowPopup(bool by_user);

  // Configure the test controller. These also call UpdateDelegate().
  void SetAccessibleName(const base::string16& name);
  void SetTooltip(const base::string16& tooltip);
  void SetEnabled(bool is_enabled);
  void SetWantsToRun(bool wants_to_run);
  void SetDisabledClickOpensMenu(bool disabled_click_opens_menu);

  int execute_action_count() const { return execute_action_count_; }

 private:
  // Updates the delegate, if one exists.
  void UpdateDelegate();

  // The id of the controller.
  std::string id_;

  // The delegate of the controller, if one exists.
  ToolbarActionViewDelegate* delegate_;

  // The optional accessible name and tooltip; by default these are empty.
  base::string16 accessible_name_;
  base::string16 tooltip_;

  // Whether or not the action is enabled. Defaults to true.
  bool is_enabled_;

  // Whether or not the action wants to run. Defaults to false.
  bool wants_to_run_;

  // Whether or not a click on a disabled action should open the context menu.
  bool disabled_click_opens_menu_;

  // The number of times the action would have been executed.
  int execute_action_count_;

  DISALLOW_COPY_AND_ASSIGN(TestToolbarActionViewController);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTION_VIEW_CONTROLLER_H_
