// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class containing predicates for the chrome automation API. Each predicate
 * can be run on one or more AutomationNodes and returns a boolean value.
 *
 * @constructor
 */
function AutomationPredicate() {}

/**
 * Returns true if |node| is a subtreeLeaf, meaning that |node| is either
 * interesting or a group (both defined below).
 *
 * @param {!chrome.automation.AutomationNode} node
 * @param {!chrome.automation.AutomationNode} scope
 * @return {boolean}
 */
AutomationPredicate.isSubtreeLeaf = function(node, scope) {
  return AutomationPredicate.isInteresting(node) ||
      AutomationPredicate.isGroup(node, scope);
};

/**
 * Returns true if |node| is a group, meaning that the node has more than one
 * interesting descendant, and that its interesting descendants exist in more
 * than one subtree of its immediate children.
 *
 * Additionally, for |node| to be a group, it cannot have the same bounding
 * box as its scope.
 *
 * @param {!chrome.automation.AutomationNode} node
 * @param {!chrome.automation.AutomationNode} scope
 * @return {boolean}
 */
AutomationPredicate.isGroup = function(node, scope) {
  if (node !== scope && AutomationPredicate.hasSameLocation_(node, scope))
    return false;

  // Work around for client nested in client. No need to have user select both
  // clients for a window. Once locations for outer client updates correctly,
  // this won't be needed.
  if (node.role === chrome.automation.RoleType.CLIENT &&
      node.role === scope.role && node !== scope)
    return false;

  let interestingBranches = 0;
  let children = node.children || [];
  for (let child of children) {
    if (AutomationPredicate.isInterestingSubtree(child))
      interestingBranches += 1;
    if (interestingBranches > 1)
      return true;
  }
  return false;
};

/**
 * Returns true if the two nodes have the same location.
 *
 * @param {!chrome.automation.AutomationNode} node1
 * @param {!chrome.automation.AutomationNode} node2
 * @return {boolean}
 */
AutomationPredicate.hasSameLocation_ = function(node1, node2) {
  let l1 = node1.location;
  let l2 = node2.location;
  return l1.left === l2.left && l1.top === l2.top && l1.width === l2.width &&
      l1.height === l2.height;
};

/**
 * Returns true if there is an interesting node in the subtree containing
 * |node| as its root (including |node| itself).
 *
 * @param {!chrome.automation.AutomationNode} node
 * @return {boolean}
 */
AutomationPredicate.isInterestingSubtree = function(node) {
  let children = node.children || [];
  return AutomationPredicate.isInteresting(node) ||
      children.some(AutomationPredicate.isInterestingSubtree);
};

/**
 * Returns true if |node| is interesting, meaning that a user can perform some
 * type of action on it.
 *
 * @param {!chrome.automation.AutomationNode} node
 * @return {boolean}
 */
AutomationPredicate.isInteresting = function(node) {
  let loc = node.location;
  let parent = node.parent;
  let root = node.root;
  let role = node.role;
  let state = node.state;

  // TODO(elichtenberg): Define shorthand for chrome.automation.RoleType and
  // StateType.

  // Skip things that are offscreen
  if (state[chrome.automation.StateType.OFFSCREEN] || loc.top < 0 ||
      loc.left < 0)
    return false;

  // Should just leave these as groups
  if (role === chrome.automation.RoleType.WEB_VIEW ||
      role === chrome.automation.RoleType.ROOT_WEB_AREA)
    return false;

  if (parent) {
    // crbug.com/710559
    // Work around for browser tabs
    if (role === chrome.automation.RoleType.TAB &&
        parent.role === chrome.automation.RoleType.TAB_LIST &&
        root.role === chrome.automation.RoleType.DESKTOP)
      return true;
  }

  // The general rule that applies to everything.
  return state[chrome.automation.StateType.FOCUSABLE] === true;
};
