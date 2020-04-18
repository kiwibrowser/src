// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/crw_wk_navigation_states.h"

#include "base/logging.h"
#import "ios/web/web_state/navigation_context_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Holds a pair of state and creation order index.
@interface CRWWKNavigationsStateRecord : NSObject {
  // Backs up |context| property.
  std::unique_ptr<web::NavigationContextImpl> _context;
}
// Navigation state.
@property(nonatomic, assign) web::WKNavigationState state;
// Numerical index representing creation order (smaller index denotes earlier
// navigations).
@property(nonatomic, assign, readonly) NSUInteger index;

// didCommitNavigation: can be called multiple times for the same navigation.
@property(nonatomic, assign, getter=isCommitted) BOOL committed;

- (instancetype)init NS_UNAVAILABLE;

// Initializes record with state and index values.
- (instancetype)initWithState:(web::WKNavigationState)state
                        index:(NSUInteger)index NS_DESIGNATED_INITIALIZER;

// Initializes record with context and index values.
- (instancetype)initWithContext:
                    (std::unique_ptr<web::NavigationContextImpl>)context
                          index:(NSUInteger)index NS_DESIGNATED_INITIALIZER;

// web::NavigationContextImpl for this navigation.
- (web::NavigationContextImpl*)context;
- (void)setContext:(std::unique_ptr<web::NavigationContextImpl>)context;

@end

@implementation CRWWKNavigationsStateRecord
@synthesize state = _state;
@synthesize index = _index;
@synthesize committed = _committed;

#ifndef NDEBUG
- (NSString*)description {
  return [NSString stringWithFormat:@"state: %d, index: %ld, context: %@",
                                    _state, static_cast<long>(_index),
                                    _context->GetDescription()];
}
#endif  // NDEBUG

- (instancetype)initWithState:(web::WKNavigationState)state
                        index:(NSUInteger)index {
  if ((self = [super init])) {
    _state = state;
    _index = index;
  }
  return self;
}

- (instancetype)initWithContext:
                    (std::unique_ptr<web::NavigationContextImpl>)context
                          index:(NSUInteger)index {
  if ((self = [super init])) {
    _context = std::move(context);
    _index = index;
  }
  return self;
}

- (void)setContext:(std::unique_ptr<web::NavigationContextImpl>)context {
  _context = std::move(context);
}

- (web::NavigationContextImpl*)context {
  return _context.get();
}

@end

@interface CRWWKNavigationStates () {
  NSMapTable* _records;
  NSUInteger _lastStateIndex;
  WKNavigation* _nullNavigation;
}

// Returns key to use for storing navigation in records table.
- (id)keyForNavigation:(WKNavigation*)navigation;

// Returns last added navigation and record.
- (void)getLastAddedNavigation:(WKNavigation**)outNavigation
                        record:(CRWWKNavigationsStateRecord**)outRecord;

@end

@implementation CRWWKNavigationStates

- (instancetype)init {
  if ((self = [super init])) {
    _records = [NSMapTable weakToStrongObjectsMapTable];
    _nullNavigation = static_cast<WKNavigation*>([NSNull null]);
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"records: %@, lastAddedNavigation: %@",
                                    _records, self.lastAddedNavigation];
}

- (void)setState:(web::WKNavigationState)state
    forNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  CRWWKNavigationsStateRecord* record = [_records objectForKey:key];
  if (!record) {
    DCHECK(state == web::WKNavigationState::REQUESTED ||
           state == web::WKNavigationState::STARTED ||
           state == web::WKNavigationState::COMMITTED);
    record =
        [[CRWWKNavigationsStateRecord alloc] initWithState:state
                                                     index:++_lastStateIndex];
  } else {
    DCHECK(record.state < state ||
           // Redirect can be called multiple times.
           (record.state == state &&
            state == web::WKNavigationState::REDIRECTED) ||
           // didFinishNavigation can be called before didCommitNvigation.
           (record.state == web::WKNavigationState::FINISHED &&
            state == web::WKNavigationState::COMMITTED));
    record.state = state;
  }
  if (state == web::WKNavigationState::COMMITTED) {
    record.committed = YES;
  }
  [_records setObject:record forKey:key];
}

- (web::WKNavigationState)stateForNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  CRWWKNavigationsStateRecord* record = [_records objectForKey:key];
  return record ? record.state : web::WKNavigationState::NONE;
}

- (void)removeNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  DCHECK([_records objectForKey:key]);
  [_records removeObjectForKey:key];
}

- (void)setContext:(std::unique_ptr<web::NavigationContextImpl>)context
     forNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  CRWWKNavigationsStateRecord* record = [_records objectForKey:key];
  if (!record) {
    record =
        [[CRWWKNavigationsStateRecord alloc] initWithContext:std::move(context)
                                                       index:++_lastStateIndex];
  } else {
    [record setContext:std::move(context)];
  }
  [_records setObject:record forKey:key];
}

- (web::NavigationContextImpl*)contextForNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  CRWWKNavigationsStateRecord* record = [_records objectForKey:key];
  return [record context];
}

- (WKNavigation*)lastAddedNavigation {
  WKNavigation* result = nil;
  CRWWKNavigationsStateRecord* unused = nil;
  [self getLastAddedNavigation:&result record:&unused];
  return result;
}

- (web::WKNavigationState)lastAddedNavigationState {
  CRWWKNavigationsStateRecord* result = nil;
  WKNavigation* unused = nil;
  [self getLastAddedNavigation:&unused record:&result];
  return result.state;
}

- (NSSet*)pendingNavigations {
  NSMutableSet* result = [NSMutableSet set];
  for (id navigation in _records) {
    CRWWKNavigationsStateRecord* record = [_records objectForKey:navigation];
    if (record.state == web::WKNavigationState::REQUESTED ||
        record.state == web::WKNavigationState::STARTED ||
        record.state == web::WKNavigationState::REDIRECTED) {
      [result addObject:navigation];
    }
  }
  return [result copy];
}

- (id)keyForNavigation:(WKNavigation*)navigation {
  return navigation ? navigation : _nullNavigation;
}

- (void)getLastAddedNavigation:(WKNavigation**)outNavigation
                        record:(CRWWKNavigationsStateRecord**)outRecord {
  NSUInteger lastAddedIndex = 0;  // record indices start with 1.
  for (WKNavigation* navigation in _records) {
    CRWWKNavigationsStateRecord* record = [_records objectForKey:navigation];
    if (lastAddedIndex < record.index) {
      *outNavigation = navigation;
      *outRecord = record;
      lastAddedIndex = record.index;
    }
  }

  if (*outNavigation == _nullNavigation) {
    // |_nullNavigation| is a key for storing null navigations.
    *outNavigation = nil;
  }
}

- (BOOL)isCommittedNavigation:(WKNavigation*)navigation {
  id key = [self keyForNavigation:navigation];
  CRWWKNavigationsStateRecord* record = [_records objectForKey:key];
  return record.committed;
}

@end
