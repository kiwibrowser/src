// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_TREE_NODE_H_
#define CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_TREE_NODE_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/browser/ui/cocoa/content_settings/cookie_details.h"

@interface CocoaCookieTreeNode : NSObject {
  base::scoped_nsobject<NSString> title_;
  base::scoped_nsobject<NSMutableArray> children_;
  base::scoped_nsobject<CocoaCookieDetails> details_;
  CookieTreeNode* treeNode_;  // weak
}

// Designated initializer.
- (id)initWithNode:(CookieTreeNode*)node;

// Re-sets all the members of the node based on |treeNode_|.
- (void)rebuild;

// Common getters..
- (NSString*)title;
- (CocoaCookieDetailsType)nodeType;
- (ui::TreeModelNode*)treeNode;

// |-mutableChildren| exists so that the CookiesTreeModelObserverBridge can
// operate on the children. Note that this lazily creates children.
- (NSMutableArray*)mutableChildren;
- (NSArray*)children;
- (BOOL)isLeaf;

- (CocoaCookieDetails*)details;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_TREE_NODE_H_
