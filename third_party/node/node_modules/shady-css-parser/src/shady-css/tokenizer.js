/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */

import { matcher } from './common';
import { Token, boundaryTokenTypes } from './token';

const currentToken = Symbol('currentToken');
const cursorToken = Symbol('cursorToken');
const getNextToken = Symbol('getNextToken');

/**
 * Class that implements tokenization of significant lexical features of the
 * CSS syntax.
 */
class Tokenizer {
  /**
   * Create a Tokenizer instance.
   * @param {string} cssText The raw CSS string to be tokenized.
   *
   */
  constructor(cssText) {
    this.cssText = cssText;
    /**
     * Tracks the position of the tokenizer in the source string.
     * Also the default head of the Token linked list.
     * @type {!Token}
     * @private
     */
    this[cursorToken] = new Token(Token.type.none, 0, 0);
    /**
     * Holds a reference to a Token that is "next" in the source string, often
     * due to having been peeked at.
     * @type {?Token}
     * @readonly
     */
    this[currentToken] = null;
  }

  get offset() {
    return this[cursorToken].end;
  }

  /**
   * The current token that will be returned by a call to `advance`. This
   * reference is useful for "peeking" at the next token ahead in the sequence.
   * If the entire CSS text has been tokenized, the `currentToken` will be null.
   * @type {Token}
   */
  get currentToken() {
    if (this[currentToken] == null) {
      this[currentToken] = this[getNextToken]();
    }

    return this[currentToken];
  }

  /**
   * Advance the Tokenizer to the next token in the sequence.
   * @return {Token} The current token prior to the call to `advance`, or null
   * if the entire CSS text has been tokenized.
   */
  advance() {
    let token;
    if (this[currentToken] != null) {
      token = this[currentToken];
      this[currentToken] = null;
    } else {
      token = this[getNextToken]();
    }
    return token;
  }

  /**
   * Extract a slice from the CSS text, using two tokens to represent the range
   * of text to be extracted. The extracted text will include all text between
   * the start index of the first token and the offset index of the second token
   * (or the offset index of the first token if the second is not provided).
   * @param {Token} startToken The token that represents the beginning of the
   * text range to be extracted.
   * @param {Token} endToken The token that represents the end of the text range
   * to be extracted. Defaults to the startToken if no endToken is provided.
   * @return {string} The substring of the CSS text corresponding to the
   * startToken and endToken.
   */
  slice(startToken, endToken) {
    endToken = endToken || startToken;
    return this.cssText.substring(startToken.start, endToken.end);
  }

  /**
   * Flush all tokens from the Tokenizer.
   * @return {array} An array of all tokens corresponding to the CSS text.
   */
  flush() {
    let tokens = [];
    while (this.currentToken) {
      tokens.push(this.advance());
    }
    return tokens;
  }

  /**
   * Extract the next token from the CSS text and advance the Tokenizer.
   * @return {Token} A Token instance, or null if the entire CSS text has beeen
   * tokenized.
   */
  [getNextToken]() {
    let character = this.cssText[this.offset];
    let token;

    this[currentToken] = null;

    if (this.offset >= this.cssText.length) {
      return null;
    } else if (matcher.whitespace.test(character)) {
      token = this.tokenizeWhitespace(this.offset);
    } else if (matcher.stringBoundary.test(character)) {
      token = this.tokenizeString(this.offset);
    } else if (character === '/' && this.cssText[this.offset + 1] === '*') {
      token = this.tokenizeComment(this.offset);
    } else if (matcher.boundary.test(character)) {
      token = this.tokenizeBoundary(this.offset);
    } else {
      token = this.tokenizeWord(this.offset);
    }

    token.previous = this[cursorToken];
    this[cursorToken].next = token;
    this[cursorToken] = token;

    return token;
  }

  /**
   * Tokenize a string starting at a given offset in the CSS text. A string is
   * any span of text that is wrapped by eclusively paired, non-escaped matching
   * quotation marks.
   * @param {number} offset An offset in the CSS text.
   * @return {Token} A string Token instance.
   */
  tokenizeString(offset) {
    let quotation = this.cssText[offset];
    let escaped = false;
    let start = offset;
    let character;

    while (character = this.cssText[++offset]) {
      if (escaped) {
        escaped = false;
        continue;
      }

      if (character === quotation) {
        ++offset;
        break;
      }

      if (character === '\\') {
        escaped = true;
      }
    }

    return new Token(Token.type.string, start, offset);
  }

  /**
   * Tokenize a word starting at a given offset in the CSS text. A word is any
   * span of text that is not whitespace, is not a string, is not a comment and
   * is not a structural delimiter (such as braces and semicolon).
   * @param {offset} number An offset in the CSS text.
   * @return {Token} A word Token instance.
   */
  tokenizeWord(offset) {
    let start = offset;
    let character;
    // TODO(cdata): change to greedy regex match?
    while ((character = this.cssText[offset]) &&
           !matcher.boundary.test(character)) {
      offset++;
    }

    return new Token(Token.type.word, start, offset);
  }

  /**
   * Tokenize whitespace starting at a given offset in the CSS text. Whitespace
   * is any span of text made up of consecutive spaces, tabs, newlines and other
   * single whitespace characters.
   * @param {offset} number An offset in the CSS text.
   * @return {Token} A whitespace Token instance.
   */
  tokenizeWhitespace(offset) {
    let start = offset;

    matcher.whitespaceGreedy.lastIndex = offset;
    let match = matcher.whitespaceGreedy.exec(this.cssText);

    if (match != null && match.index === offset) {
      offset = matcher.whitespaceGreedy.lastIndex;
    }

    return new Token(Token.type.whitespace, start, offset);
  }

  /**
   * Tokenize a comment starting at a given offset in the CSS text. A comment is
   * any span of text beginning with the two characters / and *, and ending with
   * a matching counterpart pair of consecurtive characters (* and /).
   * @param {offset} number An offset in the CSS text.
   * @return {Token} A comment Token instance.
   */
  tokenizeComment(offset) {
    let start = offset;

    matcher.commentGreedy.lastIndex = offset;
    let match = matcher.commentGreedy.exec(this.cssText);

    if (match == null) {
      offset = this.cssText.length;
    } else {
      offset = matcher.commentGreedy.lastIndex;
    }

    return new Token(Token.type.comment, start, offset);
  }

  /**
   * Tokenize a boundary at a given offset in the CSS text. A boundary is any
   * single structurally significant character. These characters include braces,
   * semicolons, the "at" symbol and others.
   * @param {offset} number An offset in the CSS text.
   * @return {Token} A boundary Token instance.
   */
  tokenizeBoundary(offset) {
    // TODO(cdata): Evaluate if this is faster than a switch statement:
    let type = boundaryTokenTypes[this.cssText[offset]] || Token.type.boundary;

    return new Token(type, offset, offset + 1);
  }
}

export { Tokenizer };
