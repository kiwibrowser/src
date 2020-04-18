/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

const path = Symbol('path');

/**
 * Class that implements a visitor pattern for ASTs produced by the Parser.
 * Extend the NodeVisitor class to implement useful tree traversal operations
 * such as stringification.
 */
class NodeVisitor {
  /**
   * Create a NodeVisitor instance.
   */
  constructor() {
    this[path] = [];
  }

  /**
   * A list of nodes that corresponds to the current path through an AST being
   * visited, leading to where the currently visited node will be found.
   * @type {array}
   */
  get path() {
    return this[path];
  }

  /**
   * Visit a node. The visited node will be added to the `path` before it is
   * visited, and will be removed after it is visited. Nodes are "visited" by
   * calling a method on the NodeVisitor instance that matches the node's type,
   * if one is available on the NodeVisitor instance.
   * @param {object} node The node to be visited.
   * @return The return value of the method visiting the node, if any.
   */
  visit(node) {
    let result;
    if (this[node.type]) {
      this[path].push(node);
      result = this[node.type](node);
      this[path].pop();
    }
    return result;
  }
}

export { NodeVisitor };
