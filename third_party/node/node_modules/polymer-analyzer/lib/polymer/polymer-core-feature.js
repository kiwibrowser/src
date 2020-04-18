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
const class_1 = require("../model/class");
const model_1 = require("../model/model");
/**
 * A scanned Polymer 1.x core "feature".
 */
class ScannedPolymerCoreFeature extends model_1.ScannedFeature {
    constructor() {
        super(...arguments);
        this.warnings = [];
        this.properties = new Map();
        this.methods = new Map();
    }
    resolve(document) {
        if (this.warnings.length > 0) {
            return;
        }
        // Sniff for the root `_addFeatures` call by presence of this method. This
        // method must only appear once (in the root `polymer.html` document).
        const isRootAddFeatureCall = this.methods.has('_finishRegisterFeatures');
        if (!isRootAddFeatureCall) {
            // We're at the `Polymer.Base` assignment or a regular `_addFeature`
            // call. We'll merge all of these below.
            return new PolymerCoreFeature(this.properties, this.methods);
        }
        // Otherwise we're at the root of the `_addFeatures` tree. Thanks to HTML
        // imports, we can be sure that this is the final core feature we'll
        // resolve. Now we'll emit a "fake" class representing the union of all the
        // feature objects.
        const allProperties = new Map(this.properties);
        const allMethods = new Map(this.methods);
        const otherCoreFeatures = document.getFeatures({
            kind: 'polymer-core-feature',
            imported: true,
            externalPackages: true,
        });
        for (const feature of otherCoreFeatures) {
            for (const [key, prop] of feature.properties) {
                allProperties.set(key, prop);
            }
            for (const [key, method] of feature.methods) {
                allMethods.set(key, method);
            }
        }
        return new class_1.Class({
            name: 'Polymer.Base',
            className: 'Polymer.Base',
            properties: allProperties,
            methods: allMethods,
            staticMethods: new Map(),
            abstract: true,
            privacy: 'public',
            // TODO(aomarks) The following should probably come from the
            // Polymer.Base assignment instead of this final addFeature call.
            jsdoc: this.jsdoc,
            description: this.description || '',
            summary: '',
            sourceRange: this.sourceRange,
            astNode: this.astNode,
        }, document);
    }
}
exports.ScannedPolymerCoreFeature = ScannedPolymerCoreFeature;
/**
 * A resolved Polymer 1.x core "feature".
 */
class PolymerCoreFeature {
    constructor(properties, methods) {
        this.properties = properties;
        this.methods = methods;
        this.kinds = new Set(['polymer-core-feature']);
        this.identifiers = new Set();
        this.warnings = [];
    }
}
exports.PolymerCoreFeature = PolymerCoreFeature;

//# sourceMappingURL=polymer-core-feature.js.map
