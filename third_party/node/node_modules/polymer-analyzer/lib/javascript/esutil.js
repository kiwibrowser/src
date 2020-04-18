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
const escodegen = require("escodegen");
const estraverse = require("estraverse");
const model_1 = require("../model/model");
const docs = require("../polymer/docs");
const docs_1 = require("../polymer/docs");
const jsdoc = require("./jsdoc");
/**
 * Returns whether an Espree node matches a particular object path.
 *
 * e.g. you have a MemberExpression node, and want to see whether it represents
 * `Foo.Bar.Baz`:
 *    matchesCallExpressio
    (node, ['Foo', 'Bar', 'Baz'])
 *
 * @param {ESTree.Node} expression The Espree node to match against.
 * @param {Array<string>} path The path to look for.
 */
function matchesCallExpression(expression, path) {
    if (!expression.property || !expression.object) {
        return false;
    }
    console.assert(path.length >= 2);
    if (expression.property.type !== 'Identifier') {
        return false;
    }
    // Unravel backwards, make sure properties match each step of the way.
    if (expression.property.name !== path[path.length - 1]) {
        return false;
    }
    // We've got ourselves a final member expression.
    if (path.length === 2 && expression.object.type === 'Identifier') {
        return expression.object.name === path[0];
    }
    // Nested expressions.
    if (path.length > 2 && expression.object.type === 'MemberExpression') {
        return matchesCallExpression(expression.object, path.slice(0, path.length - 1));
    }
    return false;
}
exports.matchesCallExpression = matchesCallExpression;
/**
 * @param {Node} key The node representing an object key or expression.
 * @return {string} The name of that key.
 */
function objectKeyToString(key) {
    if (key.type === 'Identifier') {
        return key.name;
    }
    if (key.type === 'Literal') {
        return '' + key.value;
    }
    if (key.type === 'MemberExpression') {
        return objectKeyToString(key.object) + '.' +
            objectKeyToString(key.property);
    }
    return undefined;
}
exports.objectKeyToString = objectKeyToString;
exports.CLOSURE_CONSTRUCTOR_MAP = new Map([['Boolean', 'boolean'], ['Number', 'number'], ['String', 'string']]);
/**
 * AST expression -> Closure type.
 *
 * Accepts literal values, and native constructors.
 *
 * @param {Node} node An Espree expression node.
 * @return {string} The type of that expression, in Closure terms.
 */
function closureType(node, sourceRange, document) {
    if (node.type.match(/Expression$/)) {
        return node.type.substr(0, node.type.length - 10);
    }
    else if (node.type === 'Literal') {
        return typeof node.value;
    }
    else if (node.type === 'Identifier') {
        return exports.CLOSURE_CONSTRUCTOR_MAP.get(node.name) || node.name;
    }
    else {
        throw new model_1.WarningCarryingException(new model_1.Warning({
            code: 'no-closure-type',
            message: `Unable to determine closure type for expression of type ` +
                `${node.type}`,
            severity: model_1.Severity.WARNING, sourceRange,
            parsedDocument: document,
        }));
    }
}
exports.closureType = closureType;
function getAttachedComment(node) {
    const comments = getLeadingComments(node) || [];
    return comments && comments[comments.length - 1];
}
exports.getAttachedComment = getAttachedComment;
/**
 * Returns all comments from a tree defined with @event.
 */
function getEventComments(node) {
    const eventComments = new Set();
    estraverse.traverse(node, {
        enter: (node) => {
            (node.leadingComments || [])
                .concat(node.trailingComments || [])
                .map((commentAST) => commentAST.value)
                .filter((comment) => comment.indexOf('@event') !== -1)
                .forEach((comment) => eventComments.add(comment));
        }
    });
    const events = [...eventComments]
        .map((comment) => docs_1.annotateEvent(jsdoc.parseJsdoc(jsdoc.removeLeadingAsterisks(comment).trim())))
        .filter((ev) => !!ev)
        .sort((ev1, ev2) => ev1.name.localeCompare(ev2.name));
    return new Map(events.map((e) => [e.name, e]));
}
exports.getEventComments = getEventComments;
function getLeadingComments(node) {
    if (!node) {
        return;
    }
    const comments = [];
    for (const comment of node.leadingComments || []) {
        // Espree says any comment that immediately precedes a node is "leading",
        // but we want to be stricter and require them to be touching. If we don't
        // have locations for some reason, err on the side of including the
        // comment.
        if (!node.loc || !comment.loc ||
            node.loc.start.line - comment.loc.end.line < 2) {
            comments.push(comment.value);
        }
    }
    return comments.length ? comments : undefined;
}
function getPropertyValue(node, name) {
    const properties = node.properties;
    for (const property of properties) {
        if (objectKeyToString(property.key) === name) {
            return property.value;
        }
    }
}
exports.getPropertyValue = getPropertyValue;
function isFunctionType(node) {
    return node.type === 'ArrowFunctionExpression' ||
        node.type === 'FunctionExpression' || node.type === 'FunctionDeclaration';
}
exports.isFunctionType = isFunctionType;
/**
 * Create a ScannedMethod object from an estree Property AST node.
 */
