// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utility functions for automation nodes in Select-to-Speak.

/**
 * Node state. Nodes can be on-screen like normal, or they may
 * be invisible if they are in a tab that is not in the foreground
 * or similar, or they may be invalid if they were removed from their
 * root, i.e. if they were in a window that was closed.
 * @enum {number}
 */
const NodeState = {
  NODE_STATE_INVALID: 0,
  NODE_STATE_INVISIBLE: 1,
  NODE_STATE_NORMAL: 2,
};

/**
 * Gets the current visiblity state for a given node.
 *
 * @param {AutomationNode} node The starting node.
 * @return {NodeState} the current node state.
 */
function getNodeState(node) {
  if (node === undefined || node.root === null || node.root === undefined) {
    // The node has been removed from the tree, perhaps because the
    // window was closed.
    return NodeState.NODE_STATE_INVALID;
  }
  // This might not be populated correctly on children nodes even if their
  // parents or roots are now invisible.
  // TODO: Update the C++ bindings to set 'invisible' automatically based
  // on parents, rather than going through parents in JS below.
  if (node.state.invisible) {
    return NodeState.NODE_STATE_INVISIBLE;
  }
  // Walk up the tree to make sure the window it is in is not invisible.
  var window = getNearestContainingWindow(node);
  if (window != null && window.state[chrome.automation.StateType.INVISIBLE]) {
    return NodeState.NODE_STATE_INVISIBLE;
  }
  // TODO: Also need a check for whether the window is minimized,
  // which would also return NodeState.NODE_STATE_INVISIBLE.
  return NodeState.NODE_STATE_NORMAL;
}

/**
 * Returns true if a node should be ignored by Select-to-Speak, or if it
 * is of interest. This does not deal with whether nodes have children --
 * nodes are interesting if they have a name or a value and have an onscreen
 * location.
 * @param {!AutomationNode} node The node to test
 * @param {boolean} includeOffscreen Whether to include offscreen nodes.
 * @return {boolean} whether this node should be ignored.
 */
function shouldIgnoreNode(node, includeOffscreen) {
  if (isNodeInvisible(node, includeOffscreen)) {
    return true;
  }
  if (!node.name || isWhitespace(node.name)) {
    if (isTextField(node)) {
      // Text fields may also be marked by value.
      return !node.value || isWhitespace(node.value);
    }
    return true;
  }
  // The node should not be ignored.
  return false;
}

/**
 * Returns true if a node is invisible for any reason.
 * @param {!AutomationNode} node The node to test
 * @param {boolean} includeOffscreen Whether to include offscreen nodes
 *     as visible type nodes.
 * @return {boolean} whether this node is invisible.
 */
function isNodeInvisible(node, includeOffscreen) {
  return !node.location || node.state.invisible ||
      (node.state.offscreen && !includeOffscreen);
}

/**
 * Gets the first window containing this node.
 */
function getNearestContainingWindow(node) {
  // Go upwards to root nodes' parents until we find the first window.
  if (node.root.role == RoleType.ROOT_WEB_AREA) {
    var nextRootParent = node;
    while (nextRootParent != null && nextRootParent.role != RoleType.WINDOW &&
           nextRootParent.root != null &&
           nextRootParent.root.role == RoleType.ROOT_WEB_AREA) {
      nextRootParent = nextRootParent.root.parent;
    }
    return nextRootParent;
  }
  // If the parent isn't a root web area, just walk up the tree to find the
  // nearest window.
  var parent = node;
  while (parent != null && parent.role != chrome.automation.RoleType.WINDOW) {
    parent = parent.parent;
  }
  return parent;
}

/**
 * Gets the length of a node's name. Returns 0 if the name is
 * undefined.
 * @param {AutomationNode} node The node for which to check the name.
 * @return {number} The length of the node's name
 */
function nameLength(node) {
  return node.name ? node.name.length : 0;
}

/**
 * Returns true if a node is a text field type, but not for any other type,
 * including contentEditables.
 * @param {!AutomationNode} node The node to check
 * @return {boolean} True if the node is a text field type.
 */
function isTextField(node) {
  return node.role == RoleType.TEXT_FIELD ||
      node.role == RoleType.TEXT_FIELD_WITH_COMBO_BOX;
}

/**
 * Gets the first (left-most) leaf node of a node. Returns undefined if
 *  none is found.
 * @param {AutomationNode} node The node to search for the first leaf.
 * @return {AutomationNode|undefined} The leaf node.
 */
function getFirstLeafChild(node) {
  let result = node.firstChild;
  while (result && result.firstChild) {
    result = result.firstChild;
  }
  return result;
}

/**
 * Gets the first (left-most) leaf node of a node. Returns undefined
 * if none is found.
 * @param {AutomationNode} node The node to search for the first leaf.
 * @return {AutomationNode|undefined} The leaf node.
 */
function getLastLeafChild(node) {
  let result = node.lastChild;
  while (result && result.lastChild) {
    result = result.lastChild;
  }
  return result;
}

