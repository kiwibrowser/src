// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Class to manage interactions with the accessibility tree, including moving
 * to and selecting nodes.
 *
 * @constructor
 * @param {!chrome.automation.AutomationNode} desktop
 */
function AutomationManager(desktop) {
  /**
   * Currently highlighted node.
   *
   * @private {!chrome.automation.AutomationNode}
   */
  this.node_ = desktop;

  /**
   * The root of the subtree that the user is navigating through.
   *
   * @private {!chrome.automation.AutomationNode}
   */
  this.scope_ = desktop;

  /**
   * The desktop node.
   *
   * @private {!chrome.automation.AutomationNode}
   */
  this.desktop_ = desktop;

  /**
   * A stack of past scopes. Allows user to traverse back to previous groups
   * after selecting one or more groups. The most recent group is at the end
   * of the array.
   *
   * @private {Array<!chrome.automation.AutomationNode>}
   */
  this.scopeStack_ = [];

  /**
   * Moves to the appropriate node in the accessibility tree.
   *
   * @private {!AutomationTreeWalker}
   */
  this.treeWalker_ = this.createTreeWalker_(desktop);

  this.init_();
}

/**
 * Highlight colors for the focus ring to distinguish between different types
 * of nodes.
 *
 * @const
 */
AutomationManager.Color = {
  SCOPE: '#de742f',  // dark orange
  GROUP: '#ffbb33',  // light orange
  LEAF: '#78e428'    // light green
};

