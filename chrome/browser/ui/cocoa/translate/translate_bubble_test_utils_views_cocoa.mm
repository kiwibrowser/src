// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/translate/translate_bubble_test_utils.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/translate/translate_bubble_controller.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/views/controls/button/label_button.h"

// TODO(groby): Share with translate_bubble_controller_unittest.mm
@implementation BrowserWindowController (ForTesting)

- (TranslateBubbleController*)translateBubbleController {
  return translateBubbleController_;
}

@end

namespace translate {

namespace test_utils {

const TranslateBubbleModel* GetCurrentModel(Browser* browser) {
  DCHECK(browser);
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    TranslateBubbleView* view = TranslateBubbleView::GetCurrentBubble();
    return view ? view->model() : nullptr;
  }

  NSWindow* native_window = browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:native_window];
  return [[controller translateBubbleController] model];
}

void PressTranslate(Browser* browser) {
  DCHECK(browser);

  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    TranslateBubbleView* bubble = TranslateBubbleView::GetCurrentBubble();
    DCHECK(bubble);

    views::LabelButton button(nullptr, base::string16());
    button.set_id(TranslateBubbleView::BUTTON_ID_TRANSLATE);

    bubble->ButtonPressed(&button,
                          ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_RETURN,
                                       ui::DomCode::ENTER, ui::EF_NONE));
    return;
  }

  NSWindow* native_window = browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:native_window];
  [[controller translateBubbleController] handleTranslateButtonPressed:nil];
}

}  // namespace test_utils

}  // namespace translate
