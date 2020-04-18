// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_INCOGNITO_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_NTP_INCOGNITO_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/new_tab_page_panel_protocol.h"

@protocol NewTabPageControllerDelegate;
@protocol UrlLoader;

@interface IncognitoViewController : UIViewController<NewTabPagePanelProtocol>

// Init with the given loader object. |loader| may be nil, but isn't
// retained so it must outlive this controller.
// |toolbarDelegate| is used to fade the toolbar views on page scroll.
- (id)initWithLoader:(id<UrlLoader>)loader
     toolbarDelegate:(id<NewTabPageControllerDelegate>)toolbarDelegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_INCOGNITO_VIEW_CONTROLLER_H_
