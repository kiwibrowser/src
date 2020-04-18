// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_VIEWS_H_

namespace autofill {
class SaveCardBubbleController;
class SaveCardBubbleView;
}  // namespace autofill

namespace content {
class WebContents;
}

namespace autofill {

// Creates the SaveCardBubbleView implementation.
SaveCardBubbleView* CreateSaveCardBubbleView(
    content::WebContents* web_contents,
    autofill::SaveCardBubbleController* controller,
    BrowserWindowController* browser_window_controller,
    bool user_gesture);

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_COCOA_AUTOFILL_SAVE_CARD_BUBBLE_VIEW_VIEWS_H_
