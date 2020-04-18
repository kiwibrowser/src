// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/testing/ocmock_complex_type_helper.h"

#include "base/logging.h"
#import "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation OCMockComplexTypeHelper {
  // Same as the superclass -representedObject, but retained.
  OCMockObject* _object;
  // All the blocks registered by selector.
  NSMutableDictionary* _blocks;
}

#pragma mark - public methods.

- (instancetype)initWithRepresentedObject:(id)object {
  if ((self = [super initWithRepresentedObject:object]))
    _object = object;
  return self;
}

- (void)onSelector:(SEL)selector callBlockExpectation:(id)block {
  if (!_blocks)
    _blocks = [[NSMutableDictionary alloc] init];

  NSString* key = NSStringFromSelector(selector);
  DCHECK(![_blocks objectForKey:key]) << "Only one expectation per signature";
  id value = [block copy];
  [_blocks setObject:value forKey:key];
}

- (void)removeBlockExpectationOnSelector:(SEL)selector {
  NSString* key = NSStringFromSelector(selector);
  DCHECK([_blocks objectForKey:key])
      << "No expectation for selector " << base::SysNSStringToUTF8(key);
  [_blocks removeObjectForKey:key];
}

- (id)blockForSelector:(SEL)selector {
  NSString* key = NSStringFromSelector(selector);
  id block = [_blocks objectForKey:key];
  DCHECK(block) << "Missing block expectation for selector "
                << base::SysNSStringToUTF8(key);
  return block;
}

#pragma mark - OCMockObject forwarding.

// OCMockObject -respondsToSelector responds NO for the OCMock object specific
// methods. This confuses the GTMLightweightProxy class. In order to forward
// those properly the simplest approach is to forward them explicitely.
- (id)stub {
  return [_object stub];
}
- (id)expect {
  return [_object expect];
}
- (id)reject {
  return [_object reject];
}
- (void)verify {
  [_object verify];
}
- (void)setExpectationOrderMatters:(BOOL)flag {
  [_object setExpectationOrderMatters:flag];
}

#pragma mark - Internal methods.

- (BOOL)respondsToSelector:(SEL)selector {
  DCHECK(![_blocks objectForKey:NSStringFromSelector(selector)]);
  return [super respondsToSelector:selector];
}

@end
