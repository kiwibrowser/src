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
function correctSourceRange(sourceRange, locationOffset) {
    if (!locationOffset || !sourceRange) {
        return sourceRange;
    }
    return {
        file: locationOffset.filename || sourceRange.file,
        start: correctPosition(sourceRange.start, locationOffset),
        end: correctPosition(sourceRange.end, locationOffset)
    };
}
exports.correctSourceRange = correctSourceRange;
function correctPosition(position, locationOffset) {
    return {
        line: position.line + locationOffset.line,
        column: position.column + (position.line === 0 ? locationOffset.col : 0)
    };
}
exports.correctPosition = correctPosition;
function uncorrectSourceRange(sourceRange, locationOffset) {
    if (locationOffset == null || sourceRange == null) {
        return sourceRange;
    }
    return {
        file: locationOffset.filename || sourceRange.file,
        start: uncorrectPosition(sourceRange.start, locationOffset),
        end: uncorrectPosition(sourceRange.end, locationOffset),
    };
}
exports.uncorrectSourceRange = uncorrectSourceRange;
function uncorrectPosition(position, locationOffset) {
    const line = position.line - locationOffset.line;
    return {
        line: line,
        column: position.column - (line === 0 ? locationOffset.col : 0)
    };
}
exports.uncorrectPosition = uncorrectPosition;
/**
 * If the position is inside the range, returns 0. If it comes before the range,
 * it returns -1. If it comes after the range, it returns 1.
 *
 * TODO(rictic): test this method directly (currently most of its tests are
 *   indirectly, through ast-from-source-position).
 */
function comparePositionAndRange(position, range, includeEdges) {
    // Usually we want to include the edges of a range as part
    // of the thing, but sometimes, e.g. for start and end tags,
    // we'd rather not.
    if (includeEdges == null) {
        includeEdges = true;
    }
    if (includeEdges == null) {
        includeEdges = true;
    }
    if (position.line < range.start.line) {
        return -1;
    }
    if (position.line > range.end.line) {
        return 1;
    }
    if (position.line === range.start.line) {
        if (includeEdges) {
            if (position.column < range.start.column) {
                return -1;
            }
        }
        else {
            if (position.column <= range.start.column) {
                return -1;
            }
        }
    }
    if (position.line === range.end.line) {
        if (includeEdges) {
            if (position.column > range.end.column) {
                return 1;
            }
        }
        else {
            if (position.column >= range.end.column) {
                return 1;
            }
        }
    }
    return 0;
}
exports.comparePositionAndRange = comparePositionAndRange;
function isPositionInsideRange(position, range, includeEdges) {
    if (!range) {
        return false;
    }
    return comparePositionAndRange(position, range, includeEdges) === 0;
}
exports.isPositionInsideRange = isPositionInsideRange;

//# sourceMappingURL=source-range.js.map
