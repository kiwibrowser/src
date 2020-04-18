// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/save_card_infobar_controller.h"

#include "base/strings/string16.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_save_card_infobar_delegate_mobile.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/infobars/infobar_controller+protected.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#import "ios/chrome/browser/ui/autofill/save_card_infobar_view.h"
#import "ios/chrome/browser/ui/autofill/save_card_infobar_view_delegate.h"
#import "ios/chrome/browser/ui/infobars/infobar_view_sizing_delegate.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Returns whether the UI Refresh Infobar will be used.
using ::IsRefreshInfobarEnabled;

// Returns the image for the infobar close button.
UIImage* InfoBarCloseImage() {
  if (!IsRefreshInfobarEnabled()) {
    return [UIImage imageNamed:@"infobar_close"];
  }
  ui::ResourceBundle& resourceBundle = ui::ResourceBundle::GetSharedInstance();
  return resourceBundle.GetNativeImageNamed(IDR_IOS_INFOBAR_CLOSE).ToUIImage();
}

// Returns the title for the given infobar button.
base::string16 GetTitleForButton(ConfirmInfoBarDelegate* delegate,
                                 ConfirmInfoBarDelegate::InfoBarButton button) {
  return (delegate->GetButtons() & button) ? delegate->GetButtonLabel(button)
                                           : base::string16();
}

}  // namespace

#pragma mark - SaveCardInfoBarController

@interface SaveCardInfoBarController ()<SaveCardInfoBarViewDelegate>

@property(nonatomic, assign)
    autofill::AutofillSaveCardInfoBarDelegateMobile* saveCardInfobarDelegate;

@property(nonatomic, weak) SaveCardInfoBarView* infoBarView;

@end

@implementation SaveCardInfoBarController

@synthesize saveCardInfobarDelegate = _saveCardInfobarDelegate;
@synthesize infoBarView = _infoBarView;

- (instancetype)initWithInfoBarDelegate:
    (autofill::AutofillSaveCardInfoBarDelegateMobile*)delegate {
  self = [super init];
  if (self) {
    self.saveCardInfobarDelegate = delegate;
  }
  return self;
}

