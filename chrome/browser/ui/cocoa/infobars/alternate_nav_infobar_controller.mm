// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/alternate_nav_infobar_controller.h"

#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#include "chrome/browser/ui/omnibox/alternate_nav_infobar_delegate.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"
#include "ui/base/ui_features.h"
#include "ui/base/window_open_disposition.h"

@implementation AlternateNavInfoBarController

// Link infobars have a text message, of which part is linkified.  We
// use an NSAttributedString to display styled text, and we set a
// NSLink attribute on the hyperlink portion of the message.  Infobars
// use a custom NSTextField subclass, which allows us to override
// textView:clickedOnLink:atIndex: and intercept clicks.
//
- (void)addAdditionalControls {
  // No buttons.
  [self removeButtons];

  AlternateNavInfoBarDelegate* delegate =
      static_cast<AlternateNavInfoBarDelegate*>([self delegate]);
  DCHECK(delegate);
  size_t offset = base::string16::npos;
  base::string16 message = delegate->GetMessageTextWithOffset(&offset);
  base::string16 link = delegate->GetLinkText();
  message.insert(offset, link);
  NSFont* font = [NSFont labelFontOfSize:
                  [NSFont systemFontSizeForControlSize:NSRegularControlSize]];
  HyperlinkTextView* view = (HyperlinkTextView*)label_.get();
  [view setMessage:base::SysUTF16ToNSString(message)
          withFont:font
      messageColor:[NSColor blackColor]];
  [view addLinkRange:NSMakeRange(offset, link.length())
             withURL:base::SysUTF8ToNSString(delegate->GetLinkURL().spec())
           linkColor:[NSColor blueColor]];
}

// Called when someone clicks on the link in the infobar.  This method
// is called by the InfobarTextField on its delegate (the
// AlternateNavInfoBarController).
- (void)linkClicked {
  if (![self isOwned])
    return;
  WindowOpenDisposition disposition =
      ui::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  AlternateNavInfoBarDelegate* delegate =
      static_cast<AlternateNavInfoBarDelegate*>([self delegate]);
  if (delegate->LinkClicked(disposition))
    [self removeSelf];
}

@end

// static
std::unique_ptr<infobars::InfoBar>
AlternateNavInfoBarDelegate::CreateInfoBarCocoa(
    std::unique_ptr<AlternateNavInfoBarDelegate> delegate) {
  std::unique_ptr<InfoBarCocoa> infobar(new InfoBarCocoa(std::move(delegate)));
  base::scoped_nsobject<AlternateNavInfoBarController> controller(
      [[AlternateNavInfoBarController alloc] initWithInfoBar:infobar.get()]);
  infobar->set_controller(controller);
  return std::move(infobar);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
std::unique_ptr<infobars::InfoBar> AlternateNavInfoBarDelegate::CreateInfoBar(
    std::unique_ptr<AlternateNavInfoBarDelegate> delegate) {
  return CreateInfoBarCocoa(std::move(delegate));
}

#endif
