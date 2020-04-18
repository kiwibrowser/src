// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

namespace arc {
class IntentPickerController;
}  // namespace arc

class Browser;

// The entry point for the intent picker.
class IntentPickerView : public PageActionIconView {
 public:
  IntentPickerView(Browser* browser, PageActionIconView::Delegate* delegate);
  ~IntentPickerView() override;

  // PageActionIconView:
  void SetVisible(bool visible) override;

 protected:
  // PageActionIconView:
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;
  views::BubbleDialogDelegateView* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  base::string16 GetTextForTooltipAndAccessibleName() const override;

 private:
  bool IsIncognitoMode();

  std::unique_ptr<arc::IntentPickerController> intent_picker_controller_;

  Browser* const browser_;

  DISALLOW_COPY_AND_ASSIGN(IntentPickerView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_INTENT_PICKER_VIEW_H_
