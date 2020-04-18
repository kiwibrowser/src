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
class ElementReference {
    constructor(scannedRef, _document) {
        this.kinds = new Set(['element-reference']);
        this.tagName = scannedRef.tagName;
        this.attributes = scannedRef.attributes;
        this.sourceRange = scannedRef.sourceRange;
        this.astNode = scannedRef.astNode;
        this.warnings = scannedRef.warnings;
    }
    get identifiers() {
        return new Set([this.tagName]);
    }
}
exports.ElementReference = ElementReference;
class ScannedElementReference {
    constructor(tagName, sourceRange, ast) {
        this.attributes = new Map();
        this.warnings = [];
        this.tagName = tagName;
        this.sourceRange = sourceRange;
        this.astNode = ast;
    }
    resolve(document) {
        return new ElementReference(this, document);
    }
}
exports.ScannedElementReference = ScannedElementReference;

//# sourceMappingURL=element-reference.js.map
