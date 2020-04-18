// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_ICON_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_ICON_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

class CommandUpdater;

// The location bar icon to show the Translate bubble where the user can have
// the page translated.
class TranslateIconView : public PageActionIconView {
 public:
  TranslateIconView(CommandUpdater* command_updater,
                    PageActionIconView::Delegate* delegate);
  ~TranslateIconView() override;

  // PageActionIconView:
  views::BubbleDialogDelegateView* GetBubble() const override;
  bool Refresh() override;

 protected:
  // PageActionIconView:
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;
  void OnPressed(bool activated) override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  base::string16 GetTextForTooltipAndAccessibleName() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TranslateIconView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_ICON_VIEW_H_
