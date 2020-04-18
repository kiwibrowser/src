/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

/**
 * A set of common RegExp matchers for tokenizing CSS.
 * @constant
 * @type {object}
 * @default
 */
const matcher = {
  whitespace: /\s/,
  whitespaceGreedy: /(\s+)/g,
  commentGreedy: /(\*\/)/g,
  boundary: /[\(\)\{\}'"@;:\s]/,
  stringBoundary: /['"]/
};

/**
 * An enumeration of Node types.
 * @constant
 * @type {object}
 * @default
 */
const nodeType = {
  stylesheet: 'stylesheet',
  comment: 'comment',
  atRule: 'atRule',
  ruleset: 'ruleset',
  expression: 'expression',
  declaration: 'declaration',
  rulelist: 'rulelist',
  discarded: 'discarded'
};

export { matcher, nodeType };
