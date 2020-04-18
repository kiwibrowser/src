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
const doctrine = require("doctrine");
const esutil_1 = require("../javascript/esutil");
const jsdoc = require("../javascript/jsdoc");
const model_1 = require("../model/model");
/**
 * Create a ScannedProperty object from an estree Property AST node.
 */
function toScannedPolymerProperty(node, sourceRange, document) {
    const parsedJsdoc = jsdoc.parseJsdoc(esutil_1.getAttachedComment(node) || '');
    const description = parsedJsdoc.description.trim();
    const maybeName = esutil_1.objectKeyToString(node.key);
    const warnings = [];
    if (!maybeName) {
        warnings.push(new model_1.Warning({
            code: 'unknown-prop-name',
            message: `Could not determine name of property from expression of type: ` +
                `${node.key.type}`,
            sourceRange: sourceRange,
            severity: model_1.Severity.WARNING,
            parsedDocument: document
        }));
    }
    let type = esutil_1.closureType(node.value, sourceRange, document);
    const typeTag = jsdoc.getTag(parsedJsdoc, 'type');
    if (typeTag) {
        type = doctrine.type.stringify(typeTag.type) || type;
    }
    const name = maybeName || '';
    const result = {
        name,
        type,
        description,
        sourceRange,
        warnings,
        astNode: node,
        isConfiguration: esutil_1.configurationProperties.has(name),
        jsdoc: parsedJsdoc,
        privacy: esutil_1.getOrInferPrivacy(name, parsedJsdoc)
    };
    return result;
}
exports.toScannedPolymerProperty = toScannedPolymerProperty;
;

//# sourceMappingURL=js-utils.js.map
