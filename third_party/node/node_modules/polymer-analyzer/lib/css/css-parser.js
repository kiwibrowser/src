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
const shadyCss = require("shady-css-parser");
const css_document_1 = require("./css-document");
class CssParser {
    constructor() {
        this._parser = new shadyCss.Parser();
    }
    parse(contents, url, inlineInfo) {
        const ast = this._parser.parse(contents);
        const isInline = !!inlineInfo;
        inlineInfo = inlineInfo || {};
        return new css_document_1.ParsedCssDocument({
            url,
            contents,
            ast,
            locationOffset: inlineInfo.locationOffset,
            astNode: inlineInfo.astNode, isInline,
        });
    }
}
exports.CssParser = CssParser;

//# sourceMappingURL=css-parser.js.map
