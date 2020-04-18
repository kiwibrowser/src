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
class ScannedFunction {
    constructor(name, description, summary, privacy, astNode, jsdoc, sourceRange, params, returnData) {
        this.name = name;
        this.description = description;
        this.summary = summary;
        this.jsdoc = jsdoc;
        this.sourceRange = sourceRange;
        this.astNode = astNode;
        this.warnings = [];
        this.params = params;
        this.return = returnData;
        this.privacy = privacy;
    }
    resolve(_document) {
        return new Function(this);
    }
}
exports.ScannedFunction = ScannedFunction;
class Function {
    constructor(scannedFunction) {
        this.name = scannedFunction.name;
        this.description = scannedFunction.description;
        this.summary = scannedFunction.summary;
        this.kinds = new Set(['function']);
        this.identifiers = new Set([this.name]);
        this.sourceRange = scannedFunction.sourceRange;
        this.astNode = scannedFunction.astNode;
        this.warnings = Array.from(scannedFunction.warnings);
        this.params = scannedFunction.params;
        this.return = scannedFunction.return;
        this.privacy = scannedFunction.privacy;
    }
    toString() {
        return `<Function id=${this.name}>`;
    }
}
exports.Function = Function;

//# sourceMappingURL=function.js.map
