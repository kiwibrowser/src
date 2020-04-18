"use strict";
/**
 * @license
 * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
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
function underlineCode(sourceRange, parsedDocument, colorizer = (s) => s) {
    const relativeRange = parsedDocument.absoluteToRelativeSourceRange(sourceRange);
    const code = _getRelavantSourceCode(relativeRange, parsedDocument);
    if (!code) {
        return undefined;
    }
    const outputLines = [];
    const lines = code.split('\n');
    let lineNum = relativeRange.start.line;
    for (const line of lines) {
        outputLines.push(line);
        outputLines.push(colorizer(getSquiggleUnderline(line, lineNum, relativeRange)));
        lineNum++;
    }
    return outputLines.join('\n');
}
exports.underlineCode = underlineCode;
function _getRelavantSourceCode(relativeRange, parsedDocument) {
    if (parsedDocument === null) {
        return;
    }
    const startOffset = parsedDocument.sourcePositionToOffset({ column: 0, line: relativeRange.start.line });
    let endOffset;
    if (parsedDocument.newlineIndexes.length === relativeRange.end.line) {
        endOffset = parsedDocument.sourcePositionToOffset({ column: 0, line: relativeRange.end.line + 1 });
    }
    else {
        endOffset = parsedDocument.sourcePositionToOffset({ column: -1, line: relativeRange.end.line + 1 });
    }
    return parsedDocument.contents.slice(startOffset, endOffset);
}
function getSquiggleUnderline(lineText, lineNum, sourceRange) {
    // We're on a middle line of a multiline range. Squiggle the entire line.
    if (lineNum !== sourceRange.start.line && lineNum !== sourceRange.end.line) {
        return '~'.repeat(lineText.length);
    }
    // The tricky case. Might be the start of a multiline range, or it might just
    // be a one-line range.
    if (lineNum === sourceRange.start.line) {
        const startColumn = sourceRange.start.column;
        const endColumn = sourceRange.end.line === sourceRange.start.line ?
            sourceRange.end.column :
            lineText.length;
        const prefix = lineText.slice(0, startColumn).replace(/[^\t]/g, ' ');
        if (startColumn === endColumn) {
            return prefix + '~'; // always draw at least one squiggle
        }
        return prefix + '~'.repeat(endColumn - startColumn);
    }
    // We're on the end line of a multiline range. Just squiggle up to the end
    // column.
    return '~'.repeat(sourceRange.end.column);
}

//# sourceMappingURL=code-printer.js.map