- (UIView<InfoBarViewSizing>*)viewForFrame:(CGRect)frame {
  SaveCardInfoBarView* infoBarView =
      [[SaveCardInfoBarView alloc] initWithFrame:frame];
  self.infoBarView = infoBarView;
  self.infoBarView.delegate = self;

  // Close button.
  [self.infoBarView setCloseButtonImage:InfoBarCloseImage()];

  // Icon.
  gfx::Image icon = self.saveCardInfobarDelegate->GetIcon();
  DCHECK(!icon.IsEmpty());
  if (self.saveCardInfobarDelegate->IsGooglePayBrandingEnabled())
    [self.infoBarView setGooglePayIcon:icon.ToUIImage()];
  else
    [self.infoBarView setIcon:icon.ToUIImage()];

  // Message, if any.
  base::string16 messageText = self.saveCardInfobarDelegate->GetMessageText();
  if (!messageText.empty()) {
    MessageWithLinks* message = [[MessageWithLinks alloc] init];
    const base::string16 linkText = self.saveCardInfobarDelegate->GetLinkText();
    GURL linkURL = self.saveCardInfobarDelegate->GetLinkURL();

    if (!linkText.empty() && !linkURL.is_empty()) {
      std::vector<GURL> linkURLs;
      linkURLs.push_back(linkURL);
      message.linkURLs = linkURLs;
      message.linkRanges = [[NSArray alloc]
          initWithObjects:[NSValue valueWithRange:NSMakeRange(
                                                      messageText.length() + 1,
                                                      linkText.length())],
                          nil];
      // Append the link text to the message.
      messageText += base::UTF8ToUTF16(" ") + linkText;
    }
    message.messageText = base::SysUTF16ToNSString(messageText);
    [self.infoBarView setMessage:message];
  }

  // Description, if any.
  const base::string16 description =
      self.saveCardInfobarDelegate->GetDescriptionText();
  if (!description.empty()) {
    [self.infoBarView setDescription:base::SysUTF16ToNSString(description)];
  }

  // Card details.
  [self.infoBarView
      setCardIssuerIcon:NativeImage(
                            self.saveCardInfobarDelegate->issuer_icon_id())];
  [self.infoBarView
      setCardLabel:base::SysUTF16ToNSString(
                       self.saveCardInfobarDelegate->card_label())];
  [self.infoBarView
      setCardSublabel:base::SysUTF16ToNSString(
                          self.saveCardInfobarDelegate->card_sub_label())];

  // Legal messages, if any.
  if (!self.saveCardInfobarDelegate->legal_messages().empty()) {
    NSMutableArray* legalMessages = [[NSMutableArray alloc] init];
    for (const auto& line : self.saveCardInfobarDelegate->legal_messages()) {
      MessageWithLinks* message = [[MessageWithLinks alloc] init];
      message.messageText = base::SysUTF16ToNSString(line.text());
      NSMutableArray* linkRanges = [[NSMutableArray alloc] init];
      std::vector<GURL> linkURLs;
      for (const auto& link : line.links()) {
        [linkRanges addObject:[NSValue valueWithRange:link.range.ToNSRange()]];
        linkURLs.push_back(link.url);
      }
      message.linkRanges = linkRanges;
      message.linkURLs = linkURLs;
      [legalMessages addObject:message];
    }
    [self.infoBarView setLegalMessages:legalMessages];
  }

  // Cancel button.
  const base::string16 cancelButtonTitle = GetTitleForButton(
      self.saveCardInfobarDelegate, ConfirmInfoBarDelegate::BUTTON_CANCEL);
  [self.infoBarView
      setCancelButtonTitle:base::SysUTF16ToNSString(cancelButtonTitle)];

  // Confirm button.
  const base::string16 confirmButtonTitle = GetTitleForButton(
      self.saveCardInfobarDelegate, ConfirmInfoBarDelegate::BUTTON_OK);
  [self.infoBarView
      setConfirmButtonTitle:base::SysUTF16ToNSString(confirmButtonTitle)];

  return infoBarView;
}

#pragma mark - SaveCardInfoBarViewDelegate

- (void)saveCardInfoBarViewDidTapLink:(SaveCardInfoBarView*)sender {
  // Ignore this tap if the view has been detached from the delegate.
  if (!self.delegate) {
    return;
  }

  self.saveCardInfobarDelegate->LinkClicked(
      WindowOpenDisposition::NEW_FOREGROUND_TAB);
}

- (void)saveCardInfoBarView:(SaveCardInfoBarView*)sender
         didTapLegalLinkURL:(const GURL&)linkURL {
  // Ignore this tap if the view has been detached from the delegate.
  if (!self.delegate) {
    return;
  }

  self.saveCardInfobarDelegate->OnLegalMessageLinkClicked(linkURL);
}

- (void)saveCardInfoBarViewDidTapClose:(SaveCardInfoBarView*)sender {
  // Ignore this tap if the view has been detached from the delegate.
  if (!self.delegate) {
    return;
  }

  self.saveCardInfobarDelegate->InfoBarDismissed();
  self.delegate->RemoveInfoBar();
}

- (void)saveCardInfoBarViewDidTapCancel:(SaveCardInfoBarView*)sender {
  // Ignore this tap if the view has been detached from the delegate.
  if (!self.delegate) {
    return;
  }

  if (self.saveCardInfobarDelegate->Cancel()) {
    self.delegate->RemoveInfoBar();
  }
}

- (void)saveCardInfoBarViewDidTapConfirm:(SaveCardInfoBarView*)sender {
  // Ignore this tap if the view has been detached from the delegate.
  if (!self.delegate) {
    return;
  }

  if (self.saveCardInfobarDelegate->Accept()) {
    self.delegate->RemoveInfoBar();
  }
}

@end
