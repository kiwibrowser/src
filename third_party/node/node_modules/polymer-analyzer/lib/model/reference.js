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
const feature_1 = require("./feature");
const immutable_1 = require("./immutable");
/**
 * A reference to another feature by identifier.
 */
class ScannedReference extends feature_1.ScannedFeature {
    constructor(identifier, sourceRange, astNode, description, jsdoc, warnings) {
        super(sourceRange, astNode, description, jsdoc, warnings);
        this.identifier = identifier;
    }
    resolve(_document) {
        // TODO(justinfagnani): include an actual reference?
        // Would need a way to get a Kind to pass to Document.getById().
        // Should Kind in getById by optional?
        return new Reference(this.identifier, this.sourceRange, this.astNode, this.warnings);
    }
}
exports.ScannedReference = ScannedReference;
/**
 * A reference to another feature by identifier.
 */
class Reference extends feature_1.Feature {
    constructor(identifier, sourceRange, astNode, warnings) {
        super(sourceRange, astNode, warnings);
        immutable_1.unsafeAsMutable(this.kinds).add('reference');
        this.identifier = identifier;
    }
}
exports.Reference = Reference;

//# sourceMappingURL=reference.js.map
