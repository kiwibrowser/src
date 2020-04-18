// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to move to the appropriate node in the accessibility tree. Stays in a
 * subtree determined by restrictions passed to it.
 *
 * @constructor
 * @param {!chrome.automation.AutomationNode} start
 * @param {!chrome.automation.AutomationNode} scope
 * @param {!AutomationTreeWalker.Restriction} restrictions
 */
function AutomationTreeWalker(start, scope, restrictions) {
  /**
   * Currently highlighted node.
   *
   * @private {!chrome.automation.AutomationNode}
   */
  this.node_ = start;

  /**
   * The root of the subtree that the user is navigating through.
   *
   * @private {!chrome.automation.AutomationNode}
   */
  this.scope_ = scope;

  /**
   * Function that returns true for a node that is a leaf of the current
   * subtree.
   *
   * @private {!AutomationTreeWalker.Unary}
   */
  this.leafPred_ = restrictions.leaf;

  /**
   * Function that returns true for a node in the current subtree that should
   * be visited.
   *
   * @private {!AutomationTreeWalker.Unary}
   */
  this.visitPred_ = restrictions.visit;
}

/**
 * @typedef {{leaf: AutomationTreeWalker.Unary,
 *            visit: AutomationTreeWalker.Unary}}
 */
AutomationTreeWalker.Restriction;

/**
 * @typedef {function(!chrome.automation.AutomationNode) : boolean}
 */
AutomationTreeWalker.Unary;

AutomationTreeWalker.prototype = {
  /**
   * Set this.node_ to the next/previous interesting node within the current
   * scope and return it. If no interesting node is found, return the
   * first/last interesting node. If |doNext| is true, will search for next
   * node. Otherwise, will search for previous node.
   *
   * @param {boolean} doNext
   * @return {chrome.automation.AutomationNode}
   */
  moveToNode: function(doNext) {
    let node = this.node_;
    do {
      node = doNext ? this.getNextNode_(node) : this.getPreviousNode_(node);
    } while (node && !this.visitPred_(node));
    if (node) {
      this.node_ = node;
      return node;
    }

    console.log('Restarting search for node at ' + (doNext ? 'first' : 'last'));
    node = doNext ? this.scope_ : this.getYoungestDescendant_(this.scope_);
    while (node && !this.visitPred_(node))
      node = doNext ? this.getNextNode_(node) : this.getPreviousNode_(node);
    if (node) {
      this.node_ = node;
      return node;
    }

    console.log('Found no interesting nodes to visit.');
    return null;
  },


  /**
   * Given a flat list of nodes in pre-order, get the node that comes after
   * |node| within the current scope.
   *
   * @param {!chrome.automation.AutomationNode} node
   * @return {!chrome.automation.AutomationNode|undefined}
   * @private
   */
  getNextNode_: function(node) {
    // Check for child.
    let child = node.firstChild;
    if (child && !this.leafPred_(node))
      return child;

    // Has no children, and if node is root of subtree, don't check siblings
    // or parent.
    if (node === this.scope_)
      return undefined;

    // No child. Check for right-sibling.
    let sibling = node.nextSibling;
    if (sibling)
      return sibling;

    // No right-sibling. Get right-sibling of closest ancestor.
    let ancestor = node.parent;
    while (ancestor && ancestor !== this.scope_) {
      let aunt = ancestor.nextSibling;
      if (aunt)
        return aunt;
      ancestor = ancestor.parent;
    }

    // No node found after |node|, so return undefined.
    return undefined;
  },

  /**
   * Given a flat list of nodes in pre-order, get the node that comes before
   * |node| within the current scope.
   *
   * @param {!chrome.automation.AutomationNode} node
   * @return {!chrome.automation.AutomationNode|undefined}
   * @private
   */
  getPreviousNode_: function(node) {
    // If node is root of subtree, there is no previous node.
    if (node === this.scope_)
      return undefined;

    // Check for left-sibling. If a left-sibling exists, return its youngest
    // descendant if it has one, or otherwise return the sibling.
    let sibling = node.previousSibling;
    if (sibling)
      return this.getYoungestDescendant_(sibling) || sibling;

    // No left-sibling. Return parent if it exists; otherwise return undefined.
    let parent = node.parent;
    if (parent)
      return parent;

    return undefined;
  },

  /**
   * Get the youngest descendant of |node|, if it has one, within the current
   * scope.
   *
   * @param {!chrome.automation.AutomationNode} node
   * @return {!chrome.automation.AutomationNode|undefined}
   * @private
   */
  getYoungestDescendant_: function(node) {
    if (!node.lastChild || this.leafPred_(node))
      return undefined;

    while (node.lastChild && !this.leafPred_(node))
      node = node.lastChild;

    return node;
  },

  /**
   * Return the next sibling of |node| if it has one.
   *
   * @param {chrome.automation.AutomationNode} node
   * @return {chrome.automation.AutomationNode}
   */
  debugMoveToNext: function(node) {
    if (!node)
      return null;

    let next = node.nextSibling;
    if (next) {
      return next;
    } else {
      console.log('Node is last of siblings');
      console.log('\n');
      return null;
    }
  },

  /**
   * Return the previous sibling of |node| if it has one.
   *
   * @param {chrome.automation.AutomationNode} node
   * @return {chrome.automation.AutomationNode}
   */
  debugMoveToPrevious: function(node) {
    if (!node)
      return null;

    let prev = node.previousSibling;
    if (prev) {
      return prev;
    } else {
      console.log('Node is first of siblings');
      console.log('\n');
      return null;
    }
  },

  /**
   * Return the first child of |node| if it has one.
   *
   * @param {chrome.automation.AutomationNode} node
   * @return {chrome.automation.AutomationNode}
   */
  debugMoveToFirstChild: function(node) {
    if (!node)
      return null;

    let child = node.firstChild;
    if (child) {
      return child;
    } else {
      console.log('Node has no children');
      console.log('\n');
      return null;
    }
  },

  /**
   * Return the parent of |node| if it has one.
   *
   * @param {chrome.automation.AutomationNode} node
   * @return {chrome.automation.AutomationNode}
   */
  debugMoveToParent: function(node) {
    if (!node)
      return null;

    let parent = node.parent;
    if (parent) {
      return parent;
    } else {
      console.log('Node has no parent');
      console.log('\n');
      return null;
    }
  }
};