function toScannedMethod(node, sourceRange, document) {
    const parsedJsdoc = jsdoc.parseJsdoc(getAttachedComment(node) || '');
    const description = parsedJsdoc.description.trim();
    const maybeName = objectKeyToString(node.key);
    const warnings = [];
    if (!maybeName) {
        warnings.push(new model_1.Warning({
            code: 'unknown-method-name',
            message: `Could not determine name of method from expression of type: ` +
                `${node.key.type}`,
            sourceRange: sourceRange,
            severity: model_1.Severity.INFO,
            parsedDocument: document
        }));
    }
    let type = closureType(node.value, sourceRange, document);
    const typeTag = jsdoc.getTag(parsedJsdoc, 'type');
    if (typeTag) {
        type = doctrine.type.stringify(typeTag.type) || type;
    }
    const name = maybeName || '';
    const scannedMethod = {
        name,
        type,
        description,
        sourceRange,
        warnings,
        astNode: node,
        jsdoc: parsedJsdoc,
        privacy: getOrInferPrivacy(name, parsedJsdoc)
    };
    const value = node.value;
    if (value.type === 'FunctionExpression' ||
        value.type === 'ArrowFunctionExpression') {
        const paramTags = new Map();
        if (scannedMethod.jsdoc) {
            for (const tag of (scannedMethod.jsdoc.tags || [])) {
                if (tag.title === 'param' && tag.name) {
                    paramTags.set(tag.name, tag);
                }
                else if (tag.title === 'return' || tag.title === 'returns') {
                    scannedMethod.return = {};
                    if (tag.type) {
                        scannedMethod.return.type = doctrine.type.stringify(tag.type);
                    }
                    if (tag.description) {
                        scannedMethod.return.desc = tag.description;
                    }
                }
            }
        }
        scannedMethod.params = (value.params || []).map((nodeParam) => {
            let type = undefined;
            let description = undefined;
            // With ES6 we can have a lot of param patterns. Best to leave the
            // formatting to escodegen.
            const name = escodegen.generate(nodeParam);
            const tag = paramTags.get(name);
            if (tag) {
                if (tag.type) {
                    type = doctrine.type.stringify(tag.type);
                }
                if (tag.description) {
                    description = tag.description;
                }
            }
            return { name, type, description };
        });
    }
    return scannedMethod;
}
exports.toScannedMethod = toScannedMethod;
function getOrInferPrivacy(name, annotation, defaultPrivacy = 'public') {
    const explicitPrivacy = jsdoc.getPrivacy(annotation);
    const specificName = name.slice(name.lastIndexOf('.') + 1);
    if (explicitPrivacy) {
        return explicitPrivacy;
    }
    if (specificName.startsWith('__')) {
        return 'private';
    }
    else if (specificName.startsWith('_')) {
        return 'protected';
    }
    else if (specificName.endsWith('_')) {
        return 'private';
    }
    else if (exports.configurationProperties.has(specificName)) {
        return 'protected';
    }
    return defaultPrivacy;
}
exports.getOrInferPrivacy = getOrInferPrivacy;
/**
 * Properties on element prototypes that are part of the custom elment lifecycle
 * or Polymer configuration syntax.
 *
 * TODO(rictic): only treat the Polymer ones as private when dealing with
 *   Polymer.
 */
exports.configurationProperties = new Set([
    'attached',
    'attributeChanged',
    'beforeRegister',
    'configure',
    'constructor',
    'created',
    'detached',
    'enableCustomStyleProperties',
    'extends',
    'hostAttributes',
    'is',
    'listeners',
    'mixins',
    'observers',
    'properties',
    'ready',
    'registered',
]);
/**
 * Scan any methods on the given node, if it's a class expression/declaration.
 */
function getMethods(node, document) {
    const methods = new Map();
    for (const statement of _getMethods(node)) {
        if (statement.static === false) {
            const method = toScannedMethod(statement, document.sourceRangeForNode(statement), document);
            docs.annotate(method);
            methods.set(method.name, method);
        }
    }
    return methods;
}
exports.getMethods = getMethods;
/**
 * Scan any static methods on the given node, if it's a class
 * expression/declaration.
 */
function getStaticMethods(node, document) {
    const methods = new Map();
    for (const method of _getMethods(node)) {
        if (method.static === true) {
            const scannedMethod = toScannedMethod(method, document.sourceRangeForNode(method), document);
            docs.annotate(scannedMethod);
            methods.set(scannedMethod.name, scannedMethod);
        }
    }
    return methods;
}
exports.getStaticMethods = getStaticMethods;
function* _getMethods(node) {
    if (node.type !== 'ClassDeclaration' && node.type !== 'ClassExpression') {
        return;
    }
    for (const statement of node.body.body) {
        if (statement.type === 'MethodDefinition' && statement.kind === 'method') {
            yield statement;
        }
    }
}

//# sourceMappingURL=esutil.js.map