AutomationManager.prototype = {
  /**
   * Set this.node_, this.root_, and this.desktop_ to the desktop node, and
   * creates an initial tree walker.
   *
   * @private
   */
  init_: function() {
    console.log('AutomationNode for desktop is loaded');
    this.printNode_(this.node_);

    this.desktop_.addEventListener(
        chrome.automation.EventType.FOCUS, this.handleFocusChange_.bind(this),
        false);

    // TODO(elichtenberg): Eventually use a more specific filter than
    // ALL_TREE_CHANGES.
    chrome.automation.addTreeChangeObserver(
        chrome.automation.TreeChangeObserverFilter.ALL_TREE_CHANGES,
        this.handleNodeRemoved_.bind(this));
  },

  /**
   * When an interesting element gains focus on the page, move to it. If an
   * element gains focus but is not interesting, move to the next interesting
   * node after it.
   *
   * @param {!chrome.automation.AutomationEvent} event
   * @private
   */
  handleFocusChange_: function(event) {
    if (this.node_ === event.target)
      return;
    console.log('Focus changed');

    // Rebuild scope stack and set scope for focused node.
    this.buildScopeStack_(event.target);

    // Move to focused node.
    this.node_ = event.target;
    this.treeWalker_ = this.createTreeWalker_(this.scope_, this.node_);

    // In case the node that gained focus is not a subtreeLeaf.
    if (AutomationPredicate.isSubtreeLeaf(this.node_, this.scope_)) {
      this.printNode_(this.node_);
      this.updateFocusRing_();
    } else
      this.moveToNode(true);
  },

  /**
   * Create a new scope stack and set the current scope for |node|.
   *
   * @param {!chrome.automation.AutomationNode} node
   * @private
   */
  buildScopeStack_: function(node) {
    // Create list of |node|'s ancestors, with highest level ancestor at the
    // end.
    let ancestorList = [];
    while (node.parent) {
      ancestorList.push(node.parent);
      node = node.parent;
    }

    // Starting with desktop as the scope, if an ancestor is a group, set it to
    // the new scope and push the old scope onto the scope stack.
    this.scopeStack_ = [];
    this.scope_ = this.desktop_;
    while (ancestorList.length > 0) {
      let ancestor = ancestorList.pop();
      if (ancestor.role === chrome.automation.RoleType.DESKTOP)
        continue;
      if (AutomationPredicate.isGroup(ancestor, this.scope_)) {
        this.scopeStack_.push(this.scope_);
        this.scope_ = ancestor;
      }
    }
  },

  /**
   * When a node is removed from the page, move to a new valid node.
   *
   * @param {!chrome.automation.TreeChange} treeChange
   * @private
   */
  handleNodeRemoved_: function(treeChange) {
    // TODO(elichtenberg): Only listen to NODE_REMOVED callbacks. Don't need
    // any others.
    if (treeChange.type !== chrome.automation.TreeChangeType.NODE_REMOVED)
      return;

    // TODO(elichtenberg): Currently not getting NODE_REMOVED event when whole
    // tree is deleted. Once fixed, can delete this. Should only need to check
    // if target is current node.
    let removedByRWA =
        treeChange.target.role === chrome.automation.RoleType.ROOT_WEB_AREA &&
        !this.node_.role;

    if (!removedByRWA && treeChange.target !== this.node_)
      return;

    console.log('Node removed');
    chrome.accessibilityPrivate.setFocusRing([]);

    // Current node not invalid until after treeChange callback, so move to
    // valid node after callback. Delay added to prevent moving to another
    // node about to be made invalid. If already at a valid node (e.g., user
    // moves to it or focus changes to it), won't need to move to a new node.
    window.setTimeout(function() {
      if (!this.node_.role)
        this.moveToNode(true);
    }.bind(this), 100);
  },

  /**
   * Set this.node_ to the next/previous interesting node, and then highlight
   * it on the screen. If no interesting node is found, set this.node_ to the
   * first/last interesting node. If |doNext| is true, will search for next
   * node. Otherwise, will search for previous node.
   *
   * @param {boolean} doNext
   */
  moveToNode: function(doNext) {
    // If node is invalid, set node to last valid scope.
    this.startAtValidNode_();

    let node = this.treeWalker_.moveToNode(doNext);
    if (node) {
      this.node_ = node;
      this.printNode_(this.node_);
      this.updateFocusRing_();
    }
  },

  /**
   * Select the currently highlighted node. If the node is the current scope,
   * go back to the previous scope (i.e., create a new tree walker rooted at
   * the previous scope). If the node is a group other than the current scope,
   * create a new tree walker for the new subtree the user is scanning through.
   * Otherwise, meaning the node is interesting, perform the default action on
   * it.
   */
  selectCurrentNode: function() {
    if (!this.node_.role)
      return;

    if (this.node_ === this.scope_) {
      // Don't let user select the top-level root node (i.e., the desktop node).
      if (this.scopeStack_.length === 0)
        return;

      // Find a previous scope that is still valid. The stack here always has
      // at least one valid scope (i.e., the desktop node).
      do {
        this.scope_ = this.scopeStack_.pop();
      } while (!this.scope_.role && this.scopeStack_.length > 0);

      this.treeWalker_ = this.createTreeWalker_(this.scope_, this.node_);
      this.updateFocusRing_();
      console.log('Moved to previous scope');
      this.printNode_(this.node_);
      return;
    }

    if (AutomationPredicate.isGroup(this.node_, this.scope_)) {
      this.scopeStack_.push(this.scope_);
      this.scope_ = this.node_;
      this.treeWalker_ = this.createTreeWalker_(this.scope_);
      console.log('Entered scope');
      this.moveToNode(true);
      return;
    }

    this.node_.doDefault();
    console.log('Performed default action');
    console.log('\n');
  },

  /**
   * Set the focus ring for the current node and determine the color for it.
   *
   * @private
   */
  updateFocusRing_: function() {
    let color;
    if (this.node_ === this.scope_)
      color = AutomationManager.Color.SCOPE;
    else if (AutomationPredicate.isGroup(this.node_, this.scope_))
      color = AutomationManager.Color.GROUP;
    else
      color = AutomationManager.Color.LEAF;
    chrome.accessibilityPrivate.setFocusRing([this.node_.location], color);
  },

  /**
   * If this.node_ is invalid, set this.node_ to a valid scope. Will check the
   * current scope and past scopes until a valid scope is found. this.node_
   * is set to that valid scope.
   *
   * @private
   */
  startAtValidNode_: function() {
    if (this.node_.role)
      return;
    console.log('Finding new valid node');

    // Current node is invalid, but current scope is still valid, so set node
    // to the current scope.
    if (this.scope_.role)
      this.node_ = this.scope_;

    // Current node and current scope are invalid, so set both to a valid scope
    // from the scope stack. The stack here always has at least one valid scope
    // (i.e., the desktop node).
    while (!this.node_.role && this.scopeStack_.length > 0) {
      this.node_ = this.scopeStack_.pop();
      this.scope_ = this.node_;
    }
    this.treeWalker_ = this.createTreeWalker_(this.scope_);
  },

  /**
   * Create an AutomationTreeWalker for the subtree with |scope| as its root.
   * If |opt_start| is defined, the tree walker will start walking the tree
   * from |opt_start|; otherwise, it will start from |scope|.
   *
   * @param {!chrome.automation.AutomationNode} scope
   * @param {!chrome.automation.AutomationNode=} opt_start
   * @private
   * @return {!AutomationTreeWalker}
   */
  createTreeWalker_: function(scope, opt_start) {
    // If no explicit start node, start walking the tree from |scope|.
    let start = opt_start || scope;

    let leafPred = function(node) {
      return (node !== scope &&
              AutomationPredicate.isSubtreeLeaf(node, scope)) ||
          !AutomationPredicate.isInterestingSubtree(node);
    };
    let visitPred = function(node) {
      // Avoid visiting the top-level root node (i.e., the desktop node).
      return node !== this.desktop_ &&
          AutomationPredicate.isSubtreeLeaf(node, scope);
    }.bind(this);

    let restrictions = {leaf: leafPred, visit: visitPred};
    return new AutomationTreeWalker(start, scope, restrictions);
  },

  // TODO(elichtenberg): Move print functions to a custom logger class. Only
  // log when debuggingEnabled is true.
  /**
   * Print out details about a node.
   *
   * @param {chrome.automation.AutomationNode} node
   * @private
   */
  printNode_: function(node) {
    if (node) {
      console.log('Name = ' + node.name);
      console.log('Role = ' + node.role);
      console.log('Root role = ' + node.root.role);
      if (!node.parent)
        console.log('At index ' + node.indexInParent + ', has no parent');
      else {
        let numSiblings = node.parent.children.length;
        console.log(
            'At index ' + node.indexInParent + ', there are ' + numSiblings +
            ' siblings');
      }
      console.log('Has ' + node.children.length + ' children');
    } else {
      console.log('Node is null');
    }
    console.log(node);
    console.log('\n');
  },

  /**
   * Move to the next sibling of this.node_ if it has one.
   */
  debugMoveToNext: function() {
    let next = this.treeWalker_.debugMoveToNext(this.node_);
    if (next) {
      this.node_ = next;
      this.printNode_(this.node_);
      chrome.accessibilityPrivate.setFocusRing([this.node_.location]);
    }
  },

  /**
   * Move to the previous sibling of this.node_ if it has one.
   */
  debugMoveToPrevious: function() {
    let prev = this.treeWalker_.debugMoveToPrevious(this.node_);
    if (prev) {
      this.node_ = prev;
      this.printNode_(this.node_);
      chrome.accessibilityPrivate.setFocusRing([this.node_.location]);
    }
  },

  /**
   * Move to the first child of this.node_ if it has one.
   */
  debugMoveToFirstChild: function() {
    let child = this.treeWalker_.debugMoveToFirstChild(this.node_);
    if (child) {
      this.node_ = child;
      this.printNode_(this.node_);
      chrome.accessibilityPrivate.setFocusRing([this.node_.location]);
    }
  },

  /**
   * Move to the parent of this.node_ if it has one.
   */
  debugMoveToParent: function() {
    let parent = this.treeWalker_.debugMoveToParent(this.node_);
    if (parent) {
      this.node_ = parent;
      this.printNode_(this.node_);
      chrome.accessibilityPrivate.setFocusRing([this.node_.location]);
    }
  }
};
