// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_side_swipe_provider.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "components/reading_list/core/reading_list_entry.h"
#include "components/reading_list/core/reading_list_model.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/metrics/new_tab_page_uma.h"
#include "ios/web/public/web_state/web_state.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class ReadingListObserverBridge;

@interface ReadingListSideSwipeProvider () {
  // Keep a reference to detach before deallocing.
  ReadingListModel* _readingListModel;  // weak
}

@end

@implementation ReadingListSideSwipeProvider
- (instancetype)initWithReadingList:(ReadingListModel*)readingListModel {
  if (self = [super init]) {
    _readingListModel = readingListModel;
  }
  return self;
}

- (BOOL)canGoBack {
  return NO;
}

- (void)goBack:(web::WebState*)webState {
  NOTREACHED();
}

- (BOOL)canGoForward {
  return _readingListModel->unread_size() > 0;
}

- (UIImage*)paneIcon {
  return [UIImage imageNamed:@"reading_list_side_swipe"];
}

- (BOOL)rotateForwardIcon {
  return NO;
}

- (void)goForward:(web::WebState*)webState {
  if (!webState || _readingListModel->unread_size() == 0) {
    return;
  }
  const ReadingListEntry* firstEntry = _readingListModel->GetFirstUnreadEntry(
      net::NetworkChangeNotifier::IsOffline());
  DCHECK(firstEntry);
  base::RecordAction(base::UserMetricsAction("MobileReadingListOpen"));
  new_tab_page_uma::RecordAction(
      ios::ChromeBrowserState::FromBrowserState(webState->GetBrowserState()),
      new_tab_page_uma::ACTION_OPENED_READING_LIST_ENTRY);

  web::NavigationManager::WebLoadParams params(firstEntry->URL());
  params.transition_type = ui::PageTransition::PAGE_TRANSITION_AUTO_BOOKMARK;
  webState->GetNavigationManager()->LoadURLWithParams(params);
}

@end
