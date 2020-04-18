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
const element_base_1 = require("./element-base");
class ScannedElement extends element_base_1.ScannedElementBase {
    get name() {
        return this.className;
    }
    resolve(document) {
        return new Element(this, document);
    }
}
exports.ScannedElement = ScannedElement;
class Element extends element_base_1.ElementBase {
    constructor(init, document) {
        super(init, document);
        this.tagName = init.tagName;
        if (this.tagName) {
            this.identifiers.add(this.tagName);
        }
        this.kinds.add('element');
        this.extends = init.extends;
    }
}
exports.Element = Element;

//# sourceMappingURL=element.js.map