/**
 * Finds all nodes within the subtree rooted at |node| that overlap
 * a given rectangle.
 * @param {!AutomationNode} node The starting node.
 * @param {{left: number, top: number, width: number, height: number}} rect
 *     The bounding box to search.
 * @param {Array<AutomationNode>} nodes The matching node array to be
 *     populated.
 * @return {boolean} True if any matches are found.
 */
function findAllMatching(node, rect, nodes) {
  var found = false;
  for (var c = node.firstChild; c; c = c.nextSibling) {
    if (findAllMatching(c, rect, nodes))
      found = true;
  }

  if (found)
    return true;

  // Closure needs node.location check here to allow the next few
  // lines to compile.
  if (shouldIgnoreNode(node, /* don't include offscreen */ false) ||
      node.location === undefined)
    return false;

  if (overlaps(node.location, rect)) {
    if (!node.children || node.children.length == 0 ||
        node.children[0].role != RoleType.INLINE_TEXT_BOX) {
      // Only add a node if it has no inlineTextBox children. If
      // it has text children, they will be more precisely bounded
      // and specific, so no need to add the parent node.
      nodes.push(node);
      return true;
    }
  }

  return false;
}

/**
 * Class representing a position on the accessibility, made of a
 * selected node and the offset of that selection.
 * @typedef {{node: (!AutomationNode),
 *            offset: (number)}}
 */
var Position;

/**
 * Finds the deep equivalent node where a selection starts given a node
 * object and selection offset. This is meant to be used in conjunction with
 * the anchorObject/anchorOffset and focusObject/focusOffset of the
 * automation API.
 * @param {AutomationNode} parent The parent node of the selection,
 * similar to chrome.automation.focusObject.
 * @param {number} offset The integer offset of the selection. This is
 * similar to chrome.automation.focusOffset.
 * @param {boolean} isStart whether this is the start or end of a selection.
 * @return {!Position} The node matching the selected offset.
 */
function getDeepEquivalentForSelection(parent, offset, isStart) {
  if (parent.children.length == 0)
    return {node: parent, offset: offset};
  // Create a stack of children nodes to search through.
  let nodesToCheck;
  if (isTextField(parent) && parent.firstChild &&
      parent.firstChild.firstChild) {
    // Skip ahead.
    nodesToCheck = parent.firstChild.children.slice().reverse();
  } else {
    nodesToCheck = parent.children.slice().reverse();
  }
  let index = 0;
  var node = parent;
  // Delve down into the children recursively to find the
  // one at this offset.
  while (nodesToCheck.length > 0) {
    node = nodesToCheck.pop();
    if (node.state.invisible)
      continue;
    if (node.children.length > 0) {
      // If the parent is a textField, then the whole text
      // field is selected. Ignore its contents.
      // If only part of the text field was selected, the parent type would
      // have been a text field.
      if (!isTextField(node)) {
        nodesToCheck = nodesToCheck.concat(node.children.slice().reverse());
      }
      if (node.role != RoleType.LINE_BREAK &&
          (node.parent && node.parent.parent &&
           !isTextField(node.parent.parent))) {
        // If this is inside a textField, or if it is a line break, don't
        // count the node itself. Otherwise it counts.
        index += 1;
      }
    } else {
      if (node.role == RoleType.STATIC_TEXT ||
          node.role == RoleType.INLINE_TEXT_BOX) {
        // How many characters are in the name.
        index += nameLength(node);
      } else {
        // Add one for itself only.
        index += 1;
      }
    }
    // Check if we've indexed far enough into the nodes of this parent to be
    // past the offset if |isStart|, or at the offset if !|isStart|.
    if (((isStart && index > offset) || (!isStart && index >= offset))) {
      // If the node is a text field type with children, return its first
      // (or last if !|isStart|) leaf child. Otherwise, just return the node
      // and its offset. textField nodes are indexed differently in selection
      // from others -- it seems as though the whole node counts only once in
      // the selection index if the textField is entirely selected, whereas a
      // normal staticText will count for itself plus one. This is probably
      // because textFields cannot be partially selected if other elements
      // outside of themselves are selected.
      if (isTextField(node)) {
        let leafNode =
            isStart ? getFirstLeafChild(node) : getLastLeafChild(node);
        if (leafNode) {
          return {node: leafNode, offset: isStart ? 0 : nameLength(leafNode)};
        }
      }
      let result = offset - index + nameLength(node);
      return {node: node, offset: result > 0 ? result : 0};
    }
  }
  // We are at the end of the last node.
  // If it's a textField we skipped, go ahead and find the first (or last, if
  // !|isStart|) child, otherwise just return this node itself.
  if (isTextField(node)) {
    let leafNode = isStart ? getFirstLeafChild(node) : getLastLeafChild(node);
    if (leafNode) {
      return {node: leafNode, offset: isStart ? 0 : nameLength(leafNode)};
    }
  }
  return {node: node, offset: nameLength(node)};
}
