// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

class IndentedTextFactory {
  /**
   * Create an intented text factory, optionally with initial indentation level.
   * @param {Number} indent The initial number of indents (must be natural)
   * @param {String} spaces (Optional) Spacing to use for an indent
   */
  constructor(indent, spaces) {
    this._indent = indent || 0;
    this._spaces = spaces || '  ';
    this._contents = '';
  }

  /**
   * Increase indent level.
   * @param {Number} steps The number of indents to increase by
   */
  increase(steps) { this._indent += (steps || 1); }

  /**
   * Decrease indent level.
   * @param {Number} steps The number of indents to decrease by
   */
  decrease(steps) { this._indent = (this._indent - (steps || 1)) || 0; }

  /**
   * Add a single line of text to the internal content.
   * @param {String} text A single line of text
   */
  addLine(text) {
    let indents = '';

    // Add indentation
    for (let i = 0; i < this._indent; i++) {
      indents += this._spaces;
    }

    this._contents += `${indents}${text}\n`;
  }

  /**
   * Add a multiline string of text to the internal content.
   * Each line will receive the current level of indentation.
   * @param {String} multilineText A multi-line string of text
   */
  addLines(multilineText) {
    const lines = multilineText.split('\n');

    for (let i = 0; i < lines.length; i++) {
      this.addLine(lines[i]);
    }
  }

  toString() { return this._contents; }
}
