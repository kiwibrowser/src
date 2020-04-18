// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/collected_cookies_mac.h"

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "testing/gtest_mac.h"

class CollectedCookiesMacTest : public CocoaTest {
 public:
  NSTreeNode* CreateTreeNodesFromDictionary(NSDictionary* dict) {
    base::scoped_nsobject<NSTreeNode> root(
        [[NSTreeNode alloc] initWithRepresentedObject:nil]);
    AttachTreeNodeChildren(root, dict);
    return root.autorelease();
  }

 private:
  void AttachTreeNodeChildren(NSTreeNode* parent, NSDictionary* data) {
    NSMutableArray* children = [parent mutableChildNodes];
    for (NSString* key in data) {
      NSTreeNode* node = [NSTreeNode treeNodeWithRepresentedObject:key];
      [children addObject:node];

      for (NSDictionary* data_child in [data objectForKey:key])
        AttachTreeNodeChildren(node, data_child);
    }
  }
};

TEST_F(CollectedCookiesMacTest, NormalizeSelection) {
  NSTreeNode* root = CreateTreeNodesFromDictionary(@{
    @"one" : @[ @{ @"one.1" : @[ @{ @"one.1.a" : @[], @"one.1.b" : @[] } ] } ],
    @"two" : @[],
    @"three" : @[ @{ @"three.1" : @[] } ]
  });
  NSArray* nodes = [root childNodes];
  NSTreeNode* one = [nodes objectAtIndex:0];
  NSTreeNode* one1 = [[one childNodes] objectAtIndex:0];
  NSTreeNode* one1a = [[one1 childNodes] objectAtIndex:0];
  NSTreeNode* one1b = [[one1 childNodes] objectAtIndex:1];
  NSTreeNode* two = [nodes objectAtIndex:1];
  NSTreeNode* three = [nodes objectAtIndex:2];
  NSTreeNode* three1 = [[three childNodes] objectAtIndex:0];

  NSArray* selection = @[ one, one1a, three ];
  NSArray* normalized = @[ one, three ];
  NSArray* actual =
      [CollectedCookiesWindowController normalizeNodeSelection:selection];
  EXPECT_NSEQ(normalized, actual);

  selection = @[ two, one1b, three1, one, one1a ];
  normalized = @[ two, three1, one ];
  actual =
      [CollectedCookiesWindowController normalizeNodeSelection:selection];
  EXPECT_NSEQ(normalized, actual);

  selection = @[ one, one1, one1a, one1b ];
  normalized = @[ one ];
  actual =
      [CollectedCookiesWindowController normalizeNodeSelection:selection];
  EXPECT_NSEQ(normalized, actual);
}
