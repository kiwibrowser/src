"use strict";
/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
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
const model_1 = require("./model");
class ScannedElementMixin extends model_1.ScannedElementBase {
    constructor({ name }) {
        super();
        this.abstract = false;
        this.name = name;
    }
    resolve(document) {
        return new ElementMixin(this, document);
    }
}
exports.ScannedElementMixin = ScannedElementMixin;
class ElementMixin extends model_1.ElementBase {
    constructor(init, document) {
        super(init, document);
        this.kinds.add('element-mixin');
    }
}
exports.ElementMixin = ElementMixin;

//# sourceMappingURL=element-mixin.js.map
