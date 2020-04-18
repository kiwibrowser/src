// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/sad_tab/sad_tab_legacy_coordinator.h"

#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/sad_tab/sad_tab_view.h"
#import "ios/chrome/browser/web/sad_tab_tab_helper.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/ui/crw_generic_content_view.h"
#include "ios/web/public/web_state/web_state.h"

@interface SadTabLegacyCoordinator ()<SadTabActionDelegate>
@end

@implementation SadTabLegacyCoordinator
@synthesize baseViewController = _baseViewController;
@synthesize dispatcher = _dispatcher;

#pragma mark - SadTabActionDelegate

- (void)showReportAnIssue {
  [self.dispatcher showReportAnIssueFromViewController:self.baseViewController];
}

#pragma mark - SadTabTabHelperDelegate

- (void)sadTabTabHelper:(SadTabTabHelper*)tabHelper
    presentSadTabForWebState:(web::WebState*)webState
             repeatedFailure:(BOOL)repeatedFailure {
  // Create a SadTabView so |webstate| presents it.
  SadTabView* sadTabview = [[SadTabView alloc]
           initWithMode:repeatedFailure ? SadTabViewMode::FEEDBACK
                                        : SadTabViewMode::RELOAD
      navigationManager:webState->GetNavigationManager()];
  sadTabview.dispatcher = static_cast<id<ApplicationCommands>>(self.dispatcher);
  sadTabview.actionDelegate = self;
  CRWContentView* contentView =
      [[CRWGenericContentView alloc] initWithView:sadTabview];
  webState->ShowTransientContentView(contentView);
}

@end
