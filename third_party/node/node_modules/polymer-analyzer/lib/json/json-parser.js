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
const json_document_1 = require("./json-document");
class JsonParser {
    parse(contents, url, inlineDocInfo) {
        const isInline = !!inlineDocInfo;
        inlineDocInfo = inlineDocInfo || {};
        return new json_document_1.ParsedJsonDocument({
            url,
            contents,
            ast: JSON.parse(contents),
            locationOffset: inlineDocInfo.locationOffset,
            astNode: inlineDocInfo.astNode, isInline
        });
    }
}
exports.JsonParser = JsonParser;

//# sourceMappingURL=json-parser.js.map
