// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_BUTTON_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_BUTTON_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class NewTabPageBarItem;

namespace new_tab_page_bar_button {

enum class ContentType {
  IMAGE,
  TEXT,
};

}  // namespace new_tab_page_bar_button

// Represents a button in the new tab page bar.
@interface NewTabPageBarButton : UIButton

// Returns an autoreleased button based on |item|'s |title| and |image|. By
// defaults, the button shows the title instead of the image, and with a non
// incognito color scheme.
+ (instancetype)buttonWithItem:(NewTabPageBarItem*)item;

// Selects which color scheme to use for the buttons. If |percentage| is 1.0,
// the color scheme is for incognito. If |percentage| is 0.0, the color scheme
// is for non incognito. If |percentage| is in-between, the color scheme is an
// interpolation between the two.
// Percentage must be in the interval [0, 1].
- (void)useIncognitoColorScheme:(CGFloat)percentage;

// Selects which kind of content the button should display.
- (void)setContentToDisplay:(new_tab_page_bar_button::ContentType)contentType;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_BUTTON_H_
