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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const doctrine = require("doctrine");
const ast_value_1 = require("./ast-value");
const esutil_1 = require("./esutil");
const function_1 = require("./function");
const jsdoc = require("./jsdoc");
class FunctionScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const visitor = new FunctionVisitor(document);
            yield visit(visitor);
            return { features: Array.from(visitor.functions) };
        });
    }
}
exports.FunctionScanner = FunctionScanner;
class FunctionVisitor {
    constructor(document) {
        this.functions = new Set();
        this.warnings = [];
        this.document = document;
    }
    /**
     * Scan standalone function declarations.
     */
    enterFunctionDeclaration(node, _parent) {
        this._initFunction(node, ast_value_1.getIdentifierName(node.id), node);
        return;
    }
    /**
     * Scan functions assigned to newly declared variables.
     */
    enterVariableDeclaration(node, _parent) {
        if (node.declarations.length !== 1) {
            return; // Ambiguous.
        }
        const declaration = node.declarations[0];
        const declarationId = declaration.id;
        const declarationValue = declaration.init;
        if (declarationValue && esutil_1.isFunctionType(declarationValue)) {
            return this._initFunction(node, esutil_1.objectKeyToString(declarationId), declarationValue);
        }
    }
    /**
     * Scan functions assigned to variables and object properties.
     */
    enterAssignmentExpression(node, parent) {
        if (esutil_1.isFunctionType(node.right)) {
            this._initFunction(parent, esutil_1.objectKeyToString(node.left), node.right);
        }
    }
    /**
     * Scan functions defined inside of object literals.
     */
    enterObjectExpression(node, _parent) {
        for (let i = 0; i < node.properties.length; i++) {
            const prop = node.properties[i];
            const propValue = prop.value;
            const name = esutil_1.objectKeyToString(prop.key);
            if (esutil_1.isFunctionType(propValue)) {
                this._initFunction(prop, name, propValue);
                continue;
            }
            const comment = esutil_1.getAttachedComment(prop) || '';
            const docs = jsdoc.parseJsdoc(comment);
            if (jsdoc.getTag(docs, 'function')) {
                this._initFunction(prop, name);
                continue;
            }
        }
    }
    _initFunction(node, analyzedName, _fn) {
        const comment = esutil_1.getAttachedComment(node);
        // Quickly filter down to potential candidates.
        if (!comment || comment.indexOf('@memberof') === -1) {
            return;
        }
        if (!analyzedName) {
            // TODO(fks): Propagate a warning if name could not be determined
            return;
        }
        const docs = jsdoc.parseJsdoc(comment);
        // TODO(justinfagnani): remove polymerMixin support
        if (jsdoc.hasTag(docs, 'mixinFunction') ||
            jsdoc.hasTag(docs, 'polymerMixin')) {
            // This is a mixin, not a normal function.
            return;
        }
        const functionName = ast_value_1.getNamespacedIdentifier(analyzedName, docs);
        const sourceRange = this.document.sourceRangeForNode(node);
        const returnTag = jsdoc.getTag(docs, 'return');
        const summaryTag = jsdoc.getTag(docs, 'summary');
        const summary = (summaryTag && summaryTag.description) || '';
        const description = docs.description;
        let functionReturn;
        if (returnTag) {
            functionReturn = {
                type: returnTag.type ? doctrine.type.stringify(returnTag.type) :
                    undefined,
                desc: returnTag.description || '',
            };
        }
        // TODO(justinfagnani): consolidate with similar param processing code in
        // docs.ts
        const functionParams = [];
        if (docs.tags) {
            docs.tags.forEach((tag) => {
                if (tag.title !== 'param') {
                    return;
                }
                functionParams.push({
                    type: tag.type ? doctrine.type.stringify(tag.type) : 'N/A',
                    desc: tag.description || '',
                    name: tag.name || 'N/A'
                });
            });
        }
        // TODO(fks): parse params directly from `fn`, merge with docs.tags data
        const specificName = functionName.slice(functionName.lastIndexOf('.') + 1);
        this.functions.add(new function_1.ScannedFunction(functionName, description, summary, esutil_1.getOrInferPrivacy(specificName, docs), node, docs, sourceRange, functionParams, functionReturn));
    }
}

//# sourceMappingURL=function-scanner.js.map
