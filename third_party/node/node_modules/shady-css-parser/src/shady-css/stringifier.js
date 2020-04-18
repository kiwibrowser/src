/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

import { nodeType } from './common';
import { NodeVisitor } from './node-visitor';

/**
 * Class that implements basic stringification of an AST produced by the Parser.
 */
class Stringifier extends NodeVisitor {
  /**
   * Stringify an AST such as one produced by a Parser.
   * @param {object} ast A node object representing the root of an AST.
   * @return {string} The stringified CSS corresponding to the AST.
   */
  stringify(ast) {
    return this.visit(ast) || '';
  }

  /**
   * Visit and stringify a Stylesheet node.
   * @param {object} stylesheet A Stylesheet node.
   * @return {string} The stringified CSS of the Stylesheet.
   */
  [nodeType.stylesheet](stylesheet) {
    let rules = '';

    for (let i = 0; i < stylesheet.rules.length; ++i) {
      rules += this.visit(stylesheet.rules[i]);
    }

    return rules;
  }

  /**
   * Visit and stringify an At Rule node.
   * @param {object} atRule An At Rule node.
   * @return {string} The stringified CSS of the At Rule.
   */
  [nodeType.atRule](atRule) {
    return `@${atRule.name}` +
      (atRule.parameters ? ` ${atRule.parameters}` : '') +
      (atRule.rulelist ? `${this.visit(atRule.rulelist)}` : ';');
  }

  /**
   * Visit and stringify a Rulelist node.
   * @param {object} rulelist A Rulelist node.
   * @return {string} The stringified CSS of the Rulelist.
   */
  [nodeType.rulelist](rulelist) {
    let rules = '{';

    for (let i = 0; i < rulelist.rules.length; ++i) {
      rules += this.visit(rulelist.rules[i]);
    }

    return rules + '}';
  }

  /**
   * Visit and stringify a Comment node.
   * @param {object} comment A Comment node.
   * @return {string} The stringified CSS of the Comment.
   */
  [nodeType.comment](comment) {
    return `${comment.value}`;
  }

  /**
   * Visit and stringify a Seletor node.
   * @param {object} ruleset A Ruleset node.
   * @return {string} The stringified CSS of the Ruleset.
   */
  [nodeType.ruleset](ruleset) {
    return `${ruleset.selector}${this.visit(ruleset.rulelist)}`;
  }

  /**
   * Visit and stringify a Declaration node.
   * @param {object} declaration A Declaration node.
   * @return {string} The stringified CSS of the Declaration.
   */
  [nodeType.declaration](declaration) {
    return declaration.value != null ?
      `${declaration.name}:${this.visit(declaration.value)};` :
      `${declaration.name};`;
  }

  /**
   * Visit and stringify an Expression node.
   * @param {object} expression An Expression node.
   * @return {string} The stringified CSS of the Expression.
   */
  [nodeType.expression](expression) {
    return `${expression.text}`;
  }

  /**
   * Visit a discarded node.
   * @param {object} discarded A Discarded node.
   * @return {string} An empty string, since Discarded nodes are discarded.
   */
  [nodeType.discarded](discarded) {
    return '';
  }
}

export { Stringifier };
