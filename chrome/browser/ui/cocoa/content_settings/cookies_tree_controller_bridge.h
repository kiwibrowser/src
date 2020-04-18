// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIES_TREE_CONTROLLER_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIES_TREE_CONTROLLER_BRIDGE_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#import "chrome/browser/ui/cocoa/content_settings/cookie_tree_node.h"

class CookiesTreeControllerBridge : public ui::TreeModelObserver {
 public:
  explicit CookiesTreeControllerBridge(CookiesTreeModel* model);
  ~CookiesTreeControllerBridge() override;

  // TreeModelObserver:
  void TreeNodesAdded(ui::TreeModel* model,
                      ui::TreeModelNode* parent,
                      int start,
                      int count) override;
  void TreeNodesRemoved(ui::TreeModel* model,
                        ui::TreeModelNode* parent,
                        int start,
                        int count) override;
  void TreeNodeChanged(ui::TreeModel* model, ui::TreeModelNode* node) override;

  CocoaCookieTreeNode* cocoa_model() const { return cocoa_model_.get(); }

 private:
  // Creates a CocoaCookieTreeNode from a platform-independent one.
  // Return value is autoreleased. This creates child nodes recusively.
  CocoaCookieTreeNode* CocoaNodeFromTreeNode(ui::TreeModelNode* node);

  // Finds the Cocoa model node based on a platform-independent one. This is
  // done by comparing the treeNode pointers. |start| is the node to start
  // searching at. If |start| is nil, the root is used.
  CocoaCookieTreeNode* FindCocoaNode(ui::TreeModelNode* node,
                                     CocoaCookieTreeNode* start);

  // The C++ model that this observes.
  CookiesTreeModel* model_;  // weak

  // A copy of the model using Cocoa objects instead of C++ ones.
  base::scoped_nsobject<CocoaCookieTreeNode> cocoa_model_;
};

#endif  // CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIES_TREE_CONTROLLER_BRIDGE_H_
