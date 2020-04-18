// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_H_

#import <UIKit/UIKit.h>

#include <vector>

#import "ios/chrome/browser/ui/infobars/infobar_view_sizing.h"

class GURL;
@protocol SaveCardInfoBarViewDelegate;

// Represents a message with optional links.
@interface MessageWithLinks : NSObject

@property(nonatomic, copy) NSString* messageText;

@property(nonatomic, strong) NSArray* linkRanges;

@property(nonatomic, assign) std::vector<GURL> linkURLs;

@end

// An infobar for saving credit cards. It features a header section including
// an optional leading icon, followed by an optional message, followed by an
// optional close button. Below that, there is an optional description. In the
// next section, credit card details appear including the issuer network icon,
// the card's label, and the card's sublabel. In the following section, optional
// legal messages appear. The bottom section is the footer which contains a
// an optional confirm button and an optional cancel button.
@interface SaveCardInfoBarView : UIView<InfoBarViewSizing>

@property(nonatomic, weak) id<SaveCardInfoBarViewDelegate> delegate;

@property(nonatomic, strong) UIImage* icon;

@property(nonatomic, strong) UIImage* googlePayIcon;

@property(nonatomic, strong) MessageWithLinks* message;

@property(nonatomic, strong) UIImage* closeButtonImage;

@property(nonatomic, copy) NSString* description;

@property(nonatomic, strong) UIImage* cardIssuerIcon;

@property(nonatomic, copy) NSString* cardLabel;

@property(nonatomic, copy) NSString* cardSublabel;

@property(nonatomic, strong) NSArray<MessageWithLinks*>* legalMessages;

@property(nonatomic, copy) NSString* cancelButtonTitle;

@property(nonatomic, copy) NSString* confirmButtonTitle;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_H_
