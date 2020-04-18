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
function __export(m) {
    for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];
}
Object.defineProperty(exports, "__esModule", { value: true });
__export(require("./class"));
var document_1 = require("./document");
exports.Document = document_1.Document;
exports.ScannedDocument = document_1.ScannedDocument;
__export(require("./element"));
__export(require("./element-base"));
var element_reference_1 = require("./element-reference");
exports.ElementReference = element_reference_1.ElementReference;
exports.ScannedElementReference = element_reference_1.ScannedElementReference;
__export(require("./element-mixin"));
__export(require("./feature"));
__export(require("./import"));
__export(require("./inline-document"));
var analysis_1 = require("./analysis");
exports.Analysis = analysis_1.Analysis;
__export(require("./reference"));
__export(require("./resolvable"));
__export(require("./source-range"));
__export(require("./warning"));

//# sourceMappingURL=model.js.map
