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
class Feature {
    constructor(sourceRange, astNode, warnings) {
        this.kinds = new Set();
        this.identifiers = new Set();
        this.sourceRange = sourceRange;
        this.astNode = astNode;
        this.warnings = warnings || [];
    }
}
exports.Feature = Feature;
class ScannedFeature {
    constructor(sourceRange, astNode, description, jsdoc, warnings) {
        this.sourceRange = sourceRange;
        this.astNode = astNode;
        this.description = description;
        this.jsdoc = jsdoc;
        this.warnings = warnings || [];
    }
}
exports.ScannedFeature = ScannedFeature;

//# sourceMappingURL=feature.js.map
