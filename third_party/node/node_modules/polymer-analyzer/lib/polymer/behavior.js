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
const polymer_element_1 = require("../polymer/polymer-element");
/**
 * The metadata for a Polymer behavior mixin.
 */
class ScannedBehavior extends polymer_element_1.ScannedPolymerElement {
    resolve(document) {
        return new Behavior(this, document);
    }
}
exports.ScannedBehavior = ScannedBehavior;
class Behavior extends polymer_element_1.PolymerElement {
    constructor(scannedBehavior, document) {
        super(scannedBehavior, document);
        this.tagName = undefined;
        this.kinds.delete('element');
        this.kinds.delete('polymer-element');
        this.kinds.add('behavior');
    }
    toString() {
        return `<Behavior className=${this.className}>`;
    }
}
exports.Behavior = Behavior;

//# sourceMappingURL=behavior.js.map
