"use strict";
/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at
 * http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at
 * http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at
 * http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at
 * http://polymer.github.io/PATENTS.txt
 */
Object.defineProperty(exports, "__esModule", { value: true });
const source_range_1 = require("../model/source-range");
/**
 * A parsed Document.
 *
 * @template AstNode The AST type of the document.
 * @template Visitor The type of the visitors that can walk the document.
 */
class ParsedDocument {
    constructor(from) {
        /**
         * The 0-based offsets into `contents` of all newline characters.
         *
         * Useful for converting between string offsets and SourcePositions.
         */
        this.newlineIndexes = [];
        this.url = from.url;
        this.baseUrl = from.baseUrl || this.url;
        this.contents = from.contents;
        this.ast = from.ast;
        this._locationOffset = from.locationOffset;
        this.astNode = from.astNode;
        this.isInline = from.isInline;
        let lastSeenLine = -1;
        while (true) {
            lastSeenLine = from.contents.indexOf('\n', lastSeenLine + 1);
            if (lastSeenLine === -1) {
                break;
            }
            this.newlineIndexes.push(lastSeenLine);
        }
        this.sourceRange = this.offsetsToSourceRange(0, this.contents.length);
    }
    sourceRangeForNode(node) {
        const baseSource = this._sourceRangeForNode(node);
        return this.relativeToAbsoluteSourceRange(baseSource);
    }
    ;
    offsetToSourcePosition(offset) {
        const linesLess = binarySearch(offset, this.newlineIndexes);
        let colOffset = this.newlineIndexes[linesLess - 1];
        if (colOffset == null) {
            colOffset = 0;
        }
        else {
            colOffset = colOffset + 1;
        }
        return { line: linesLess, column: offset - colOffset };
    }
    offsetsToSourceRange(start, end) {
        const sourceRange = {
            file: this.url,
            start: this.offsetToSourcePosition(start),
            end: this.offsetToSourcePosition(end)
        };
        return source_range_1.correctSourceRange(sourceRange, this._locationOffset);
    }
    sourcePositionToOffset(position) {
        const line = Math.max(0, position.line);
        let lineOffset;
        if (line === 0) {
            lineOffset = -1;
        }
        else if (line > this.newlineIndexes.length) {
            lineOffset = this.contents.length - 1;
        }
        else {
            lineOffset = this.newlineIndexes[line - 1];
        }
        const result = position.column + lineOffset + 1;
        // Clamp within bounds.
        return Math.min(Math.max(0, result), this.contents.length);
    }
    relativeToAbsoluteSourceRange(sourceRange) {
        return source_range_1.correctSourceRange(sourceRange, this._locationOffset);
    }
    absoluteToRelativeSourceRange(sourceRange) {
        return source_range_1.uncorrectSourceRange(sourceRange, this._locationOffset);
    }
    sourceRangeToOffsets(range) {
        return [
            this.sourcePositionToOffset(range.start),
            this.sourcePositionToOffset(range.end)
        ];
    }
    toString() {
        if (this.isInline) {
            return `Inline ${this.constructor.name} on line ` +
                `${this.sourceRange.start.line} of ${this.url}`;
        }
        return `${this.constructor.name} at ${this.url}`;
    }
}
exports.ParsedDocument = ParsedDocument;
/**
 * The variant of binary search that returns the number of elements in the
 * array that is strictly less than the target.
 */
function binarySearch(target, arr) {
    let lower = 0;
    let upper = arr.length - 1;
    while (true) {
        if (lower > upper) {
            return lower;
        }
        const m = Math.floor((upper + lower) / 2);
        if (target === arr[m]) {
            return m;
        }
        if (target > arr[m]) {
            lower = m + 1;
        }
        else {
            upper = m - 1;
        }
    }
}

//# sourceMappingURL=document.js.map
