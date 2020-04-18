// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"

class ToolbarActionsBarBubbleViewsTest;

namespace views {
class ImageButton;
class Label;
}

class ToolbarActionsBarBubbleViews : public views::BubbleDialogDelegateView,
                                     public views::ButtonListener {
 public:
  // Creates the bubble anchored to |anchor_view| or, if that is null, to
  // |anchor_point| in screen coordinates.
  ToolbarActionsBarBubbleViews(
      views::View* anchor_view,
      const gfx::Point& anchor_point,
      bool anchored_to_action,
      std::unique_ptr<ToolbarActionsBarBubbleDelegate> delegate);
  ~ToolbarActionsBarBubbleViews() override;

  void Show();

  const views::Label* body_text() const { return body_text_; }
  const views::Label* item_list() const { return item_list_; }
  views::ImageButton* learn_more_button() const { return image_button_; }

 private:
  friend class ToolbarActionsBarBubbleViewsTest;

  // views::BubbleDialogDelegateView:
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  views::View* CreateExtraView() override;
  bool Cancel() override;
  bool Accept() override;
  bool Close() override;
  int GetDialogButtons() const override;
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  void Init() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  std::unique_ptr<ToolbarActionsBarBubbleDelegate> delegate_;
  bool delegate_notified_of_close_ = false;
  views::Label* body_text_ = nullptr;
  views::Label* item_list_ = nullptr;
  views::ImageButton* image_button_ = nullptr;
  const bool anchored_to_action_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBarBubbleViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_H_
