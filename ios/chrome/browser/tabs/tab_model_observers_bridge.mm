// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_observers_bridge.h"

#include "base/logging.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab_model_observers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TabModelObserversBridge {
  // The TabModel owning self.
  __weak TabModel* _tabModel;

  // The TabModelObservers that forward events to TabModelObserver instances
  // registered with owning TabModel.
  __weak TabModelObservers* _tabModelObservers;
}

- (instancetype)initWithTabModel:(TabModel*)tabModel
               tabModelObservers:(TabModelObservers*)tabModelObservers {
  DCHECK(tabModel);
  DCHECK(tabModelObservers);
  if ((self = [super init])) {
    _tabModel = tabModel;
    _tabModelObservers = tabModelObservers;
  }
  return self;
}

#pragma mark WebStateListObserving

- (void)webStateList:(WebStateList*)webStateList
    didInsertWebState:(web::WebState*)webState
              atIndex:(int)atIndex
           activating:(BOOL)activating {
  DCHECK_GE(atIndex, 0);
  [_tabModelObservers tabModel:_tabModel
                  didInsertTab:LegacyTabHelper::GetTabForWebState(webState)
                       atIndex:static_cast<NSUInteger>(atIndex)
                  inForeground:activating];
  [_tabModelObservers tabModelDidChangeTabCount:_tabModel];
}

- (void)webStateList:(WebStateList*)webStateList
     didMoveWebState:(web::WebState*)webState
           fromIndex:(int)fromIndex
             toIndex:(int)toIndex {
  DCHECK_GE(fromIndex, 0);
  DCHECK_GE(toIndex, 0);
  [_tabModelObservers tabModel:_tabModel
                    didMoveTab:LegacyTabHelper::GetTabForWebState(webState)
                     fromIndex:static_cast<NSUInteger>(fromIndex)
                       toIndex:static_cast<NSUInteger>(toIndex)];
}

- (void)webStateList:(WebStateList*)webStateList
    didReplaceWebState:(web::WebState*)oldWebState
          withWebState:(web::WebState*)newWebState
               atIndex:(int)atIndex {
  DCHECK_GE(atIndex, 0);
  [_tabModelObservers tabModel:_tabModel
                 didReplaceTab:LegacyTabHelper::GetTabForWebState(oldWebState)
                       withTab:LegacyTabHelper::GetTabForWebState(newWebState)
                       atIndex:static_cast<NSUInteger>(atIndex)];
}

- (void)webStateList:(WebStateList*)webStateList
    didDetachWebState:(web::WebState*)webState
              atIndex:(int)atIndex {
  DCHECK_GE(atIndex, 0);
  [_tabModelObservers tabModel:_tabModel
                  didRemoveTab:LegacyTabHelper::GetTabForWebState(webState)
                       atIndex:static_cast<NSUInteger>(atIndex)];
  [_tabModelObservers tabModelDidChangeTabCount:_tabModel];
}

- (void)webStateList:(WebStateList*)webStateList
    didChangeActiveWebState:(web::WebState*)newWebState
                oldWebState:(web::WebState*)oldWebState
                    atIndex:(int)atIndex
                     reason:(int)reason {
  if (!newWebState)
    return;

  // If there is no new active WebState, then it means that the atIndex will be
  // set to WebStateList::kInvalidIndex, so only check for a positive index if
  // there is a new WebState.
  DCHECK_GE(atIndex, 0);

  Tab* oldTab =
      oldWebState ? LegacyTabHelper::GetTabForWebState(oldWebState) : nil;
  [_tabModelObservers tabModel:_tabModel
            didChangeActiveTab:LegacyTabHelper::GetTabForWebState(newWebState)
                   previousTab:oldTab
                       atIndex:static_cast<NSUInteger>(atIndex)];
}

- (void)webStateList:(WebStateList*)webStateList
    willDetachWebState:(web::WebState*)webState
               atIndex:(int)atIndex {
  DCHECK_GE(atIndex, 0);
  [_tabModelObservers tabModel:_tabModel
                 willRemoveTab:LegacyTabHelper::GetTabForWebState(webState)];
}

@end
