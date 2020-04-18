// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/app/application_delegate/mock_tab_opener.h"

#include "base/ios/block_types.h"
#include "base/mac/scoped_block.h"
#include "ios/chrome/app/application_mode.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MockTabOpener

@synthesize url = _url;
@synthesize applicationMode = _applicationMode;
@synthesize completionBlock = _completionBlock;

- (void)dismissModalsAndOpenSelectedTabInMode:(ApplicationMode)targetMode
                                      withURL:(const GURL&)url
                               dismissOmnibox:(BOOL)dismissOmnibox
                                   transition:(ui::PageTransition)transition
                                   completion:(ProceduralBlock)completion {
  _url = url;
  _applicationMode = targetMode;
  _completionBlock = [completion copy];
}

- (void)resetURL {
  _url = _url.EmptyGURL();
}

- (void)openTabFromLaunchOptions:(NSDictionary*)launchOptions
              startupInformation:(id<StartupInformation>)startupInformation
                        appState:(AppState*)appState {
  // Stub.
}

- (BOOL)shouldOpenNTPTabOnActivationOfTabModel:(TabModel*)tabModel {
  // Stub.
  return YES;
}

- (ProceduralBlock)completionBlockForTriggeringAction:
    (NTPTabOpeningPostOpeningAction)action {
  // Stub
  return nil;
}

- (BOOL)shouldCompletePaymentRequestOnCurrentTab:
    (id<StartupInformation>)startupInformation {
  // Stub.
  return NO;
}

@end
