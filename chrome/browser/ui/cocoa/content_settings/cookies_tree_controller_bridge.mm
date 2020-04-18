// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/cookies_tree_controller_bridge.h"
#include "base/containers/queue.h"

CookiesTreeControllerBridge::CookiesTreeControllerBridge(
    CookiesTreeModel* model)
    : model_(model),
      cocoa_model_([CocoaNodeFromTreeNode(model_->GetRoot()) retain]) {
  model_->AddObserver(this);
}

CookiesTreeControllerBridge::~CookiesTreeControllerBridge() {
  model_->RemoveObserver(this);
}

// Notification that nodes were added to the specified parent.
void CookiesTreeControllerBridge::TreeNodesAdded(ui::TreeModel* model,
                                                 ui::TreeModelNode* parent,
                                                 int start,
                                                 int count) {
  CocoaCookieTreeNode* cocoa_parent = FindCocoaNode(parent, nil);
  NSMutableArray* cocoa_children = [cocoa_parent mutableChildren];

  [cocoa_model_ willChangeValueForKey:@"children"];
  CookieTreeNode* cookie_parent = static_cast<CookieTreeNode*>(parent);
  for (int i = 0; i < count; ++i) {
    CookieTreeNode* cookie_child = cookie_parent->GetChild(start + i);
    CocoaCookieTreeNode* new_child = CocoaNodeFromTreeNode(cookie_child);
    [cocoa_children addObject:new_child];
  }
  [cocoa_model_ didChangeValueForKey:@"children"];
}

// Notification that nodes were removed from the specified parent.
void CookiesTreeControllerBridge::TreeNodesRemoved(ui::TreeModel* model,
                                                   ui::TreeModelNode* parent,
                                                   int start,
                                                   int count) {
  CocoaCookieTreeNode* cocoa_parent = FindCocoaNode(parent, nil);
  NSMutableArray* cocoa_children = [cocoa_parent mutableChildren];
  [cocoa_model_ willChangeValueForKey:@"children"];
  for (int i = start + count - 1; i >= start; --i) {
    [cocoa_children removeObjectAtIndex:i];
  }
  [cocoa_model_ didChangeValueForKey:@"children"];
}

// Notification that the contents of a node has changed.
void CookiesTreeControllerBridge::TreeNodeChanged(ui::TreeModel* model,
                                                  ui::TreeModelNode* node) {
  [cocoa_model_ willChangeValueForKey:@"children"];
  CocoaCookieTreeNode* changed_node = FindCocoaNode(node, nil);
  [changed_node rebuild];
  [cocoa_model_ didChangeValueForKey:@"children"];
}

CocoaCookieTreeNode* CookiesTreeControllerBridge::CocoaNodeFromTreeNode(
    ui::TreeModelNode* node) {
  CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
  return [[[CocoaCookieTreeNode alloc] initWithNode:cookie_node] autorelease];
}

// Does breadth-first search on the tree to find |node|. This method is most
// commonly used to find origin/folder nodes, which are at the first level off
// the root (hence breadth-first search).
CocoaCookieTreeNode* CookiesTreeControllerBridge::FindCocoaNode(
    ui::TreeModelNode* target, CocoaCookieTreeNode* start) {
  if (!start) {
    start = cocoa_model_.get();
  }
  if ([start treeNode] == target) {
    return start;
  }

  // Enqueue the root node of the search (sub-)tree.
  base::queue<CocoaCookieTreeNode*> horizon;
  horizon.push(start);

  // Loop until we've looked at every node or we found the target.
  while (!horizon.empty()) {
    // Dequeue the item at the front.
    CocoaCookieTreeNode* node = horizon.front();
    horizon.pop();

    // If this is the target node, report it up.
    if ([node treeNode] == target)
      return node;

    // Add all child nodes to the queue for searching.
    if (![node isLeaf]) {
      NSArray* children = [node children];
      for (CocoaCookieTreeNode* child in children) {
        horizon.push(child);
      }
    }
  }

  return nil;  // The node could not be found.
}
