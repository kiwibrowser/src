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
/**
 * The metadata for a JavaScript namespace.
 */
class ScannedNamespace {
    constructor(name, description, summary, astNode, jsdoc, sourceRange) {
        this.name = name;
        this.description = description;
        this.summary = summary;
        this.jsdoc = jsdoc;
        this.sourceRange = sourceRange;
        this.astNode = astNode;
        this.warnings = [];
    }
    resolve(_document) {
        return new Namespace(this);
    }
}
exports.ScannedNamespace = ScannedNamespace;
class Namespace {
    constructor(scannedNamespace) {
        this.name = scannedNamespace.name;
        this.description = scannedNamespace.description;
        this.summary = scannedNamespace.summary;
        this.kinds = new Set(['namespace']);
        this.identifiers = new Set([this.name]);
        this.sourceRange = scannedNamespace.sourceRange;
        this.astNode = scannedNamespace.astNode;
        this.warnings = Array.from(scannedNamespace.warnings);
    }
    toString() {
        return `<Namespace id=${this.name}>`;
    }
}
exports.Namespace = Namespace;

//# sourceMappingURL=namespace.js.map
