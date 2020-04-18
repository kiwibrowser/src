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
const ast_value_1 = require("./ast-value");
const esutil = require("./esutil");
const jsdoc = require("./jsdoc");
const namespace_1 = require("./namespace");
class NamespaceScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const visitor = new NamespaceVisitor(document);
            yield visit(visitor);
            return {
                features: Array.from(visitor.namespaces),
                warnings: visitor.warnings
            };
        });
    }
}
exports.NamespaceScanner = NamespaceScanner;
class NamespaceVisitor {
    constructor(document) {
        this.namespaces = new Set();
        this.warnings = [];
        this.document = document;
    }
    /**
     * Look for object declarations with @namespace in the docs.
     */
    enterVariableDeclaration(node, _parent) {
        if (node.declarations.length !== 1) {
            return; // Ambiguous.
        }
        this._initNamespace(node, node.declarations[0].id);
    }
    /**
     * Look for object assignments with @namespace in the docs.
     */
    enterAssignmentExpression(node, parent) {
        this._initNamespace(parent, node.left);
    }
    enterProperty(node, _parent) {
        this._initNamespace(node, node.key);
    }
    _initNamespace(node, nameNode) {
        const comment = esutil.getAttachedComment(node);
        // Quickly filter down to potential candidates.
        if (!comment || comment.indexOf('@namespace') === -1) {
            return;
        }
        const analyzedName = ast_value_1.getIdentifierName(nameNode);
        const docs = jsdoc.parseJsdoc(comment);
        const namespaceTag = jsdoc.getTag(docs, 'namespace');
        const explicitName = namespaceTag && namespaceTag.name;
        let namespaceName;
        if (explicitName) {
            namespaceName = explicitName;
        }
        else if (analyzedName) {
            namespaceName = ast_value_1.getNamespacedIdentifier(analyzedName, docs);
        }
        else {
            // TODO(fks): Propagate a warning if name could not be determined
            return;
        }
        const sourceRange = this.document.sourceRangeForNode(node);
        if (!sourceRange) {
            throw new Error(`Unable to determine sourceRange for @namespace: ${comment}`);
        }
        const summaryTag = jsdoc.getTag(docs, 'summary');
        const summary = (summaryTag && summaryTag.description) || '';
        const description = docs.description;
        this.namespaces.add(new namespace_1.ScannedNamespace(namespaceName, description, summary, node, docs, sourceRange));
    }
}

//# sourceMappingURL=namespace-scanner.js.map
