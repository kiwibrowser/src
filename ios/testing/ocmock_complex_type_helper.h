// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_TESTING_OCMOCK_COMPLEX_TYPE_HELPER_H_
#define IOS_TESTING_OCMOCK_COMPLEX_TYPE_HELPER_H_

#import "third_party/google_toolbox_for_mac/src/Foundation/GTMLightweightProxy.h"
#import "third_party/ocmock/OCMock/OCMock.h"

// OCMock cannot check scalar arguments to method as it requires NSObjects to
// work its magic. This class tries to alleviate this restriction in a crude
// way. For an example of use, see the associated unittest class.
@interface OCMockComplexTypeHelper : GTMLightweightProxy
// As opposed to its parent class the OCMockComplexTypeHelper retains its
// represented object.
- (instancetype)initWithRepresentedObject:(id)object;
// Registers a block to be called when a selector is called.
- (void)onSelector:(SEL)selector callBlockExpectation:(id)block;
// Unregisters the block associated to the selector.
- (void)removeBlockExpectationOnSelector:(SEL)selector;
// Returns the block for the given selector. Intended for use by subclasses.
- (id)blockForSelector:(SEL)selector;
@end

#endif  // IOS_TESTING_OCMOCK_COMPLEX_TYPE_HELPER_H_
