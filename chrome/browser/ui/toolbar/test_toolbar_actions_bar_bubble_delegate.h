// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTIONS_BAR_BUBBLE_DELEGATE_H_
#define CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTIONS_BAR_BUBBLE_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "ui/base/ui_base_types.h"

// A test delegate for a bubble to hang off the toolbar actions bar.
class TestToolbarActionsBarBubbleDelegate {
 public:
  TestToolbarActionsBarBubbleDelegate(
      const base::string16& heading,
      const base::string16& body,
      const base::string16& action);
  ~TestToolbarActionsBarBubbleDelegate();

  // Returns a delegate to pass to the bubble. Since the bubble typically owns
  // the delegate, it means we can't have this object be the delegate, because
  // it would be deleted once the bubble closes.
  std::unique_ptr<ToolbarActionsBarBubbleDelegate> GetDelegate();

  void set_action_button_text(const base::string16& action) {
    action_ = action;
  }
  void set_dismiss_button_text(const base::string16& dismiss) {
    dismiss_ = dismiss;
  }
  void set_learn_more_button_text(const base::string16& learn_more) {
    learn_more_ = learn_more;

    if (!info_) {
      info_ =
          std::make_unique<ToolbarActionsBarBubbleDelegate::ExtraViewInfo>();
    }
    info_->text = learn_more;
    info_->is_learn_more = true;
  }
  void set_default_dialog_button(ui::DialogButton default_button) {
    default_button_ = default_button;
  }
  void set_item_list_text(const base::string16& item_list) {
    item_list_ = item_list;
  }
  void set_close_on_deactivate(bool close_on_deactivate) {
    close_on_deactivate_ = close_on_deactivate;
  }
  void set_extra_view_info(
      std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo> info) {
    info_ = std::move(info);
  }
  const ToolbarActionsBarBubbleDelegate::CloseAction* close_action() const {
    return close_action_.get();
  }
  bool shown() const { return shown_; }

 private:
  class DelegateImpl;

  // Whether or not the bubble has been shown.
  bool shown_;

  // The action that was taken to close the bubble.
  std::unique_ptr<ToolbarActionsBarBubbleDelegate::CloseAction> close_action_;

  // Strings for the bubble.
  base::string16 heading_;
  base::string16 body_;
  base::string16 action_;
  base::string16 dismiss_;
  base::string16 learn_more_;
  base::string16 item_list_;

  // The default button for the bubble.
  ui::DialogButton default_button_;

  // Whether to close the bubble on deactivation.
  bool close_on_deactivate_;

  // Information about the extra view to show, if any.
  std::unique_ptr<ToolbarActionsBarBubbleDelegate::ExtraViewInfo> info_;

  DISALLOW_COPY_AND_ASSIGN(TestToolbarActionsBarBubbleDelegate);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TEST_TOOLBAR_ACTIONS_BAR_BUBBLE_DELEGATE_H_
