// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORDS_ICON_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORDS_ICON_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/passwords/manage_passwords_bubble_model.h"
#include "chrome/browser/ui/passwords/manage_passwords_icon_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "ui/views/controls/image_view.h"

class CommandUpdater;

// View for the password icon in the Omnibox.
class ManagePasswordsIconViews : public ManagePasswordsIconView,
                                 public PageActionIconView {
 public:
  ManagePasswordsIconViews(CommandUpdater* updater,
                           PageActionIconView::Delegate* delegate);
  ~ManagePasswordsIconViews() override;

  // ManagePasswordsIconView:
  void SetState(password_manager::ui::State state) override;

  // PageActionIconView:
  views::BubbleDialogDelegateView* GetBubble() const override;
  bool Refresh() override;
  void OnExecuting(PageActionIconView::ExecuteSource source) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  base::string16 GetTextForTooltipAndAccessibleName() const override;

  // views::View:
  void AboutToRequestFocusFromTabTraversal(bool reverse) override;

 private:
  friend class ManagePasswordsIconViewTest;

  // Updates the UI to match |state_|.
  void UpdateUiForState();

  password_manager::ui::State state_;

  DISALLOW_COPY_AND_ASSIGN(ManagePasswordsIconViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORDS_ICON_VIEWS_H_
