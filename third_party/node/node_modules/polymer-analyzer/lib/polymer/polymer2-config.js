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
const ast_value_1 = require("../javascript/ast-value");
const analyze_properties_1 = require("./analyze-properties");
function getStaticGetterValue(node, name) {
    const getter = node.body.body.find((n) => n.type === 'MethodDefinition' && n.static === true &&
        n.kind === 'get' && ast_value_1.getIdentifierName(n.key) === name);
    if (!getter) {
        return undefined;
    }
    // TODO(justinfagnani): consider generating warnings for these checks
    const getterBody = getter.value.body;
    if (getterBody.body.length !== 1) {
        // not a single statement function
        return undefined;
    }
    const statement = getterBody.body[0];
    if (statement.type !== 'ReturnStatement') {
        // we only support a return statement
        return undefined;
    }
    return statement.argument;
}
exports.getStaticGetterValue = getStaticGetterValue;
function getIsValue(node) {
    const getterValue = getStaticGetterValue(node, 'is');
    if (!getterValue || getterValue.type !== 'Literal') {
        // we only support literals
        return undefined;
    }
    if (typeof getterValue.value !== 'string') {
        return undefined;
    }
    return getterValue.value;
}
exports.getIsValue = getIsValue;
/**
 * Returns the properties defined in a Polymer config object literal.
 */
function getPolymerProperties(node, document) {
    if (node.type !== 'ClassDeclaration' && node.type !== 'ClassExpression') {
        return [];
    }
    const propertiesNode = getStaticGetterValue(node, 'properties');
    return propertiesNode ? analyze_properties_1.analyzeProperties(propertiesNode, document) : [];
}
exports.getPolymerProperties = getPolymerProperties;

//# sourceMappingURL=polymer2-config.js.map
