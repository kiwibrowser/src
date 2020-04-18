// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_PRESENTER_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_PRESENTER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/presenters/contained_presenter.h"
#import "ios/chrome/browser/ui/util/named_guide.h"

@protocol PopupMenuCommands;

// Presenter for the popup menu. It handles showing/dismissing a popup menu.
@interface PopupMenuPresenter : NSObject<ContainedPresenter>

// CommandHandler.
@property(nonatomic, weak) id<PopupMenuCommands> commandHandler;
// Guide name used for the presentation.
@property(nonatomic, strong) GuideName* guideName;

@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_PRESENTER_H_
