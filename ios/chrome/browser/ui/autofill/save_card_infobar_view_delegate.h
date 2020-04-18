// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_DELEGATE_H_

class GURL;

// A protocol implemented by a delegate of SaveCardInfoBarView.
@protocol SaveCardInfoBarViewDelegate

// Notifies the delegate that user tapped the infobar link.
- (void)saveCardInfoBarViewDidTapLink:(SaveCardInfoBarView*)sender;

// Notifies the delegate that user tapped a legal link with |linkURL|.
- (void)saveCardInfoBarView:(SaveCardInfoBarView*)sender
         didTapLegalLinkURL:(const GURL&)linkURL;

// Notifies the delegate that user tapped the dismiss button.
- (void)saveCardInfoBarViewDidTapClose:(SaveCardInfoBarView*)sender;

// Notifies the delegate that user tapped the cancel button.
- (void)saveCardInfoBarViewDidTapCancel:(SaveCardInfoBarView*)sender;

// Notifies the delegate that user tapped the confirm button.
- (void)saveCardInfoBarViewDidTapConfirm:(SaveCardInfoBarView*)sender;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_SAVE_CARD_INFOBAR_VIEW_DELEGATE_H_
