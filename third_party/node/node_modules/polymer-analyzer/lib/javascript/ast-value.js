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
const jsdoc = require("./jsdoc");
/**
 * Converts an ast literal to its underlying valie.
 */
function literalToValue(literal) {
    return literal.value;
}
/**
 * Early evaluates a unary expression.
 */
function unaryToValue(unary) {
    const operand = expressionToValue(unary.argument);
    switch (unary.operator) {
        case '!':
            return !operand;
        case '-':
            return -operand;
        case '+':
            return +operand;
        case '~':
            return ~operand;
        case 'typeof':
            return typeof operand;
        case 'void':
            return void operand;
        case 'delete':
            return undefined;
        default:
            const never = unary.operator;
            throw new Error(`Unknown unary operator found: ${never}`);
    }
}
/**
 * Try to evaluate function bodies.
 */
function functionDeclarationToValue(fn) {
    if (fn.body.type === 'BlockStatement') {
        return blockStatementToValue(fn.body);
    }
}
function functionExpressionToValue(fn) {
    if (fn.body.type === 'BlockStatement') {
        return blockStatementToValue(fn.body);
    }
}
function arrowFunctionExpressionToValue(fn) {
    if (fn.body.type === 'BlockStatement') {
        return blockStatementToValue(fn.body);
    }
    else {
        return expressionToValue(fn.body);
    }
}
/**
 * Block statement: find last return statement, and return its value
 */
function blockStatementToValue(block) {
    for (let i = block.body.length - 1; i >= 0; i--) {
        const body = block.body[i];
        if (body.type === 'ReturnStatement') {
            return returnStatementToValue(body);
        }
    }
}
/**
 * Evaluates return's argument
 */
function returnStatementToValue(ret) {
    return expressionToValue(ret.argument || { type: 'Literal', value: null, raw: 'null' });
}
/**
 * Evaluate array expression
 */
function arrayExpressionToValue(arry) {
    const value = [];
    for (let i = 0; i < arry.elements.length; i++) {
        const v = expressionToValue(arry.elements[i]);
        if (v === undefined) {
            continue;
        }
        value.push(v);
    }
    return value;
}
/**
 * Evaluate object expression
 */
function objectExpressionToValue(obj) {
    const evaluatedObjectExpression = {};
    for (const prop of obj.properties) {
        if (prop.key.type !== 'Literal') {
            return;
        }
        const evaluatedKey = '' + literalToValue(prop.key);
        const evaluatedValue = expressionToValue(prop.value);
        if (evaluatedValue === undefined) {
            return;
        }
        evaluatedObjectExpression[evaluatedKey] = evaluatedValue;
    }
    return evaluatedObjectExpression;
}
/**
 * Binary expressions, like 5 + 5
 */
function binaryExpressionToValue(member) {
    const left = expressionToValue(member.left);
    const right = expressionToValue(member.right);
    if (left == null || right == null) {
        return;
    }
    if (member.operator === '+') {
        // We need to cast to `any` here because, while it's usually not the right
        // thing to do to use '+' on two values of a mix of types because it's
        // unpredictable, that is what the original code we're evaluating does.
        return left + right;
    }
    return;
}
/**
 * Tries to get the value of an expression. Returns undefined on failure.
 */
function expressionToValue(valueExpression) {
    switch (valueExpression.type) {
        case 'Literal':
            return literalToValue(valueExpression);
        case 'UnaryExpression':
            return unaryToValue(valueExpression);
        case 'FunctionDeclaration':
            return functionDeclarationToValue(valueExpression);
        case 'FunctionExpression':
            return functionExpressionToValue(valueExpression);
        case 'ArrowFunctionExpression':
            return arrowFunctionExpressionToValue(valueExpression);
        case 'ArrayExpression':
            return arrayExpressionToValue(valueExpression);
        case 'ObjectExpression':
            return objectExpressionToValue(valueExpression);
        case 'BinaryExpression':
            return binaryExpressionToValue(valueExpression);
        default:
            return;
    }
}
exports.expressionToValue = expressionToValue;
/**
 * Extracts the name of the identifier or `.` separated chain of identifiers.
 *
 * Returns undefined if the given node isn't a simple identifier or chain of
 * simple identifiers.
 */
function getIdentifierName(node) {
    if (node.type === 'Identifier') {
        return node.name;
    }
    if (node.type === 'MemberExpression') {
        const object = getIdentifierName(node.object);
        let property;
        if (node.computed) {
            property = expressionToValue(node.property);
        }
        else {
            property = getIdentifierName(node.property);
        }
        if (object != null && property != null) {
            return `${object}.${property}`;
        }
    }
}
exports.getIdentifierName = getIdentifierName;
/**
 * Formats the given identifier name under a namespace, if one is mentioned in
 * the commentedNode's comment. Otherwise, name is returned.
 */
function getNamespacedIdentifier(name, docs) {
    if (!docs) {
        return name;
    }
    const memberofTag = jsdoc.getTag(docs, 'memberof');
    const namespace = memberofTag && memberofTag.description;
    if (namespace) {
        const rightMostIdentifierName = name.substring(name.lastIndexOf('.') + 1);
        return namespace + '.' + rightMostIdentifierName;
    }
    else {
        return name;
    }
}
exports.getNamespacedIdentifier = getNamespacedIdentifier;
exports.CANT_CONVERT = 'UNKNOWN';

//# sourceMappingURL=ast-value.js.map
