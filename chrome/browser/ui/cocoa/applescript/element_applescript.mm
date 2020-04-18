// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/applescript/element_applescript.h"

@implementation ElementAppleScript

@synthesize uniqueID = uniqueID_;
@synthesize container = container_;
@synthesize containerProperty = containerProperty_;

// calling objectSpecifier asks an object to return an object specifier
// record referring to itself.  You must call setContainer:property: before
// you can call this method.
- (NSScriptObjectSpecifier*)objectSpecifier {
  return [[[NSUniqueIDSpecifier allocWithZone:[self zone]]
      initWithContainerClassDescription:
          (NSScriptClassDescription*)[[self container] classDescription]
                     containerSpecifier:[[self container] objectSpecifier]
                                    key:[self containerProperty]
                               uniqueID:[self uniqueID]] autorelease];
}

- (void)setContainer:(id)value property:(NSString*)property {
  [self setContainer:value];
  [self setContainerProperty:property];
}

- (void)dealloc {
  [uniqueID_ release];
  [container_ release];
  [containerProperty_ release];
  [super dealloc];
}

@end
