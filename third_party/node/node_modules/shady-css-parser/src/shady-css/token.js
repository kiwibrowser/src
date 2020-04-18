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
 * Class that describes individual tokens as produced by the Tokenizer.
 */
class Token {
  /**
   * Create a Token instance.
   * @param {number} type The lexical type of the Token.
   * @param {number} start The start index of the text corresponding to the
   * Token in the CSS text.
   * @param {number} end The end index of the text corresponding to the Token
   * in the CSS text.
   */
  constructor(type, start, end) {
    this.type = type;
    this.start = start;
    this.end = end;
    this.previous = null;
    this.next = null;
  }

  /**
   * Test if the Token matches a given numeric type. Types match if the bitwise
   * AND of the Token's type and the argument type are equivalent to the
   * argument type.
   * @param {number} type The numeric type to test for equivalency with the
   * Token.
   */
  is(type) {
    return (this.type & type) === type;
  }
}

/**
 * An enumeration of Token types.
 * @type {object}
 * @default
 * @static
 */
Token.type = {
  none: 0,
  whitespace: 1,
  string: 2,
  comment: 4,
  word: 8,
  boundary: 16,
  propertyBoundary: 32,
  // Special cases for boundary:
  openParenthesis: 64 | 16,
  closeParenthesis: 128 | 16,
  at: 256 | 16,
  openBrace: 512 | 16,
  // [};] are property boundaries:
  closeBrace: 1024 | 32 | 16,
  semicolon: 2048 | 32 | 16,
  // : is a chimaeric abomination:
  // foo:bar{}
  // foo:bar;
  colon: 4096 | 16 | 8
};

/**
 * A mapping of boundary token text to their corresponding types.
 * @type {object}
 * @default
 * @const
 */
const boundaryTokenTypes = {
  '(': Token.type.openParenthesis,
  ')': Token.type.closeParenthesis,
  ':': Token.type.colon,
  '@': Token.type.at,
  '{': Token.type.openBrace,
  '}': Token.type.closeBrace,
  ';': Token.type.semicolon,
  '-': Token.type.hyphen,
  '_': Token.type.underscore
};

export { Token, boundaryTokenTypes };
